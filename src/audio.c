#include "../vendor/miniaudio/miniaudio.c"
#include <fftw3.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>    

/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

typedef struct {
	ma_format format;
	ma_uint32 channels;
} AudioFormat;

typedef struct {
	size_t 		size;
	size_t 		frames_count;
	size_t 		frames_cursor;
	ma_int32 	**frames;
} AudioBuffer;

typedef struct {
	double *fft_in; // Input for FFT
	double *fft_out; // Output for FFT
	fftw_plan fft_plan; // FFT plan
	size_t fft_size; // Size of the FFT
} AudioAnalysis;

typedef struct {
	double **freq_bins; // Frequency bins for FFT results for each channel
	float *norm_avg; // Average normalized value for each channel
	ma_uint32 acquired_frames; // Number of frames acquired for analysis
	// AudioBuffer buffer; // Buffer to hold audio data for analysis, will be larger than the ring buffer
} AudioAnalysisResults;

typedef struct {
	AudioFormat playback;
	AudioFormat capture;
	ma_uint32 sample_rate; // Sample rate for audio processing
	ma_pcm_rb buffer; //ring buffer to transport audio data
	size_t buffer_size_in_frames; // Size of the ring buffer
	AudioAnalysis analysis; // Analysis data for FFT
	AudioAnalysisResults analysis_results; // Results of the analysis
} AudioData;

static AudioData *g_audio_data;

static ma_device device;

static int _is_fft_running = 0; // Flag to indicate if the FFT thread is running

static pthread_t fft_thread;

AudioData *get_audio_data() {
	if (g_audio_data == NULL) {
		printf("Audio data is not initialized\n");
		return NULL;
	}
	return g_audio_data;
}

void* fft_loop(void *arg) {
	printf("FFT thread started\n");
	// This function is intended to run in a separate thread to process the audio data
	// and perform FFT analysis on the captured audio.
	// It will continuously read from the ring buffer and perform FFT on the data.
	while (_is_fft_running) {
		if (g_audio_data == NULL) {
			printf("Audio data is not initialized\n");
			abort();
		}

		// Acquire read access to the ring buffer
		void *pBuffer;
		ma_uint32 sizeInFrames = g_audio_data->buffer_size_in_frames;
		ma_result result = ma_pcm_rb_acquire_read(&g_audio_data->buffer, &sizeInFrames, &pBuffer);
		if (result != MA_SUCCESS) {
			printf("Failed to acquire read buffer: %s\n", ma_result_description(result));
			continue; // Retry in the next iteration
		}

		if (sizeInFrames == 0) {
			ma_pcm_rb_commit_read(&g_audio_data->buffer, 0);
			continue; // No data to process
		}
		g_audio_data->analysis_results.acquired_frames = sizeInFrames;
		printf("FFT thread acquired read buffer with size: %u frames\n", sizeInFrames);

		for (ma_uint32 j = 0; j < sizeInFrames; j++) {
			for (ma_uint32 i = 0; i < g_audio_data->capture.channels; i++) {
				g_audio_data->analysis.fft_in[j] = ((ma_int32 *)pBuffer)[j * g_audio_data->capture.channels + i]; // / 2147483648.0; // Normalize to [-1.0, 1.0]
			}
		}
		for (ma_uint32 i = 0; i < g_audio_data->capture.channels; i++) {
			fftw_execute(g_audio_data->analysis.fft_plan); // Execute FFT for this channel

			for (ma_uint32 j = 0; j < sizeInFrames; j++) {
				g_audio_data->analysis_results.freq_bins[i][j] = fabs(g_audio_data->analysis.fft_out[j]); // Store FFT output
			}
			float sum = 0.0f;
			for (ma_uint32 j = 0; j < sizeInFrames; j++) {
				sum += g_audio_data->analysis.fft_in[j];
			}
			g_audio_data->analysis_results.norm_avg[i] = sum / sizeInFrames; // Calculate average for this channel
		}

		// Release the read access to the ring buffer
		result = ma_pcm_rb_commit_read(&g_audio_data->buffer, sizeInFrames);
		if (result != MA_SUCCESS) {
			printf("Failed to release read buffer: %s\n", ma_result_description(result));
			continue; // Retry in the next iteration
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
	ma_result result = ma_pcm_rb_acquire_write(&audio_data->buffer, &sizeInFrames, &pBuffer);
	if (result != MA_SUCCESS) {
		printf("Failed to acquire write buffer: %s\n", ma_result_description(result));
		return;
	}
	// Copy the input samples to the ring buffer
	ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels);
	ma_uint32 bytesToWrite = sizeInFrames * bytesPerFrame;
	MA_COPY_MEMORY(pBuffer, pInput, bytesToWrite);
	result = ma_pcm_rb_commit_write(&audio_data->buffer, sizeInFrames);
	if (result != MA_SUCCESS) {
		printf("Failed to commit write buffer: %s\n", ma_result_description(result));
		return;
	}

	MA_COPY_MEMORY(pOutput, pInput, frameCount * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels));
}

AudioData *_init_audio_data(
	ma_uint32 sample_rate,
	size_t buffer_size
) {
	AudioData *audio_data = (AudioData *)malloc(sizeof(AudioData));
	if (audio_data == NULL) {
		printf("Failed to allocate memory for audio data\n");
		return NULL;
	}

	audio_data->capture.format = ma_format_s32; // 32-bit signed integer format
	audio_data->capture.channels = 2; // Stereo
	audio_data->playback.format = ma_format_s32; // 32-bit signed integer format
	audio_data->playback.channels = 2; // Stereo
	audio_data->buffer_size_in_frames = buffer_size;
	audio_data->sample_rate = sample_rate;
	ma_result result = ma_pcm_rb_init(
		audio_data->capture.format,
		audio_data->capture.channels,
		audio_data->buffer_size_in_frames,
		NULL,
		NULL,
		&audio_data->buffer
	);
	if (result != MA_SUCCESS) {
		printf("Failed to initialize PCM ring buffer: %s\n", ma_result_description(result));
		return NULL;
	}
	

	audio_data->analysis_results.norm_avg = malloc(sizeof(float) * audio_data->capture.channels);
	audio_data->analysis_results.freq_bins = malloc(sizeof(double *) * audio_data->capture.channels);
	for (size_t i = 0; i < audio_data->capture.channels; i++) {
		audio_data->analysis_results.freq_bins[i] = malloc(sizeof(double) * audio_data->buffer_size_in_frames);
	}

	audio_data->analysis.fft_size = buffer_size;

	audio_data->analysis.fft_in = (double*) fftw_malloc(sizeof(double) * audio_data->analysis.fft_size);
	audio_data->analysis.fft_out = (double*) fftw_malloc(sizeof(double) *  audio_data->analysis.fft_size);
	audio_data->analysis.fft_plan = fftw_plan_r2r_1d(
		audio_data->buffer_size_in_frames,
		audio_data->analysis.fft_in,
		audio_data->analysis.fft_out,
		FFTW_REDFT10,
		FFTW_ESTIMATE
	);

	return audio_data;
}

int _init_device(AudioData *audio_data) {
	ma_device_config deviceConfig = ma_device_config_init(ma_device_type_duplex);
	deviceConfig.capture.pDeviceID = NULL;
	deviceConfig.capture.format = audio_data->capture.format;
	deviceConfig.capture.channels = audio_data->capture.channels;
	deviceConfig.capture.shareMode = ma_share_mode_shared;
	deviceConfig.playback.pDeviceID = NULL;
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

int init_audio(
	ma_uint32 sample_rate,
	size_t buffer_size,
	ma_format capture_format,
	ma_uint32 capture_channels,
	ma_format playback_format,
	ma_uint32 playback_channels
)
{
	//--------------------------------------------------------------------------------------
	// Audio Initialization
	//--------------------------------------------------------------------------------------
	
	g_audio_data = _init_audio_data(sample_rate,buffer_size);

	if(_init_device(g_audio_data) != 0) {
		printf("Failed to initialize audio device\n");
		abort();
	}

	_is_fft_running = 1; // Set the flag to indicate that the FFT thread should run
	fft_thread = pthread_create(&fft_thread, NULL, fft_loop, NULL);
	// pthread_detach(fft_thread);

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
	ma_pcm_rb_uninit(&g_audio_data->buffer);

	// Free the FFTW resources
	fftw_destroy_plan(g_audio_data->analysis.fft_plan);
	fftw_cleanup();

	// Free the audio data structures
	free(g_audio_data->analysis_results.norm_avg);
	/*printf("Freeing FFT freq_bins\n");*/
	for (size_t i = 0; i < g_audio_data->capture.channels; i++) {
		free(g_audio_data->analysis_results.freq_bins[i]);
	}
	free(g_audio_data->analysis_results.freq_bins);


	free(g_audio_data);
	g_audio_data = NULL;

	printf("Audio closed successfully\n");
}

