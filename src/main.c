////----------------------------------------------------------------------------------
//// Audio Visualizer
////----------------------------------------------------------------------------------
#define GLSL_VERSION            330
#define PLATFORM_DESKTOP
#define SAMPLE_TYPE ma_float

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
		float td = (float)time_data[i];
		int td_h = (int)(((float)rh / 2) * td);

		if (td > 0) {
			DrawRectangle(i * w, rh - td_h, w, td_h / 5 + 1, foreground);
		} else if (td < 0) {
			td_h = -td_h;
			DrawRectangle(i * w, rh, w, td_h / 5 + 1, foreground);
		}
	}
	w = ceil((((float)rw) / (float)bin_count));
	// printf("bin_count: %d, rw: %d, rh: %d, h: %f\n", bin_count, rw, rh, w);
	for (int i = 0; i < bin_count; i++) {
		double fd = freq_bins[i];
		int fd_h = (int)(((float)rh / 2) * fd);
		if (fd > 0) {
			DrawRectangle(
				i * w, 
				rh-fd_h,
				w,
				fd_h / 5 + 1,
				CLITERAL(Color){0x00, 0xff, 0x00, 0xff}
			);
		} else if (fd < 0) {
			fd_h = -fd_h;
			DrawRectangle(
				i * w,
				rh-fd_h,
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
	if(argc > 1) {
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
			printf("Usage: %s [options]\n", argv[0]);
			printf("Options:\n");
			printf("  --help, -h       Show this help message\n");
			printf("  --fullscreen, -f Toggle fullscreen mode\n");
			printf("  --file, -f <path> Specify audio file path\n");
			exit(0);
		} else if (strcmp(argv[1], "--file") == 0 || strcmp(argv[1], "-f") == 0) {
			if (argc < 3) {
				fprintf(stderr, "Error: No file path provided.\n");
				exit(1);
			}
			audio_config.source_type = AUDIO_SOURCE_TYPE_FILE;
			audio_config.file_path = argv[2];
			printf("Using audio file: %s\n", audio_config.file_path);
		} else if (strcmp(argv[1], "--fullscreen") == 0 || strcmp(argv[1], "-f") == 0) {
			if(argv[2] != NULL && (strcmp(argv[2], "true") == 0 || strcmp(argv[2], "1") == 0)) {
				app->fullscreen = true;
				printf("Fullscreen mode enabled.\n");
			} else if(argv[2] != NULL && (strcmp(argv[2], "false") == 0 || strcmp(argv[2], "0") == 0)) {
				app->fullscreen = false;
				printf("Fullscreen mode disabled.\n");
			} else if (argc > 2) {
				fprintf(stderr, "Error: Invalid argument for --fullscreen option.\n");
				exit(1);
			}
			app->fullscreen = true;
			printf("Fullscreen mode enabled.\n");
		} else {
			fprintf(stderr, "Error: Unknown option '%s'.\n", argv[1]);
			exit(1);
		}
	}


	init_audio(&audio_config); // Initialize audio with default configuration
	start_analysis(&(AudioAnalysisConfig){
		.buffer_size = audio_config.buffer_size,
		.channels = audio_config.capture_channels
	}); // Start audio analysis

	//--------------------------------------------------------------------------------------
	// Graphics Initialization
	//--------------------------------------------------------------------------------------

	InitWindow(0, 0, "Vizualizer");

	int display = GetCurrentMonitor();
	int screenWidth = GetMonitorWidth(display);
	int screenHeight = GetMonitorHeight(display);
	int resolution[2] = {screenWidth, screenHeight};
	SetWindowSize(screenWidth, screenHeight);
	SetTargetFPS(60); // Set our game to run at 60 frames-per-second
	GuiLoadStyleDark(); // Load dark style for raygui
	ToggleFullscreen();

	Image imBlank = GenImageColor(screenWidth, screenHeight, BLANK);

	Texture2D texture = LoadTextureFromImage(imBlank); // Load blank texture to fill on shader
	UnloadImage(imBlank);

	// NOTE: Using GLSL 330 shader version
	Shader shader = LoadShader(0, "resources/shaders/cubes.fs.glsl");

	float time = 0.0f;
	int timeLoc = GetShaderLocation(shader, "u_time");
	float signalLoc = GetShaderLocation(shader, "u_signal");
	SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
	SetShaderValue(
		shader,
		signalLoc,
		&g_audio_analysis->norm_avg[0],
		SHADER_UNIFORM_FLOAT
	); 
	SetShaderValue(
		shader,
		GetShaderLocation(shader, "u_resolution"),
		&resolution,
		SHADER_UNIFORM_VEC2
	);

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
			time = (float)GetTime();
			SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
			SetShaderValue(shader, signalLoc,&g_audio_analysis->norm_avg[0], SHADER_UNIFORM_FLOAT);
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
			ClearBackground(BLACK); // Clear background with white color
			render_audio_analysis(g_audio_analysis);


			// raygui: controls drawing
			//----------------------------------------------------------------------------------
			// BeginShaderMode(shader);    // Enable our custom shader for next
			// // shapes/textures drawings 
			// 	DrawTextureRec(
			// 		texture,
			// 		(Rectangle) {0,0,screenWidth, -screenHeight},
			// 		(Vector2){0,},
			// 		WHITE
			// 	);  // Drawing BLANK texture, all magic happens on shader
			// EndShaderMode();            //
			// Disable our custom shader, return to default shader
			if(app->show_menu) {
				GuiAudioConfig(&state, &audio_config, app);
			}
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

int main_bak(void)
{
	// Initialization
	//--------------------------------------------------------------------------------------
	const int screenWidth = 800;
	const int screenHeight = 450;

	InitWindow(screenWidth, screenHeight, "raylib [shaders] example - texture drawing");

	Image imBlank = GenImageColor(screenWidth, screenHeight, BLANK);
	Texture2D texture = LoadTextureFromImage(imBlank);  // Load blank texture to fill on shader
	UnloadImage(imBlank);

	// NOTE: Using GLSL 330 shader version, on OpenGL ES 2.0 use GLSL 100 shader version
	Shader shader = LoadShader(0, "resources/shaders/cubes.fs.glsl");

	float time = 0.0f;
	int timeLoc = GetShaderLocation(shader, "u_time");
	SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
	SetShaderValue(
		shader,
		GetShaderLocation(shader, "u_resolution"),
		&(int[2]){screenWidth, screenHeight},
		SHADER_UNIFORM_VEC2
	);

	SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
	// -------------------------------------------------------------------------------------------------------------

	// Main game loop
	while (!WindowShouldClose())    // Detect window close button or ESC key
	{
		// Update
		//----------------------------------------------------------------------------------
		time = (float)GetTime();
		SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
		//----------------------------------------------------------------------------------

		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();

			ClearBackground(RAYWHITE);

			BeginShaderMode(shader);    // Enable our custom shader for next shapes/textures drawings
				DrawTextureRec(
					texture,
					(Rectangle) {0,0,screenWidth, -screenHeight},
					(Vector2){0,},
					WHITE
				);  // Drawing BLANK texture, all magic happens on shader
			EndShaderMode();            // Disable our custom shader, return to default shader

			DrawText("BACKGROUND is PAINTED and ANIMATED on SHADER!", 10, 10, 20, MAROON);

		EndDrawing();
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	//--------------------------------------------------------------------------------------
	UnloadShader(shader);
	UnloadTexture(texture);

	CloseWindow();        // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

	return 0;
}
// #define MA_IMPLEMENTATION
// #include "miniaudio.h"
//
// #include <stdio.h>
//
// void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
// {
//     ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
//     if (pDecoder == NULL) {
//         return;
//     }
//
//     ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);
//
//     (void)pInput;
// }
//
// int main(int argc, char** argv)
// {
//     ma_result result;
//     ma_decoder decoder;
//     ma_device_config deviceConfig;
//     ma_device device;
//
//     if (argc < 2) {
//         printf("No input file.\n");
//         return -1;
//     }
//
//     result = ma_decoder_init_file(argv[1], NULL, &decoder);
//     if (result != MA_SUCCESS) {
//         printf("Could not load file: %s\n", argv[1]);
//         return -2;
//     }
//
//     deviceConfig = ma_device_config_init(ma_device_type_playback);
//     deviceConfig.playback.format   = decoder.outputFormat;
//     deviceConfig.playback.channels = decoder.outputChannels;
//     deviceConfig.sampleRate        = decoder.outputSampleRate;
//     deviceConfig.dataCallback      = data_callback;
//     deviceConfig.pUserData         = &decoder;
//
//     if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
//         printf("Failed to open playback device.\n");
//         ma_decoder_uninit(&decoder);
//         return -3;
//     }
//
//     if (ma_device_start(&device) != MA_SUCCESS) {
//         printf("Failed to start playback device.\n");
//         ma_device_uninit(&device);
//         ma_decoder_uninit(&decoder);
//         return -4;
//     }
//
//     printf("Press Enter to quit...");
//     getchar();
//
//     ma_device_uninit(&device);
//     ma_decoder_uninit(&decoder);
//
//     return 0;
// }
