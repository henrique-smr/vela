#ifndef AUDIO_H
#define AUDIO_H
#define MA_IMPLEMENTATION
#include <fftw3.h>
#include <miniaudio.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#ifndef SAMPLE_TYPE
#define SAMPLE_TYPE ma_int32
#endif

typedef struct {
	ma_pcm_rb rb;                 // ring buffer to transport audio data
} AudioData;

typedef struct {
	ma_uint32 sample_rate;
	size_t buffer_size;

	ma_format capture_format;
	ma_uint32 capture_channels;
	ma_device_id* capture_device_id;

	ma_format playback_format;
	ma_uint32 playback_channels;
	ma_device_id* playback_device_id;

} AudioConfig;

typedef struct {
	ma_device_info *capture_devices; // List of available capture devices
	ma_uint32 capture_device_count;   // Number of available capture devices
	ma_device_info *playback_devices; // List of available playback devices
	ma_uint32 playback_device_count;   // Number of available playback devices
} AudioDevicesInfo;

static AudioData *g_audio_data;

static ma_context g_audio_context;

static ma_device g_audio_device;

static int _is_audio_initialized = 0; // Flag to indicate if audio is initialized

AudioData *get_audio_data() {
	return g_audio_data;
}

int is_audio_initialized() {
	return _is_audio_initialized;
}

SAMPLE_TYPE* read_audio_data(ma_uint32 *sizeInFrames) {
	// This function is intended to run in a separate thread to read audio data from the ring buffer
	AudioData *audio_data = get_audio_data();
	if (audio_data == NULL) {
		printf("Audio data is not initialized\n");
		return NULL;
	}

	void *pBuffer;

	ma_result result = ma_pcm_rb_acquire_read(&audio_data->rb, sizeInFrames, &pBuffer);
	if (result != MA_SUCCESS) {
		printf("Failed to acquire read buffer: %s\n", ma_result_description(result));
		return NULL;
	}

	if (sizeInFrames == 0) {
		ma_pcm_rb_commit_read(&audio_data->rb, 0);
		return NULL; // No data to read
	}


	return (SAMPLE_TYPE *)pBuffer; // Return the pointer to the read buffer
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

AudioData *_init_audio_data(AudioConfig *config) {
	AudioData *audio_data = (AudioData *)malloc(sizeof(AudioData));
	ma_result result = ma_pcm_rb_init(
		config->capture_format,
		config->capture_channels,
		1200,
		NULL,
		NULL,
		&audio_data->rb
	);

	if (result != MA_SUCCESS) {
		printf("Failed to initialize PCM ring buffer: %s\n", ma_result_description(result));
		return NULL;
	}

	return audio_data;

}

AudioConfig init_audio_config() {
	AudioConfig config;
	config.sample_rate = 48000; // Default sample rate
	config.buffer_size = 1200;   // Default buffer size

	config.capture_format = ma_format_s32; // 32-bit signed integer format
	config.capture_channels = 2;           // Stereo
	config.capture_device_id = NULL;       // Use default capture device

	config.playback_format = ma_format_s32; // 32-bit signed integer format
	config.playback_channels = 2;           // Stereo
	config.playback_device_id = NULL;       // Use default playback device

	return config;
}

int _init_device(AudioConfig *config) {
	ma_device_config deviceConfig = ma_device_config_init(ma_device_type_duplex);

	deviceConfig.capture.pDeviceID = config->capture_device_id; // Use the selected capture device
	deviceConfig.capture.format = config->capture_format;
	deviceConfig.capture.channels = config->capture_channels;
	deviceConfig.capture.shareMode = ma_share_mode_shared;
	deviceConfig.playback.pDeviceID = config->playback_device_id; // Use the selected playback device
	deviceConfig.playback.format = config->playback_format;
	deviceConfig.playback.channels = config->playback_channels;
	deviceConfig.sampleRate = config->sample_rate; // Set the sample rate
	deviceConfig.pUserData = g_audio_data; // Pass the audio data to the callback
	deviceConfig.dataCallback = ma_callback;
	ma_result result;
	result = ma_device_init(NULL, &deviceConfig, &g_audio_device);
	if (result != MA_SUCCESS) {
		printf("Failed to initialize audio device: %s\n", ma_result_description(result));
		return result;
	}
	ma_device_start(&g_audio_device);
	return 0;
}

int init_audio_context() {
	// Initialize the audio context
	if (ma_context_init(NULL, 0, NULL, &g_audio_context) != MA_SUCCESS) {
		printf("Failed to initialize audio context\n");
		return -1;
	}
	return 0;
}

AudioDevicesInfo get_audio_devices_info() {
	AudioDevicesInfo devices_info;

	ma_result result = ma_context_get_devices(
		&g_audio_context,
		&devices_info.playback_devices,
		&devices_info.playback_device_count,
		&devices_info.capture_devices,
		&devices_info.capture_device_count
	);

	if (result != MA_SUCCESS) {
		printf("Failed to get audio devices\n");
		printf("Error: %s\n", ma_result_description(result));
		exit(-1);
	}

	return devices_info;
}


int init_audio(AudioConfig *config) {

	if (config == NULL) {
		printf("Audio configuration is NULL\n");
		return -1;
	}

	g_audio_data = _init_audio_data(config);
	if (g_audio_data == NULL) {
		printf("Failed to initialize audio data\n");
		return -1;
	}

	// if (_init_device(g_audio_data, &pCaptureInfos[*capture_device_index].id, &pPlaybackInfos[*playback_device_index].id) != 0) {
	if (_init_device(config) != 0) {
		printf("Failed to initialize audio device\n");
		abort();
	}


	_is_audio_initialized = 1; // Set the flag to indicate that audio is initialized

	return 0;
}

void close_audio() {
	if (g_audio_data == NULL) {
		printf("Audio data is not initialized\n");
		return;
	}
	ma_device_stop(&g_audio_device);
	ma_device_uninit(&g_audio_device);
	ma_pcm_rb_uninit(&g_audio_data->rb);
	free(g_audio_data);
	g_audio_data = NULL;

	printf("Audio closed successfully\n");
}

#endif // AUDIO_H
