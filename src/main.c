// #include "../vendor/miniaudio/miniaudio.c"
#include "raylib.h"
#include "./audio.c"
// #include <fftw3.h>

#define AUDIO_FRAMES 4800
#define AUDIO_INPUT_CHANNELS 2
#define AUDIO_OUTPUT_CHANNELS 2

static const int g_audio_input_format = ma_format_s32; // Using 32-bit signed integer format
static const int g_audio_output_format = ma_format_s32; // Using 32-bit signed integer format

/* Black */
static const Color background = CLITERAL(Color){0x00, 0x00, 0x00, 0xff};

/* White */
static const Color foreground = CLITERAL(Color){0xff, 0xff, 0xff, 0xff};

void render_samples(ma_int32 *buf, int fcount) {
	int width = GetRenderWidth();
	int height = GetRenderHeight();

	float w = (((float)width) / (float)fcount);

	for (int i = 0; i < (fcount); i++) {
		float sample = (float)buf[i]/1000000000.0f; // Use the first channel for visualization
		// printf("Sample %d: %f\n", i, sample);
		int h = (int)(((float)height / 2) * (sample)); // Scale sample to fit the height

		if (sample > 0) {
			DrawRectangle(i * w, height/2 - h, 1, h / 5 + 1, foreground);
		} else if (sample < 0) {
			h = -h;
			DrawRectangle(i * w, height/2, 1, h / 5 + 1,
					  foreground);
		}
	}
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
	
	if(init_audio(
		AUDIO_FRAMES,
		g_audio_input_format,
		AUDIO_INPUT_CHANNELS,
		g_audio_output_format,
		AUDIO_OUTPUT_CHANNELS
	) != 0) {
		printf("Failed to initialize audio\n");
		return -1;
	} else {
		printf("Audio initialized successfully\n");
	}

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

	// float time = 0.0f;
	// int timeLoc = GetShaderLocation(shader, "u_time");
	// float signalLoc = GetShaderLocation(shader, "u_signal");
	// SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
	// SetShaderValue(shader, signalLoc, &g_audio_norm_avg[0], SHADER_UNIFORM_FLOAT);
	// SetShaderValue(shader, GetShaderLocation(shader, "u_resolution"), &resolution, SHADER_UNIFORM_VEC2);

	AudioData *g_audio_data = get_audio_data();

	// -------------------------------------------------------------------------------------------------------------

	// Main game loop
	while (!WindowShouldClose())    // Detect window close button or ESC key
	{
		// Update
		//----------------------------------------------------------------------------------
		// time = (float)GetTime();
		// SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
		 // SetShaderValue(shader, signalLoc, &g_audio_norm_avg[0], SHADER_UNIFORM_FLOAT);
		//----------------------------------------------------------------------------------

		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();

		ClearBackground(background);

		if (g_audio_data->buffer.frames_count != 0) {
			// const double *cutted_freq_bins = g_audio_data->analysis_results.freq_bins[0]+2048;
			render_samples( g_audio_data->buffer.frames[0], AUDIO_FRAMES);
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
	//
	// fftw_destroy_plan(g_audio_fft_plan);
	// fftw_free(g_audio_fft_in);
	// fftw_free(g_audio_fft_out);
	//
	close_audio(); // Close audio device and free memory

	CloseWindow();        // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

	return 0;
}
