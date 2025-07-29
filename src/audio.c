#include "../vendor/miniaudio/miniaudio.c"
#include <fftw3.h>
#include <stddef.h>
#include <stdlib.h>


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
} AudioAnalysis;

typedef struct {
	double **freq_bins; // Frequency bins for FFT results for each channel
	float *norm_avg; // Average normalized value for each channel
} AudioAnalysisResults;

typedef struct {
	AudioFormat playback;
	AudioFormat capture;
	AudioBuffer buffer;
	AudioAnalysis analysis; // Analysis data for FFT
	AudioAnalysisResults analysis_results; // Results of the analysis
} AudioData;

static AudioData *g_audio_data;

static ma_device device;

static ma_pcm_rb rb;

AudioData *get_audio_data() {
	if (g_audio_data == NULL) {
		printf("Audio data is not initialized\n");
		return NULL;
	}
	return g_audio_data;
}


void proc_audio(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
	if (frameCount == 0) return;

	const ma_int32 *input_samples = (const ma_int32 *)pInput;

	for (ma_uint32 i = 0; i < g_audio_data->capture.channels; i++) {
		float sum = 0.0f;
		for (ma_uint32 j = 0; j < frameCount; j++) {
			// Calculate the index in the circular buffer
			size_t buffer_index = (g_audio_data->buffer.frames_cursor + j) % g_audio_data->buffer.size;

			size_t corrected_frame_index = j% g_audio_data->buffer.size;
			// printf("Processing channel %d, frame %d, buffer index %zu, cursor %zu \n", i, j, buffer_index, g_audio_data->buffer.frames_cursor);
			// printf("Processing channel %d, frame %d, buffer index %zu\n", i, j, buffer_index);
			// Copy the sample for this channel
			g_audio_data->buffer.frames[i][buffer_index] = input_samples[i * frameCount + j];

			g_audio_data->analysis.fft_in[corrected_frame_index] = (double)g_audio_data->buffer.frames[i][buffer_index] / 2147483648.0; // Normalize to [-1.0, 1.0] range

			sum += (float)g_audio_data->buffer.frames[i][buffer_index]/ 2147483648.0f; // Normalize to [-1.0, 1.0] range
		}
		fftw_execute(g_audio_data->analysis.fft_plan); // Execute FFT for this channel
		// Store the FFT results in the frequency bins
		for (ma_uint32 j = 0; j < frameCount; j++) {
			size_t freq_bin_index = (g_audio_data->buffer.frames_cursor + j) % g_audio_data->buffer.size;
			g_audio_data->analysis_results.freq_bins[i][freq_bin_index] = fabs(g_audio_data->analysis.fft_out[j]); // Store FFT output
		}
		g_audio_data->analysis_results.norm_avg[i] = sum / frameCount; // Calculate average for this channel

	}

	// Update cursor with wrapping
	g_audio_data->buffer.frames_cursor = (g_audio_data->buffer.frames_cursor + frameCount) % g_audio_data->buffer.size;

	// Update frame count
	if (g_audio_data->buffer.frames_count < g_audio_data->buffer.size) {
		g_audio_data->buffer.frames_count += frameCount;
		if (g_audio_data->buffer.frames_count > g_audio_data->buffer.size) {
			g_audio_data->buffer.frames_count = g_audio_data->buffer.size;
		}
	}
	MA_COPY_MEMORY(pOutput, pInput, frameCount * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels));
}

AudioData *_init_audio_data(size_t buffer_size) {
	AudioData *audio_data = (AudioData *)malloc(sizeof(AudioData));
	if (audio_data == NULL) {
		printf("Failed to allocate memory for audio data\n");
		return NULL;
	}

	audio_data->capture.format = ma_format_s32; // 32-bit signed integer format
	audio_data->capture.channels = 2; // Stereo
	audio_data->playback.format = ma_format_s32; // 32-bit signed integer format
	audio_data->playback.channels = 2; // Stereo
	audio_data->buffer.size = buffer_size; // Buffer size
	audio_data->buffer.frames_count = 0;
	audio_data->buffer.frames_cursor = 0;
	audio_data->buffer.frames = malloc(sizeof(ma_int32 *) * audio_data->capture.channels);
	for (size_t i = 0; i < audio_data->capture.channels; i++) {
		audio_data->buffer.frames[i] = malloc(sizeof(ma_int32) * audio_data->buffer.size);
	}

	audio_data->analysis_results.norm_avg = malloc(sizeof(float) * audio_data->capture.channels);
	audio_data->analysis_results.freq_bins = malloc(sizeof(double *) * audio_data->capture.channels);
	for (size_t i = 0; i < audio_data->capture.channels; i++) {
		audio_data->analysis_results.freq_bins[i] = malloc(sizeof(double) * audio_data->buffer.size);
	}

	audio_data->analysis.fft_in = (double*) fftw_malloc(sizeof(double) * audio_data->buffer.size);
	audio_data->analysis.fft_out = (double*) fftw_malloc(sizeof(double) * audio_data->buffer.size);
	audio_data->analysis.fft_plan = fftw_plan_r2r_1d(
		audio_data->buffer.size,
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
	deviceConfig.pUserData = audio_data;
	deviceConfig.dataCallback = proc_audio;
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
	
	g_audio_data = _init_audio_data(buffer_size);

	if(_init_device(g_audio_data) != 0) {
		printf("Failed to initialize audio device\n");
		abort();
	}

	return 0;
}

void close_audio() {
	if (g_audio_data == NULL) {
		printf("Audio data is not initialized\n");
		return;
	}

	ma_device_uninit(&device);

	for (size_t i = 0; i < g_audio_data->capture.channels; i++) {
		free(g_audio_data->buffer.frames[i]);
	}
	free(g_audio_data->buffer.frames);
	fftw_destroy_plan(g_audio_data->analysis.fft_plan);
	fftw_cleanup();
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

