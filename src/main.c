//----------------------------------------------------------------------------------
// Audio Visualizer
//----------------------------------------------------------------------------------

#define SAMPLE_TYPE ma_int32

#include "application.h"
#include "audio.h"
#include "audio_analysis.h"
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "gui_audio_config.h"
#include "style_dark.h"


static const Color background = CLITERAL(Color){0x00, 0x00, 0x00, 0xff};
static const Color foreground = CLITERAL(Color){0xff, 0xff, 0xff, 0xff};

void render_audio_analysis(AudioAnalysis *analysis) {
	// This function can be used to render the audio analysis results
	int rw = GetRenderWidth();
	int rh = GetRenderHeight();
	int fcount = analysis->buffer.size;
	int accumulation = 48; // How many frames to accumulate for visualization
	int bin_count = floor(fcount / (float)accumulation);

	double *time_data =
		analysis->time_data[0]; // Use the first channel for visualization
	double *freq_data =
		analysis->freq_data[0]; // Use the first channel for frequency bins
	//
	//acumulate freq_data in
	double *freq_bins = (double *)malloc(sizeof(double) * bin_count);
	for (int i = 0; i < bin_count; i++) {
		for (int j = 0; j < accumulation; j++) {
			freq_bins[i] += freq_data[(i * 4 + j)%fcount];
		}
		freq_bins[i] = freq_bins[i]/accumulation;
	}

	float w = ceil(((float)rw) / (float)fcount);


	// printf("rw: %d, rh: %d, fcount: %d, bin_count: %d, w: %f\n", rw, rh, fcount, bin_count, w);

	for (int i = 0; i < fcount; i++) {
		float td = (float)time_data[i] / 214748360.0f;
		int td_h = (int)(((float)rh / 2) * td);

		if (td > 0) {
			DrawRectangle(i * w, 2 * rh / 3 - td_h, w, td_h / 5 + 1, foreground);
		} else if (td < 0) {
			td_h = -td_h;
			DrawRectangle(i * w, 2 * rh / 3, w, td_h / 5 + 1, foreground);
		}
	}
	w = ceil((((float)rw) / (float)bin_count));
	// printf("bin_count: %d, rw: %d, rh: %d, h: %f\n", bin_count, rw, rh, w);
	for (int i = 0; i < bin_count; i++) {
		double fd = freq_bins[i] / 1000000.0;
		int fd_h = (int)(((float)rh / 2) * fd);
		if (fd > 0) {
			DrawRectangle(
				i * w, 
				rh / 3 - fd_h,
				w,
				fd_h / 5 + 1,
				CLITERAL(Color){0x00, 0xff, 0x00, 0xff}
			);
		} else if (fd < 0) {
			fd_h = -fd_h;
			DrawRectangle(
				i * w,
				rh / 3,
				w, 
				fd_h / 5 + 1,
				CLITERAL(Color){0x00, 0xff, 0x00, 0xff}
			);
		}
	}
	free(freq_bins);
}


//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char **argv) {

	Application *app = init_application();

	init_audio_context();
	AudioConfig audio_config = init_audio_config();
	init_audio(&audio_config); // Initialize audio with default configuration
	start_analysis(&(AudioAnalysisConfig){
		.buffer_size = audio_config.buffer_size,
		.channels = audio_config.capture_channels
	}); // Start audio analysis

	//--------------------------------------------------------------------------------------
	// Graphics Initialization
	//--------------------------------------------------------------------------------------
	int display = GetCurrentMonitor();
	const int screenWidth = GetMonitorWidth(display);
	const int screenHeight = GetMonitorHeight(display);

	InitWindow(screenWidth, screenHeight, "Vizualizer");
	SetTargetFPS(60); // Set our game to run at 60 frames-per-second
	GuiLoadStyleDark(); // Load dark style for raygui
	ToggleFullscreen();

	Image imBlank = GenImageColor(1024, 1024, BLANK);

	Texture2D texture = LoadTextureFromImage(imBlank); // Load blank texture to fill on shader
	UnloadImage(imBlank);

	// NOTE: Using GLSL 330 shader version
	Shader shader = LoadShader(0, TextFormat("resources/shaders/cubes.fs.glsl"));

	// float time = 0.0f;
	// int timeLoc = GetShaderLocation(shader, "u_time");
	// float signalLoc = GetShaderLocation(shader, "u_signal");
	// SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
	// SetShaderValue(shader, signalLoc, &g_audio_norm_avg[0],
	// SHADER_UNIFORM_FLOAT); SetShaderValue(shader, GetShaderLocation(shader,
	// "u_resolution"), &resolution, SHADER_UNIFORM_VEC2);

	// AudioData *g_audio_data = get_audio_data();


	GuiAudioConfigState state = InitGuiAudioConfig(&audio_config);

	// -------------------------------------------------------------------------------------------------------------
	// Main game loop
	while (!WindowShouldClose()) // Detect window close button or ESC key
	{
	SetExitKey(KEY_NULL);
		if (app->is_running) {
			// Update
			//----------------------------------------------------------------------------------
			// time = (float)GetTime();
			// SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
			// SetShaderValue(shader, signalLoc, &g_audio_norm_avg[0],
			// SHADER_UNIFORM_FLOAT);
			//----------------------------------------------------------------------------------
			// check for alt + enter
			if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))
			{
				// toggle the state
				ToggleFullscreen();
			}
			if (IsKeyPressed(KEY_ESCAPE)) {
				if (app->show_menu) {
					app->show_menu = false;
				} else {
					app->show_menu = true;
				}
			}
			// Draw
			//----------------------------------------------------------------------------------
			BeginDrawing();

			// ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR))); 
			ClearBackground(background);
			render_audio_analysis(g_audio_analysis);

			if(app->show_menu) {
				GuiAudioConfig(&state, &audio_config, app);
			}
			// raygui: controls drawing
			//----------------------------------------------------------------------------------
			// BeginShaderMode(shader);    // Enable our custom shader for next
			// shapes/textures drawings DrawTexture(texture, 0, 0, WHITE);  // Drawing
			// BLANK texture, all magic happens on shader EndShaderMode();            //
			// Disable our custom shader, return to default shader

			EndDrawing();


		} else {
			CloseWindow(); // Close window and OpenGL context

		}

		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	//--------------------------------------------------------------------------------------
	UnloadShader(shader);
	UnloadTexture(texture);

	stop_analysis(); // Stop audio analysis
	close_analysis();
	close_audio(); // Close audio device and free memory
	uinit_application(app); // Free application resources
	//--------------------------------------------------------------------------------------


	return 0;
}
