#ifndef AUDIO_H
#define AUDIO_H
#include <stdio.h>
#define MA_IMPLEMENTATION
#include <fftw3.h>
#include <miniaudio.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#ifndef SAMPLE_TYPE
#define SAMPLE_TYPE ma_float
#endif

typedef struct {
	ma_pcm_rb rb;                 // ring buffer to transport audio data
	ma_decoder* decoder;         // audio decoder for processing the sound file if needed
} AudioData;

typedef enum {
	AUDIO_SOURCE_TYPE_INLINE,		  // inline input
	AUDIO_SOURCE_TYPE_FILE,	  // Audio file input
} AudioSourceType;

typedef struct {
	ma_uint32 sample_rate;
	size_t buffer_size;
	AudioSourceType source_type; // Type of audio source (e.g., inline, file)
	char *file_path; // Path to the audio file if source_type is AUDIO_SOURCE_TYPE_FILE

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

int init_audio_context() {
	// Initialize the audio context
	if (ma_context_init(NULL, 0, NULL, &g_audio_context) != MA_SUCCESS) {
		printf("Failed to initialize audio context\n");
		return -1;
	}
	return 0;
}

SAMPLE_TYPE* read_audio_data(ma_uint32 *sizeInFrames) {
	// This function is intended to run in a separate thread to read audio data from the ring buffer
	AudioData *audio_data = get_audio_data();
	if (audio_data == NULL) {
		printf("Audio data is not initialized\n");
		return NULL;
	}

	void *pBuffer;

	void *raw_data;

	ma_result result = ma_pcm_rb_acquire_read(&audio_data->rb, sizeInFrames, &pBuffer);
	if (result != MA_SUCCESS) {
		printf("Failed to acquire read buffer: %s\n", ma_result_description(result));
		return NULL;
	}

	if (sizeInFrames == 0) {
		ma_pcm_rb_commit_read(&audio_data->rb, 0);
		return NULL; // No data to read
	}

	// Calculate the size in bytes to read
	ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(g_audio_device.capture.format, g_audio_device.capture.channels);
	raw_data = malloc(*sizeInFrames * bytesPerFrame);
	if (raw_data == NULL) {
		printf("Failed to allocate memory for audio data\n");
		ma_pcm_rb_commit_read(&audio_data->rb, 0);
		return NULL;
	}
	// Copy the data from the ring buffer to the raw_data buffer
	MA_COPY_MEMORY(raw_data, pBuffer, *sizeInFrames * bytesPerFrame);

	ma_pcm_rb_commit_read(&audio_data->rb, *sizeInFrames);

	return (SAMPLE_TYPE*)raw_data; // Return the pointer to the read buffer

}

void ma_callback_file(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
	if (frameCount == 0) return;

	AudioData *audio_data = (AudioData *)pDevice->pUserData;

	void *pBuffer;
	void *pOutputBuffer = malloc(frameCount * ma_get_bytes_per_frame(audio_data->rb.format, audio_data->rb.channels));
	ma_uint32 availableFrames = frameCount;
	ma_result result = ma_pcm_rb_acquire_write(&audio_data->rb, &availableFrames, &pBuffer);
	if (result != MA_SUCCESS) {
		printf("Failed to acquire write buffer: %s\n", ma_result_description(result));
		return;
	}
	ma_uint64 framesRead = 0;
	// Read PCM frames from the decoder
	result = ma_decoder_read_pcm_frames(audio_data->decoder, pOutputBuffer, frameCount, &framesRead);
	if (result != MA_SUCCESS) {
		printf("Failed to read PCM frames: %s\n", ma_result_description(result));
		ma_pcm_rb_commit_write(&audio_data->rb, 0); // Commit with 0 frames if read fails
		return;
	}

	ma_copy_pcm_frames(pBuffer, pOutputBuffer, availableFrames>framesRead?framesRead:availableFrames, audio_data->rb.format, audio_data->rb.channels);

	result = ma_pcm_rb_commit_write(&audio_data->rb, availableFrames>framesRead?framesRead:availableFrames);
	if (result != MA_SUCCESS) {
		printf("Failed to commit write buffer: %s\n", ma_result_description(result));
		return;
	}
	// Copy the output data to the output buffer
	// ma_copy_and_apply_volume_factor_pcm_frames(void *pFramesOut, const void *pFramesIn, ma_uint64 frameCount, ma_format format, ma_uint32 channels, float factor)
	ma_copy_pcm_frames(pOutput, pOutputBuffer, framesRead, audio_data->rb.format, audio_data->rb.channels);
	free(pOutputBuffer);
	// MA_COPY_MEMORY(pOutput, pBuffer, frameCount * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels));
}

void ma_callback_inline(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
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
	ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels);
	ma_uint32 bytesToWrite = sizeInFrames * bytesPerFrame;
	MA_COPY_MEMORY(pBuffer, pInput, bytesToWrite);
	MA_COPY_MEMORY(pOutput, pInput, frameCount * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels));
	result = ma_pcm_rb_commit_write(&audio_data->rb, sizeInFrames);
	if (result != MA_SUCCESS) {
		printf("Failed to commit write buffer: %s\n", ma_result_description(result));
		return;
	}

}

ma_decoder_config g_decoder_config;

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

	if (config->source_type == AUDIO_SOURCE_TYPE_FILE) {
		if (config->file_path == NULL) {
			printf("File path is NULL for AUDIO_SOURCE_TYPE_FILE\n");
			return NULL;
		}
		// g_decoder_config = ma_decoder_config_init(config->capture_format, config->sample_rate, config->capture_channels);
		// g_decoder_config.channels=2;
		// g_decoder_config.sampleRate = 0;
		// g_decoder_config.format = 0;
		// g_decoder_config.encodingFormat = ma_encoding_format_mp3;
		// ma_decoder_config decoder_config = ma_decoder_config_init();
		audio_data->decoder = (ma_decoder *)malloc(sizeof(ma_decoder));
		ma_decoder_init_file(config->file_path,NULL, audio_data->decoder);
		if (audio_data->decoder == NULL) {
			printf("Failed to initialize audio decoder for file: %s\n", config->file_path);
			return NULL;
		}
		config->sample_rate = audio_data->decoder->outputSampleRate; // Set the sample rate from the decoder
		config->playback_format = audio_data->decoder->outputFormat; // Set the playback format from the decoder
		config->playback_channels = audio_data->decoder->outputChannels; // Set the playback channels from the decoder
		config->capture_format = audio_data->decoder->outputFormat; // Set the capture format from the decoder
		config->capture_channels = audio_data->decoder->outputChannels; // Set the capture channels from the decoder

	} else {
		audio_data->decoder = NULL; // No decoder for inline input
	}

	return audio_data;

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


AudioConfig init_audio_config() {
	AudioConfig config;
	config.sample_rate = 48000; // Default sample rate
	config.buffer_size = 1200;   // Default buffer size
	config.source_type = AUDIO_SOURCE_TYPE_INLINE; // Default source type is inline

	config.capture_format = ma_format_f32;
	config.capture_channels = 2;           // Stereo
	config.capture_device_id = NULL;       // Use default capture device

	config.playback_format = ma_format_f32;
	config.playback_channels = 2;           // Stereo
	config.playback_device_id = NULL;       // Use default playback device
	AudioDevicesInfo devices_info = get_audio_devices_info();
	if (devices_info.capture_device_count > 0) {
		for (ma_uint32 i = 0; i < devices_info.capture_device_count; i++) {
			if (devices_info.capture_devices[i].isDefault) {
				printf("Using default capture device: %s\n", devices_info.capture_devices[i].name);
				config.capture_device_id = &devices_info.capture_devices[i].id; // Use the first default capture device
				break;
			}
		}
	}
	if (devices_info.playback_device_count > 0) {
		for (ma_uint32 i = 0; i < devices_info.playback_device_count; i++) {
			if (devices_info.playback_devices[i].isDefault) {
				printf("Using default playback device: %s\n", devices_info.playback_devices[i].name);
				config.playback_device_id = &devices_info.playback_devices[i].id; // Use the first default playback device
				break;
			}
		}
	}
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
	// deviceConfig.sampleRate = config->sample_rate; // Set the sample rate
	deviceConfig.pUserData = g_audio_data; // Pass the audio data to the callback
	if (config->source_type == AUDIO_SOURCE_TYPE_FILE) {
		deviceConfig.dataCallback = ma_callback_file; // Use the file callback for file input
	} else {
		deviceConfig.dataCallback = ma_callback_inline; // Use the inline callback for inline input
	}
	ma_result result;
	result = ma_device_init(&g_audio_context, &deviceConfig, &g_audio_device);
	if (result != MA_SUCCESS) {
		printf("Failed to initialize audio device: %s\n", ma_result_description(result));
		return result;
	}
	ma_device_start(&g_audio_device);

	printf("Audio device initialized successfully\n");
	return 0;
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
	printf("Audio initialized successfully\n");

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
	if (g_audio_data->decoder != NULL) {
		ma_decoder_uninit(g_audio_data->decoder);
		free(g_audio_data->decoder);
		g_audio_data->decoder = NULL;
	}
	free(g_audio_data);
	g_audio_data = NULL;
	_is_audio_initialized = 0; // Reset the flag to indicate that audio is closed

	printf("Audio closed successfully\n");
}

#endif // AUDIO_H
