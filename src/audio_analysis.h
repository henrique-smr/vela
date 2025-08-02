#ifndef AUDIO_ANALYSIS_H
#define AUDIO_ANALYSIS_H
#include <fftw3.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include "audio.h"

#ifndef SAMPLE_TYPE
#define SAMPLE_TYPE ma_int32
#endif

typedef struct {
	size_t buffer_size; // Size of the buffer for FFT
	size_t channels;   // Number of channels
} AudioAnalysisConfig;

typedef struct {
	size_t size;
	size_t channels;
	size_t frames_count;
	size_t frames_cursor;
	SAMPLE_TYPE **frames;
} AudioBuffer;


typedef struct {
	double *fft_in;     // Input for FFT
	double *fft_out;    // Output for FFT
	fftw_plan fft_plan; // FFT plan
	AudioBuffer buffer; // Buffer to hold audio data for analysis, will be larger
	double **freq_data; // Frequency domain data for each channel
	double **time_data; // Time domain data for each channel
	float *norm_avg;    // Average normalized value for each channel
} AudioAnalysis;

static AudioAnalysis *g_audio_analysis;

static int _is_analysis_running = 0; // Flag to indicate if the FFT thread is running

static pthread_t fft_thread;

int is_audio_analysis_running() {
	return _is_analysis_running;
}

AudioAnalysisConfig init_audio_analysis_config() {
	AudioAnalysisConfig config;
	config.buffer_size = 1200; // Default buffer size for FFT
	config.channels = 2;        // Default number of channels
	return config;
}

void *fft_loop(void *arg) {

	// AudioAnalysisConfig *config = (AudioAnalysisConfig *)arg;

	printf("FFT thread started\n");
	// This function is intended to run in a separate thread to process the audio
	// data and perform FFT analysis on the captured audio. It will continuously
	// read from the ring buffer and perform FFT on the data.
	while (_is_analysis_running) {
		

		// Acquire read access to the ring buffer

		AudioBuffer *buffer = &g_audio_analysis->buffer;

		// The amount of frames to read. It might be less than the buffer size if not enough data is available. This will be set by the read_audio_data function.
		ma_uint32 sizeInFrames = buffer->size; 

		SAMPLE_TYPE *raw_data = read_audio_data(&sizeInFrames);


		if(raw_data == NULL || sizeInFrames == 0) {
			// No data to process, sleep for a while and continue
			usleep(1000); // Sleep for 1 ms
			continue;
		}

		// fill the buffer with the acquired frames
		for (ma_uint32 i = 0; i < buffer->channels; i++) {
			ma_uint32 buffer_frames_cursor;
			for (ma_uint32 j = 0; j < sizeInFrames; j++) {

				ma_uint32 frame_index = j * buffer->channels + i;

				buffer_frames_cursor = (j + buffer->frames_cursor) % buffer->size;

				buffer->frames[i][buffer_frames_cursor] = raw_data[frame_index]; // Copy data to the buffer
				// Update the frames count
				if (buffer->frames_count < buffer->size) {
					buffer->frames_count++;
				}
				// Update the cursor for the next frame
			}
		}
		buffer->frames_cursor = (buffer->frames_cursor + sizeInFrames) % buffer->size; // Update the cursor for the next read

		if (buffer->frames_count >= buffer->size) {
			for (ma_uint32 i = 0; i < buffer->channels; i++) {
				for (ma_uint32 j = 0; j < buffer->size; j++) {
					g_audio_analysis->fft_in[j] = (double)buffer->frames[i][j]; // Fill FFT input with the buffer data
					g_audio_analysis->time_data[i][j] = (double)buffer->frames[i][j]; // Store time domain data
				}
				fftw_execute(g_audio_analysis->fft_plan); // Execute FFT for this channel

				for (ma_uint32 j = 0; j < buffer->size; j++) {
					double ssample = fabs(g_audio_analysis->fft_out[j]) / buffer->size; // Normalize the FFT output
					if (j == 0) {
						g_audio_analysis->freq_data[i][j] = 0; // Store FFT output
					} else {
						g_audio_analysis->freq_data[i][j] = ssample; // remove pink noise
					}
				}
				float sum = 0.0f;
				for (ma_uint32 j = 0; j < buffer->size; j++) {
					sum += g_audio_analysis->fft_in[j];
				}
				g_audio_analysis->norm_avg[i] = sum / buffer->size; // Calculate average for this channel
			}
		}
	}
	// After processing, we can stop the FFT thread
	return NULL;
}

int start_analysis(AudioAnalysisConfig *config) {

	if(is_audio_initialized() == 0) {
		printf("Audio data is not initialized\n");
		return -1;
	}

	if (_is_analysis_running) {
		printf("FFT thread is already running\n");
		return 0; // FFT thread is already running
	}

	g_audio_analysis = malloc(sizeof(AudioAnalysis));

	g_audio_analysis->norm_avg = calloc(config->channels,sizeof(float));
	g_audio_analysis->freq_data = malloc(sizeof(double *) * config->channels);
	for (size_t i = 0; i < config->channels; i++) {
		g_audio_analysis->freq_data[i] = calloc(config->buffer_size,sizeof(double));
	}
	g_audio_analysis->time_data = malloc(sizeof(double *) * config->channels);
	for (size_t i = 0; i < config->channels; i++) {
		g_audio_analysis->time_data[i] = calloc(config->buffer_size,sizeof(double));
	}
	g_audio_analysis->buffer.size = config->buffer_size;
	g_audio_analysis->buffer.channels = config->channels;
	g_audio_analysis->buffer.frames_count = 0;
	g_audio_analysis->buffer.frames_cursor = 0;
	g_audio_analysis->buffer.frames = malloc(sizeof(ma_int32 *) * config->channels);
	for (size_t i = 0; i < config->channels;i++) {
		g_audio_analysis->buffer.frames[i] = calloc(config->buffer_size,sizeof(ma_int32));
	}

	g_audio_analysis->fft_in = (double *)fftw_malloc(sizeof(double) * config->buffer_size);
	g_audio_analysis->fft_out = (double *)fftw_malloc(sizeof(double) * config->buffer_size);
	g_audio_analysis->fft_plan = fftw_plan_r2r_1d(
		config->buffer_size,
		g_audio_analysis->fft_in,
		g_audio_analysis->fft_out,
		FFTW_REDFT10,
		FFTW_ESTIMATE
	);
	_is_analysis_running = 1; // Set the flag to indicate that the FFT thread should run
	pthread_create(&fft_thread, NULL, fft_loop, config);

	return 0;
}

int stop_analysis() {

	if(g_audio_analysis == NULL) {
		printf("Audio analysis is not initialized\n");
		return -1;
	}

	if (!_is_analysis_running) {
		printf("FFT thread is not running\n");
		return 0; // FFT thread is not running
	}

	_is_analysis_running = 0; // Set the flag to indicate that the FFT thread should stop
	pthread_join(fft_thread, NULL); // Wait for the FFT thread to finish

	return 0;
}

void close_analysis() {

	if(get_audio_data() == NULL) {
		printf("Audio data is not initialized\n");
		return;
	}

	if(g_audio_analysis == NULL || g_audio_analysis == NULL) {
		printf("Audio analysis is not initialized\n");
		return;
	}

	_is_analysis_running = 0; // Stop the FFT thread

	// Free the FFTW resources
	fftw_destroy_plan(g_audio_analysis->fft_plan);
	fftw_cleanup();

	// Free analysis
	for (size_t i = 0; i < g_audio_analysis->buffer.channels; i++) {
		free(g_audio_analysis->buffer.frames[i]);
	}
	free(g_audio_analysis->buffer.frames);

	for (size_t i = 0; i < g_audio_analysis->buffer.channels; i++) {
		free(g_audio_analysis->freq_data[i]);
	}
	free(g_audio_analysis->freq_data);
	for (size_t i = 0; i < g_audio_analysis->buffer.channels; i++) {
		free(g_audio_analysis->time_data[i]);
	}
	free(g_audio_analysis->time_data);

	free(g_audio_analysis->norm_avg);
	free(g_audio_analysis);
	g_audio_analysis = NULL;

}
#endif // AUDIO_ANALYSIS_H
