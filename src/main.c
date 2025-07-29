#include "../vendor/miniaudio/miniaudio.c"
#include "raylib.h"
#include <fftw3.h>

#define PLATFORM_DESKTOP

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION            100
#endif

#define AUDIO_FRAMES 4800
#define AUDIO_INPUT_CHANNELS 2
#define AUDIO_OUTPUT_CHANNELS 2

static const int g_audio_input_format = ma_format_s32; // Using 32-bit signed integer format
static const int g_audio_output_format = ma_format_s32; // Using 32-bit signed integer format

static ma_int32 g_audio_frames[AUDIO_INPUT_CHANNELS][AUDIO_FRAMES];
static float g_audio_norm_avg[AUDIO_INPUT_CHANNELS] = {0.0f, 0.0f}; // Average for each channel
static size_t g_audio_frames_count = 0;
static size_t g_audio_frames_cursor = 0;

static double *g_audio_fft_in;
static double *g_audio_fft_out;
static fftw_plan g_audio_fft_plan;

static float g_audio_fft_results[AUDIO_INPUT_CHANNELS][AUDIO_FRAMES]={{0.0f}}; // FFT results for visualization


/* Black */
static const Color background = CLITERAL(Color){0x00, 0x00, 0x00, 0xff};

/* White */
static const Color foreground = CLITERAL(Color){0xff, 0xff, 0xff, 0xff};

void run_fft(ma_int32 *input, float *output, int n) {

	// Fill input array
	for (int i = 0; i < n; i++) {
		g_audio_fft_in[i] = (float)input[i]; // Real part
	}

	// Execute FFT
	fftw_execute(g_audio_fft_plan);

	// Copy output to output array
	for (int i = 0; i < n; i++) {
		output[i] = fabs((float)g_audio_fft_out[i]); // Real part of the output
		// printf("FFT Output[%d]: %f\n", i, output[i]); // Debug output
	}

	// Cleanup
}
void proc_audio(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
	if (frameCount == 0) return;
	if(pDevice->capture.channels != AUDIO_INPUT_CHANNELS) {
		printf("Error: Expected %d channels, but got %d channels.\n", AUDIO_INPUT_CHANNELS, pDevice->capture.channels);
		return;
	}

	// printf("Received %u frames of audio data.\n", frameCount);

	const ma_int32 *input_samples = (const ma_int32 *)pInput;

	// Handle the copy with potential wrapping
	for (ma_uint32 i = 0; i < AUDIO_INPUT_CHANNELS; i++) {
		float sum = 0.0f;
		for (ma_uint32 j = 0; j < frameCount; j++) {
			// Calculate the index in the circular buffer
			size_t buffer_index = (g_audio_frames_cursor + j) % AUDIO_FRAMES;

			// Copy the sample for this channel
			g_audio_frames[i][buffer_index] = input_samples[i * frameCount + j];

			sum += (float)g_audio_frames[i][buffer_index]/ 2147483648.0f; // Normalize to [-1.0, 1.0] range
		}
		g_audio_norm_avg[i] = sum / (float)frameCount; // Calculate average for this channel
		// Run FFT on the channel data
		run_fft(g_audio_frames[i], g_audio_fft_results[i], AUDIO_FRAMES);

	}

	// Update cursor with wrapping
	g_audio_frames_cursor = (g_audio_frames_cursor + frameCount) % AUDIO_FRAMES;

	// Update frame count
	if (g_audio_frames_count < AUDIO_FRAMES) {
		g_audio_frames_count += frameCount;
		if (g_audio_frames_count > AUDIO_FRAMES) {
			g_audio_frames_count = AUDIO_FRAMES;
		}
	}
	MA_COPY_MEMORY(pOutput, pInput, frameCount * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels));
}
void render_samples(float buf[AUDIO_INPUT_CHANNELS][AUDIO_FRAMES], int fcount) {
	int width = GetRenderWidth();
	int height = GetRenderHeight();

	float w = (((float)width) / (float)fcount);

	for (int i = 0; i < (fcount); i++) {
		float sample = buf[0][i]; // Use the first channel for visualization
		// printf("Sample %d: %f\n", i, sample);

		int h = (int)(((float)height / 2) * (sample/1000000000.0f)); // Scale sample to fit the height

		if (sample > 0) {
			DrawRectangle(i * w, height - h, 1, h / 5 + 1, foreground);
		} else if (sample < 0) {
			h = -h;
			DrawRectangle(i * w, 0, 1, h / 5 + 1,
					  foreground);
		}
	}
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
	//--------------------------------------------------------------------------------------
	// Audio Initialization
	//--------------------------------------------------------------------------------------
	ma_result result;
	ma_device_config deviceConfig;
	ma_device device;
	deviceConfig = ma_device_config_init(ma_device_type_duplex);
	deviceConfig.capture.pDeviceID = NULL;
	deviceConfig.capture.format = g_audio_input_format;
	deviceConfig.capture.channels = AUDIO_INPUT_CHANNELS;
	deviceConfig.capture.shareMode = ma_share_mode_shared;
	deviceConfig.playback.pDeviceID = NULL;
	deviceConfig.playback.format = g_audio_output_format;
	deviceConfig.playback.channels = AUDIO_OUTPUT_CHANNELS;
	deviceConfig.dataCallback = proc_audio;
	result = ma_device_init(NULL, &deviceConfig, &device);
	if (result != MA_SUCCESS) {
		printf("Failed to initialize audio device: %s\n", ma_result_description(result));
		return result;
	}
	ma_device_start(&device);

	g_audio_fft_in = (double*) fftw_malloc(sizeof(double) * AUDIO_FRAMES);
	g_audio_fft_out = (double*) fftw_malloc(sizeof(double) * AUDIO_FRAMES);
	g_audio_fft_plan = fftw_plan_r2r_1d(
		AUDIO_FRAMES,
		g_audio_fft_in,
		g_audio_fft_out,
		FFTW_REDFT10,
		FFTW_ESTIMATE
	);

	//--------------------------------------------------------------------------------------
	// Graphics Initialization
	//--------------------------------------------------------------------------------------
	const int screenWidth = 800;
	const int screenHeight = 800;
	float resolution[2] = { (float)screenWidth, (float)screenHeight };

	InitWindow(screenWidth, screenHeight, "Vizualizer");
	SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
	
	Image imBlank = GenImageColor(1024, 1024, BLANK);
	Texture2D texture = LoadTextureFromImage(imBlank);  // Load blank texture to fill on shader
	UnloadImage(imBlank);

	// NOTE: Using GLSL 330 shader version
	Shader shader = LoadShader(0, TextFormat("resources/shaders/cubes.fs.glsl"));

	float time = 0.0f;
	int timeLoc = GetShaderLocation(shader, "u_time");
	float signalLoc = GetShaderLocation(shader, "u_signal");
	SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
	SetShaderValue(shader, signalLoc, &g_audio_norm_avg[0], SHADER_UNIFORM_FLOAT);
	SetShaderValue(shader, GetShaderLocation(shader, "u_resolution"), &resolution, SHADER_UNIFORM_VEC2);

	// -------------------------------------------------------------------------------------------------------------

	// Main game loop
	while (!WindowShouldClose())    // Detect window close button or ESC key
	{
		// Update
		//----------------------------------------------------------------------------------
		time = (float)GetTime();
		SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
		SetShaderValue(shader, signalLoc, &g_audio_norm_avg[0], SHADER_UNIFORM_FLOAT);
		//----------------------------------------------------------------------------------

		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();

		ClearBackground(background);

		if (g_audio_frames_count != 0) {
			render_samples(g_audio_fft_results, g_audio_frames_count);
		}
		// BeginShaderMode(shader);    // Enable our custom shader for next shapes/textures drawings
		// DrawTexture(texture, 0, 0, WHITE);  // Drawing BLANK texture, all magic happens on shader
		// EndShaderMode();            // Disable our custom shader, return to default shader


		EndDrawing();
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	//--------------------------------------------------------------------------------------
	UnloadShader(shader);
	UnloadTexture(texture);

	fftw_destroy_plan(g_audio_fft_plan);
	fftw_free(g_audio_fft_in);
	fftw_free(g_audio_fft_out);

	CloseWindow();        // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

	return 0;
}
