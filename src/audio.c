#include "../vendor/miniaudio/miniaudio.c"
#include <fftw3.h>
#include <stddef.h>


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
	AudioFormat playback;
	AudioFormat capture;
	AudioBuffer buffer;
} AudioData;

static AudioData g_audio_data;

// static double *g_audio_fft_in;
// static double *g_audio_fft_out;
// static fftw_plan g_audio_fft_plan;
//
// static float g_audio_fft_results[AUDIO_INPUT_CHANNELS][AUDIO_FRAMES]={{0.0f}}; // FFT results for visualization
//
// void run_fft(ma_int32 *input, float *output, int n) {
//
// 	// Fill input array
// 	for (int i = 0; i < n; i++) {
// 		g_audio_fft_in[i] = (float)input[i]; // Real part
// 	}
//
// 	// Execute FFT
// 	fftw_execute(g_audio_fft_plan);
//
// 	// Copy output to output array
// 	for (int i = 0; i < n; i++) {
// 		output[i] = fabs((float)g_audio_fft_out[i]); // Real part of the output
// 		// printf("FFT Output[%d]: %f\n", i, output[i]); // Debug output
// 	}
//
// 	// Cleanup
// }

void proc_audio(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
	if (frameCount == 0) return;


	const ma_int32 *input_samples = (const ma_int32 *)pInput;
	AudioData *audio_data = (AudioData *)pDevice->pUserData;

	// Handle the copy with potential wrapping
	for (ma_uint32 i = 0; i < audio_data->capture.channels; i++) {
		float sum = 0.0f;
		for (ma_uint32 j = 0; j < frameCount; j++) {
			// Calculate the index in the circular buffer
			size_t buffer_index = (audio_data->buffer.frames_cursor + j) % audio_data->buffer.size;

			// Copy the sample for this channel
			audio_data->buffer.frames[i][buffer_index] = input_samples[i * frameCount + j];

			sum += (float)audio_data->buffer.frames[i][buffer_index]/ 2147483648.0f; // Normalize to [-1.0, 1.0] range
		}
		// g_audio_data.norm_avg[i] = sum / (float)frameCount; // Calculate average for this channel
		// Run FFT on the channel data
		// run_fft(audio_data->buffer.frames[i], g_audio_fft_results[i], AUDIO_FRAMES);

	}

	// Update cursor with wrapping
	audio_data->buffer.frames_cursor = (audio_data->buffer.frames_cursor + frameCount) % audio_data->buffer.size;

	// Update frame count
	if (audio_data->buffer.frames_count < audio_data->buffer.size) {
		audio_data->buffer.frames_count += frameCount;
		if (audio_data->buffer.frames_count > audio_data->buffer.size) {
			audio_data->buffer.frames_count = audio_data->buffer.size;
		}
	}
	MA_COPY_MEMORY(pOutput, pInput, frameCount * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels));
}

ma_device_config _init_device_config(AudioData *audio_data) {
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

	return deviceConfig;
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
	
	g_audio_data = (AudioData){
		.playback = {playback_format, playback_channels},
		.capture = {capture_format, capture_channels},
		.buffer = {
			.size = buffer_size,
			.frames_count = 0,
			.frames_cursor = 0,
			.frames = (ma_int32**)malloc(sizeof(ma_int32*) * capture_channels)
		}
	};



	ma_result result;
	ma_device_config deviceConfig;
	ma_device device;
	deviceConfig = _init_device_config(&g_audio_data);
	result = ma_device_init(NULL, &deviceConfig, &device);
	if (result != MA_SUCCESS) {
		printf("Failed to initialize audio device: %s\n", ma_result_description(result));
		return result;
	}
	ma_device_start(&device);
	//
	// g_audio_fft_in = (double*) fftw_malloc(sizeof(double) * AUDIO_FRAMES);
	// g_audio_fft_out = (double*) fftw_malloc(sizeof(double) * AUDIO_FRAMES);
	// g_audio_fft_plan = fftw_plan_r2r_1d(
	// 	AUDIO_FRAMES,
	// 	g_audio_fft_in,
	// 	g_audio_fft_out,
	// 	FFTW_REDFT10,
	// 	FFTW_ESTIMATE
	// );
	//
	// fftw_destroy_plan(g_audio_fft_plan);
	// fftw_free(g_audio_fft_in);
	// fftw_free(g_audio_fft_out);
	//
	return 0;
}

