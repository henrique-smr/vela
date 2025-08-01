#ifndef AUDIO_H
#define AUDIO_H
#define MA_IMPLEMENTATION
#include <fftw3.h>
#include <math.h>
#include <miniaudio.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
	ma_format format;
	ma_uint32 channels;

} AudioFormat;

typedef struct {
	size_t size;
	size_t frames_count;
	size_t frames_cursor;
	ma_int32 **frames;
} AudioBuffer;

typedef struct {
	double *fft_in;     // Input for FFT
	double *fft_out;    // Output for FFT
	fftw_plan fft_plan; // FFT plan
} AudioAnalysis;

typedef struct {
	double **freq_data; // Frequency bins for FFT results for each channel
	float *norm_avg;    // Average normalized value for each channel
} AudioAnalysisResults;

typedef struct {
	AudioFormat playback;
	AudioFormat capture;
	ma_uint32 sample_rate;        // Sample rate for audio processing
	ma_pcm_rb rb;                 // ring buffer to transport audio data
	size_t buffer_size_in_frames; // Size of the ring buffer
	AudioBuffer buffer; // Buffer to hold audio data for analysis, will be larger
	// than the ring buffer
	AudioAnalysis analysis;                // Analysis data for FFT
	AudioAnalysisResults analysis_results; // Results of the analysis
} AudioData;

typedef struct {
	ma_context context;
	ma_device_info* pPlaybackInfos;
	ma_uint32 playbackCount;
	ma_device_info* pCaptureInfos;
	ma_uint32 captureCount;


	ma_device_id *capture_device_index; // Index of the capture device
	ma_device_id *playback_device_index; // Index of the playback device
} AudioDeviceContext;

static AudioData *g_audio_data;

static ma_device device;

static int _is_fft_running = 0; // Flag to indicate if the FFT thread is running

static pthread_t fft_thread;

static int is_audio_initialized = 0; // Flag to indicate if audio is initialized


AudioData *get_audio_data() {
	if (g_audio_data == NULL) {
		printf("Audio data is not initialized\n");
		return NULL;
	}
	return g_audio_data;
}
void *fft_loop(void *arg) {
	printf("FFT thread started\n");
	// This function is intended to run in a separate thread to process the audio
	// data and perform FFT analysis on the captured audio. It will continuously
	// read from the ring buffer and perform FFT on the data.
	while (_is_fft_running) {
		if (g_audio_data == NULL) {
			printf("Audio data is not initialized\n");
			abort();
		}

		// Acquire read access to the ring buffer
		void *pBuffer;
		ma_uint32 sizeInFrames = g_audio_data->buffer_size_in_frames;
		ma_result result = ma_pcm_rb_acquire_read(&g_audio_data->rb, &sizeInFrames, &pBuffer);
		if (result != MA_SUCCESS) {
			printf("Failed to acquire read buffer: %s\n", ma_result_description(result));
			continue; // Retry in the next iteration
		}

		if (sizeInFrames == 0) {
			ma_pcm_rb_commit_read(&g_audio_data->rb, 0);
			continue; // No data to process
		}
		// printf("FFT thread acquired read buffer with size: %u frames\n",
		//        sizeInFrames);

		AudioBuffer *buffer = &g_audio_data->buffer;
		ma_uint32 channels = g_audio_data->capture.channels;

		// fill the buffer with the acquired frames
		for (ma_uint32 i = 0; i < channels; i++) {
			ma_uint32 buffer_frames_cursor;
			for (ma_uint32 j = 0; j < sizeInFrames; j++) {

				ma_uint32 frame_index = j * channels + i;

				buffer_frames_cursor = (j + buffer->frames_cursor) % buffer->size;

				g_audio_data->buffer.frames[i][buffer_frames_cursor] = ((ma_int32 *)pBuffer)[frame_index]; // Copy data to the buffer
				// Update the frames count
				if (buffer->frames_count < buffer->size) {
					buffer->frames_count++;
				}
				// Update the cursor for the next frame
			}
		}
		buffer->frames_cursor = (buffer->frames_cursor + sizeInFrames) % buffer->size; // Update the cursor for the next read
		// printf("FFT thread filled buffer at frame cursor: %zu\n", buffer->frames_cursor);
		// Release the read access to the ring buffer
		result = ma_pcm_rb_commit_read(&g_audio_data->rb, sizeInFrames);
		if (result != MA_SUCCESS) {
			printf("Failed to release read buffer: %s\n", ma_result_description(result));
			continue; // Retry in the next iteration
		}

		if (buffer->frames_count >= buffer->size) {
			for (ma_uint32 i = 0; i < channels; i++) {
				for (ma_uint32 j = 0; j < buffer->size; j++) {
					g_audio_data->analysis.fft_in[j] = (double)g_audio_data->buffer.frames[i][j]; // Fill FFT input with the buffer data
				}
				fftw_execute(g_audio_data->analysis.fft_plan); // Execute FFT for this channel

				for (ma_uint32 j = 0; j < buffer->size; j++) {
					double ssample = fabs(g_audio_data->analysis.fft_out[j]) / buffer->size; // Normalize the FFT output
					if (j == 0) {
						g_audio_data->analysis_results.freq_data[i][j] = 0; // Store FFT output
					} else {
						g_audio_data->analysis_results.freq_data[i][j] = ssample; // remove pink noise
					}
				}
				float sum = 0.0f;
				for (ma_uint32 j = 0; j < buffer->size; j++) {
					sum += g_audio_data->analysis.fft_in[j];
				}
				g_audio_data->analysis_results.norm_avg[i] =
					sum / buffer->size; // Calculate average for this channel
			}
		}
	}
	// After processing, we can stop the FFT thread
	return NULL;
}

void ma_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
	if (frameCount == 0) return;

	AudioData *audio_data = (AudioData *)pDevice->pUserData;

	void *pBuffer;
	ma_uint32 sizeInFrames = frameCount;
	ma_result result = ma_pcm_rb_acquire_write(&audio_data->rb, &sizeInFrames, &pBuffer);
	if (result != MA_SUCCESS) {
		printf("Failed to acquire write buffer: %s\n", ma_result_description(result));
		return;
	}
	// Copy the input samples to the ring buffer
	ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->capture.format,
																  pDevice->capture.channels);
	ma_uint32 bytesToWrite = sizeInFrames * bytesPerFrame;
	MA_COPY_MEMORY(pBuffer, pInput, bytesToWrite);
	result = ma_pcm_rb_commit_write(&audio_data->rb, sizeInFrames);
	if (result != MA_SUCCESS) {
		printf("Failed to commit write buffer: %s\n", ma_result_description(result));
		return;
	}

	MA_COPY_MEMORY(pOutput, pInput, frameCount * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels));
}

AudioData *_init_audio_data(ma_uint32 sample_rate, size_t buffer_size) {
	AudioData *audio_data = (AudioData *)malloc(sizeof(AudioData));
	if (audio_data == NULL) {
		printf("Failed to allocate memory for audio data\n");
		return NULL;
	}

	audio_data->capture.format = ma_format_s32;  // 32-bit signed integer format
	audio_data->capture.channels = 2;            // Stereo
	audio_data->playback.format = ma_format_s32; // 32-bit signed integer format
	audio_data->playback.channels = 2;           // Stereo
	audio_data->buffer_size_in_frames = buffer_size;
	audio_data->sample_rate = sample_rate;
	ma_result result = ma_pcm_rb_init(audio_data->capture.format, audio_data->capture.channels, 1200, NULL, NULL, &audio_data->rb);
	audio_data->buffer.size = audio_data->buffer_size_in_frames;
	audio_data->buffer.frames_count = 0;
	audio_data->buffer.frames_cursor = 0;
	audio_data->buffer.frames = malloc(sizeof(ma_int32 *) * audio_data->capture.channels);
	for (size_t i = 0; i < audio_data->capture.channels; i++) {
		audio_data->buffer.frames[i] = malloc(sizeof(ma_int32) * audio_data->buffer.size);
	}

	if (result != MA_SUCCESS) {
		printf("Failed to initialize PCM ring buffer: %s\n",
			ma_result_description(result));
		return NULL;
	}

	audio_data->analysis_results.norm_avg = malloc(sizeof(float) * audio_data->capture.channels);
	audio_data->analysis_results.freq_data = malloc(sizeof(double *) * audio_data->capture.channels);
	for (size_t i = 0; i < audio_data->capture.channels; i++) {
		audio_data->analysis_results.freq_data[i] = malloc(sizeof(double) * audio_data->buffer_size_in_frames);
	}

	audio_data->analysis.fft_in = (double *)fftw_malloc(sizeof(double) * audio_data->buffer_size_in_frames);
	audio_data->analysis.fft_out = (double *)fftw_malloc(sizeof(double) * audio_data->buffer_size_in_frames);
	audio_data->analysis.fft_plan = fftw_plan_r2r_1d(
		audio_data->buffer_size_in_frames, audio_data->analysis.fft_in,
		audio_data->analysis.fft_out, FFTW_REDFT10, FFTW_ESTIMATE);

	return audio_data;
}

int _init_device(AudioData *audio_data, ma_device_id* capture_device_index, ma_device_id* playback_device_index) {
	ma_device_config deviceConfig = ma_device_config_init(ma_device_type_duplex);

	deviceConfig.capture.pDeviceID = capture_device_index;
	deviceConfig.capture.format = audio_data->capture.format;
	deviceConfig.capture.channels = audio_data->capture.channels;
	deviceConfig.capture.shareMode = ma_share_mode_shared;
	deviceConfig.playback.pDeviceID = playback_device_index;
	deviceConfig.playback.format = audio_data->playback.format;
	deviceConfig.playback.channels = audio_data->playback.channels;
	deviceConfig.sampleRate = audio_data->sample_rate; // Set the sample rate
	deviceConfig.pUserData = audio_data;
	deviceConfig.dataCallback = ma_callback;
	ma_result result;
	result = ma_device_init(NULL, &deviceConfig, &device);
	if (result != MA_SUCCESS) {
		printf("Failed to initialize audio device: %s\n", ma_result_description(result));
		return result;
	}
	ma_device_start(&device);
	return 0;
}



int init_audio(ma_uint32 sample_rate, size_t buffer_size,
					ma_format capture_format, ma_uint32 capture_channels,
					ma_format playback_format, ma_uint32 playback_channels) {
	//--------------------------------------------------------------------------------------
	// Audio Initialization
	//--------------------------------------------------------------------------------------
	ma_context context;
	ma_device_info* pPlaybackInfos;
	ma_uint32 playbackCount;
	ma_device_info* pCaptureInfos;
	ma_uint32 captureCount;

	int* capture_device_index;
	int* playback_device_index;

	capture_device_index = malloc(sizeof(int));
	playback_device_index = malloc(sizeof(int));

	// *capture_device_index = 2; // Default to the first capture device
	// *playback_device_index = 1; // Default to the first playback device

	if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
		printf("Failed to initialize audio context\n");
		return -1;
	}
	if (ma_context_get_devices(&context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS) {
		printf("Failed to get audio devices\n");
		return -1;
	}
	if (captureCount == 0) {
		printf("No capture devices found\n");
		return -1;
	}
	if (playbackCount == 0) {
		printf("No playback devices found\n");
		return -1;
	}

	// Print available capture devices and get user input for selection
	printf("Available capture devices:\n");
	for (ma_uint32 i = 0; i < captureCount; i++) {
		printf("%d: %s\n", i, pCaptureInfos[i].name);
	}
	printf("Select a capture device (0-%d): ", captureCount - 1);
	if (scanf("%d", capture_device_index) != 1 || *capture_device_index < 0 || *capture_device_index >= captureCount) {
		printf("Invalid capture device index\n");
		return -1;
	}
	// Print available playback devices and get user input for selection
	printf("Available playback devices:\n");
	for (ma_uint32 i = 0; i < playbackCount; i++) {
		printf("%d: %s\n", i, pPlaybackInfos[i].name);
	}
	printf("Select a playback device (0-%d): ", playbackCount - 1);
	if (scanf("%d", playback_device_index) != 1 || *playback_device_index < 0 || *playback_device_index >= playbackCount) {
		printf("Invalid playback device index\n");
		return -1;
	}
	
	g_audio_data = _init_audio_data(sample_rate, buffer_size);

	if (_init_device(g_audio_data, &pCaptureInfos[*capture_device_index].id, &pPlaybackInfos[*playback_device_index].id) != 0) {
		printf("Failed to initialize audio device\n");
		abort();
	}

	_is_fft_running = 1; // Set the flag to indicate that the FFT thread should run
	fft_thread = pthread_create(&fft_thread, NULL, fft_loop, NULL);

	is_audio_initialized = 1; // Set the flag to indicate that audio is initialized

	return 0;
}

void close_audio() {
	if (g_audio_data == NULL) {
		printf("Audio data is not initialized\n");
		return;
	}
	_is_fft_running = 0; // Stop the FFT thread
	// pthread_cancel(fft_thread);

	// Stop and uninitialize the audio device
	ma_device_stop(&device);
	ma_device_uninit(&device);
	ma_pcm_rb_uninit(&g_audio_data->rb);

	// Free the FFTW resources
	fftw_destroy_plan(g_audio_data->analysis.fft_plan);
	fftw_cleanup();

	// Free the audio data structures
	for (size_t i = 0; i < g_audio_data->capture.channels; i++) {
		free(g_audio_data->buffer.frames[i]);
	}
	free(g_audio_data->buffer.frames);
	free(g_audio_data->analysis_results.norm_avg);
	/*printf("Freeing FFT freq_data\n");*/
	for (size_t i = 0; i < g_audio_data->capture.channels; i++) {
		free(g_audio_data->analysis_results.freq_data[i]);
	}
	free(g_audio_data->analysis_results.freq_data);

	free(g_audio_data);
	g_audio_data = NULL;

	printf("Audio closed successfully\n");
}

#endif // AUDIO_H
