////----------------------------------------------------------------------------------
//// Audio Visualizer
////----------------------------------------------------------------------------------
#include <math.h>
#include <stdio.h>
#define GLSL_VERSION 330
#define PLATFORM_DESKTOP
#define SAMPLE_TYPE ma_float

#include "application.h"
#include "audio.h"
#include "audio_analysis.h"
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "gui_audio_config.h"
#include "rlgl.h"
#include "style_dark.h"

static const Color background = CLITERAL(Color){0x00, 0x00, 0x00, 0xff};
static const Color foreground = CLITERAL(Color){0xff, 0xff, 0xff, 0xff};

void render_analysis_time_data(AudioAnalysis *analysis) {
  // This function can be used to render the time domain data
  int rw = GetRenderWidth();
  int rh = GetRenderHeight();
  int fcount = analysis->buffer.size;
  float w = ceil(((float)rw) / (float)fcount);

  double *time_data =
		analysis->time_data[0]; // Use the first channel for visualization

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
}
void render_analysis_freq_data(AudioAnalysis *analysis) {
	// This function can be used to render the frequency domain data
	int rw = GetRenderWidth();
	int rh = GetRenderHeight();
	int fcount = 125;

	double *pitch = analysis->pitch[0]; // Use the first channel for visualization

	//vamos agrupar as frequencias em bins logaritmicos
	// int log_fcount = ceil(log2(fcount));
	// int num_bins = 100;
	float w = ceil(((float)rw) / (float)fcount);
	// double *pitch = calloc(num_bins, sizeof(double));

	// Calculate the pitch for each frequency bin
	// for (int i = 0; i < num_bins; i++) {
	// 	int bin_start = floor(pow(2, i*(log_fcount/(float)num_bins)) - 1);
	// 	//quando chegar no ultimo bin, garantir que bin_end=fcount
	// 	int bin_end = ceil(pow(2, (i + 1)*(log_fcount/(float)num_bins)));
	// 	if (bin_end > fcount) {
	// 		bin_end = fcount;
	// 	}
	//
	// 	double sum = 0.0;
	// 	for (int j = bin_start; j < bin_end; j++) {
	// 		// printf("j: %d, freq_data[j]: %f\n", j, freq_data[j]);
	// 		sum += freq_data[j];
	// 	}
	// 	pitch[i] = sum / (bin_end - bin_start);
	// 	// printf("bin %d: start: %d, end: %d, pitch: %f\n", i, bin_start, bin_end, pitch[i]);
	// }

	// Render the frequency data as bars

	for (int i = 0; i < fcount; i++) {
		double fd = pitch[i]*50;
		int fd_h = (int)(((float)rh / 2) * fd);
		if (fd > 0) {
			DrawRectangle(i * w, rh - (fd_h / 5 + 1), w, fd_h / 5 + 1,
					  CLITERAL(Color){0x00, 0xff, 0x00, 0xff});
		} else if (fd < 0) {
			fd_h = -fd_h;
			DrawRectangle(i * w, rh - (fd_h / 5 + 1), w, fd_h / 5 + 1,
					  CLITERAL(Color){0x00, 0xff, 0x00, 0xff});
		}
		//render a line with ticks for the frequency data
		if (i % 1 == 0) {
			DrawLine(i * w, rh - (fd_h / 5 + 1), i * w, rh, CLITERAL(Color){0x00, 0xff, 0x00, 0xff});
			DrawText(TextFormat("%d", i), i * w + 2, rh - (fd_h / 5 + 1) - 10, 10, CLITERAL(Color){0x00, 0xff, 0x00, 0xff});
		}
	}
}

void render_audio_analysis(AudioAnalysis *analysis) {
  // This function can be used to render the audio analysis results
  int rw = GetRenderWidth();
  int rh = GetRenderHeight();
  int fcount = analysis->buffer.size;
  int accumulation = 1; // How many frames to accumulate for visualization
  int bin_count = floor(fcount / (float)accumulation);

  double *time_data =
      analysis->time_data[0]; // Use the first channel for visualization
  double *freq_data =
      analysis->freq_data[0]; // Use the first channel for frequency bins
  //
  // acumulate freq_data in
  double *freq_bins = (double *)calloc(bin_count, sizeof(double));
  for (int i = 0; i < bin_count; i++) {
    for (int j = 0; j < accumulation; j++) {
		if ((i * accumulation + j) >= fcount) {
		  break; // Prevent out of bounds access
		}
      freq_bins[i] += freq_data[(i * accumulation + j) % fcount];
    }
    freq_bins[i] = freq_bins[i] / accumulation;
  }

  float w = ceil(((float)rw) / (float)fcount);

  // printf("rw: %d, rh: %d, fcount: %d, bin_count: %d, w: %f\n", rw, rh,
  // fcount, bin_count, w);

  // for (int i = 0; i < fcount; i++) {
  //   float td = (float)time_data[i];
  //   int td_h = (int)(((float)rh / 2) * td);
  //
  //   if (td > 0) {
  //     DrawRectangle(i * w, rh - td_h, w, td_h / 5 + 1, foreground);
  //   } else if (td < 0) {
  //     td_h = -td_h;
  //     DrawRectangle(i * w, rh, w, td_h / 5 + 1, foreground);
  //   }
  // }
  w = ceil((((float)rw) / (float)bin_count));
  // printf("bin_count: %d, rw: %d, rh: %d, h: %f\n", bin_count, rw, rh, w);
  for (int i = 0; i < bin_count; i++) {
    double fd = freq_bins[i];
    int fd_h = (int)(((float)rh / 2) * fd);
    if (fd > 0) {
      DrawRectangle(i * w, rh - fd_h, w, fd_h / 5 + 1,
                    CLITERAL(Color){0x00, 0xff, 0x00, 0xff});
    } else if (fd < 0) {
      fd_h = -fd_h;
      DrawRectangle(i * w, rh - fd_h, w, fd_h / 5 + 1,
                    CLITERAL(Color){0x00, 0xff, 0x00, 0xff});
    }
  }
  free(freq_bins);
}

void parse_args(int argc, char **argv, AudioConfig *audio_config, Application *app) {

  if (argc > 1) {
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
      audio_config->source_type = AUDIO_SOURCE_TYPE_FILE;
      audio_config->file_path = argv[2];
      printf("Using audio file: %s\n", audio_config->file_path);
    } else if (strcmp(argv[1], "--fullscreen") == 0 || strcmp(argv[1], "-f") == 0) {
      if (argv[2] != NULL &&
          (strcmp(argv[2], "true") == 0 || strcmp(argv[2], "1") == 0)) {
        app->fullscreen = true;
        printf("Fullscreen mode enabled.\n");
      } else if (argv[2] != NULL &&
                 (strcmp(argv[2], "false") == 0 || strcmp(argv[2], "0") == 0)) {
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
}
void GenerateExampleMonoAudio(double *monoData, int numSamples) {
  float frequency1 = 440.0f;  // A4 note for left channel
  float sampleRate = 44100.0f;

  for (int i = 0; i < numSamples; i++) {
    float t = (float)i / sampleRate;
    monoData[i] = 0.5f * sinf(2.0f * PI * frequency1 * t);

  }
}
Texture2D CreateWaveformTexture(double *monoData, int numSamples) {

	float *textureData = (float *)calloc(numSamples, sizeof(float));
	for (int i = 0; i < numSamples; i++) {
		textureData[i] = (float)monoData[i];
	}

	unsigned int textureId = rlLoadTexture(monoData, numSamples, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32, 1);
	Texture2D waveformTexture = {0};
	waveformTexture.id = textureId;
	waveformTexture.width = numSamples;
	waveformTexture.height = 1;
	waveformTexture.mipmaps = 1;
	waveformTexture.format = RL_PIXELFORMAT_UNCOMPRESSED_R32;

	free(textureData);
	return waveformTexture;
}
void UpdateWaveformTexture(Texture2D *texture, double *monoData, int numSamples) {
	if (texture->id == 0 || texture->width != numSamples) {
		UnloadTexture(*texture);
		*texture = CreateWaveformTexture(monoData, numSamples);
	} else {
		float *textureData = (float *)calloc(numSamples, sizeof(float));
		for (int i = 0; i < numSamples; i++) {
			textureData[i] = (float)monoData[i];
		}
		rlUpdateTexture(texture->id,0,0,numSamples,1,RL_PIXELFORMAT_UNCOMPRESSED_R32, textureData);
	}
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char **argv) {

	Application *app = init_application();

	InitWindow(0, 0, "Vizualizer");

	int display = GetCurrentMonitor();
	int screenWidth = GetMonitorWidth(display);
	int screenHeight = GetMonitorHeight(display);
	SetWindowSize(screenWidth, screenHeight);
	SetTargetFPS(60); 
	GuiLoadStyleDark();
	ToggleFullscreen();
	screenWidth = GetMonitorWidth(display);
	screenHeight = GetMonitorHeight(display);
	float resolution[2] = {(float)screenWidth,(float) screenHeight};

	printf("Screen resolution: %dx%d\n", screenWidth, screenHeight);


	init_audio_context();
	AudioConfig audio_config = init_audio_config();

	parse_args(argc, argv, &audio_config, app);

	audio_config.buffer_size = screenWidth*2;

	init_audio(&audio_config);
	start_analysis(&(AudioAnalysisConfig){
		.buffer_size = audio_config.buffer_size,
		.channels = audio_config.capture_channels});

	//--------------------------------------------------------------------------------------
	// Graphics Initialization
	//--------------------------------------------------------------------------------------



	Image imBlank = GenImageColor(screenWidth, screenHeight, BLANK);

	Texture2D texture = LoadTextureFromImage(imBlank);
	UnloadImage(imBlank);

	Texture audio_channel_0 	= CreateWaveformTexture( g_audio_analysis->time_data[0], g_audio_analysis->buffer.size);
	Texture audio_channel_1 	= CreateWaveformTexture( g_audio_analysis->time_data[1], g_audio_analysis->buffer.size);
	Texture spectrum_channel_0 = CreateWaveformTexture( g_audio_analysis->pitch[0], 125);
	Texture spectrum_channel_1 = CreateWaveformTexture( g_audio_analysis->pitch[1], 125);
	Shader shader = LoadShader(0, "resources/shaders/ray.fs.glsl");

	float time = 0.0f;
	int timeLoc = GetShaderLocation(shader, "u_time");
	int signalLoc = GetShaderLocation(shader, "u_signal");
	int resolutionLoc = GetShaderLocation(shader, "u_resolution");
	int audio_channel_0_loc = GetShaderLocation(shader, "u_audio_channel_0");
	int audio_channel_1_loc = GetShaderLocation(shader, "u_audio_channel_1");
	int spectrum_channel_0_loc = GetShaderLocation(shader, "u_spectrum_channel_0");
	int spectrum_channel_1_loc = GetShaderLocation(shader, "u_spectrum_channel_1");
	SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
	SetShaderValue(shader, signalLoc, &g_audio_analysis->norm_avg[0], SHADER_UNIFORM_FLOAT);
	SetShaderValue(shader, resolutionLoc, &resolution, SHADER_UNIFORM_VEC2);
	SetShaderValueTexture(shader, audio_channel_0_loc, audio_channel_0);
	SetShaderValueTexture(shader, audio_channel_1_loc, audio_channel_1);
	SetShaderValueTexture(shader, spectrum_channel_0_loc, spectrum_channel_0);
	SetShaderValueTexture(shader, spectrum_channel_1_loc, spectrum_channel_1);

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
			float dt = GetFrameTime();
			//----------------------------------------------------------------------------------
			// check for alt + enter
			if (IsKeyPressed(KEY_ENTER) &&
				(IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))) {
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
			UpdateWaveformTexture(&audio_channel_0, g_audio_analysis->time_data[0], g_audio_analysis->buffer.size);
			UpdateWaveformTexture(&audio_channel_1, g_audio_analysis->time_data[1], g_audio_analysis->buffer.size);
			UpdateWaveformTexture(&spectrum_channel_0, g_audio_analysis->pitch[0], 125);
			UpdateWaveformTexture(&spectrum_channel_1, g_audio_analysis->pitch[1], 125);
			// Draw
			//----------------------------------------------------------------------------------
			BeginDrawing();

				// ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
				ClearBackground(BLACK);
				// render_audio_analysis(g_audio_analysis);
				// render_analysis_time_data(g_audio_analysis);
				render_analysis_freq_data(g_audio_analysis);

				// raygui: controls drawing
				//----------------------------------------------------------------------------------
				// BeginShaderMode(shader);
				// 	SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
				// 	SetShaderValue(shader, signalLoc, &g_audio_analysis->norm_avg[0], SHADER_UNIFORM_FLOAT);
				// 	SetShaderValue(shader, resolutionLoc, &resolution, SHADER_UNIFORM_VEC2);
				// 	SetShaderValueTexture(shader, audio_channel_0_loc, audio_channel_0);
				// 	SetShaderValueTexture(shader, audio_channel_1_loc, audio_channel_1);
				// 	SetShaderValueTexture(shader, spectrum_channel_0_loc, spectrum_channel_0);
				// 	SetShaderValueTexture(shader, spectrum_channel_1_loc, spectrum_channel_1);
				// 	DrawTextureRec(
				// 		texture, (Rectangle){0, 0, screenWidth, -screenHeight},
				// 		(Vector2){
				// 			0,
				// 		},
				// 		WHITE);
				// EndShaderMode(); //
				if (app->show_menu) {
					GuiAudioConfig(&state, &audio_config, app);
				}
			EndDrawing();

		} else {
			CloseWindow();
		}
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	//--------------------------------------------------------------------------------------
	UnloadShader(shader);
	UnloadTexture(texture);
	UnloadTexture(audio_channel_0);
	UnloadTexture(audio_channel_1);
	UnloadTexture(spectrum_channel_0);
	UnloadTexture(spectrum_channel_1);

	stop_analysis();
	close_analysis();
	close_audio();
	uinit_application(app);
	//--------------------------------------------------------------------------------------

	return 0;
}

int main_bak(void) {
	// Initialization
	//--------------------------------------------------------------------------------------
	const int screenWidth = 800;
	const int screenHeight = 450;

	InitWindow(screenWidth, screenHeight,
				"raylib [shaders] example - texture drawing");

	Image imBlank = GenImageColor(screenWidth, screenHeight, BLANK);
	Texture2D texture = LoadTextureFromImage(imBlank); // Load blank texture to fill on shader
	UnloadImage(imBlank);

	// NOTE: Using GLSL 330 shader version, on OpenGL ES 2.0 use GLSL 100 shader
	// version
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

	SetTargetFPS(60); // Set our game to run at 60 frames-per-second
	// -------------------------------------------------------------------------------------------------------------

	// Main game loop
	while (!WindowShouldClose()) // Detect window close button or ESC key
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

			BeginShaderMode(
				shader); // Enable our custom shader for next shapes/textures drawings
				DrawTextureRec(
					texture,
					(Rectangle){
						0,
						0,
						screenWidth,
						-screenHeight,
					},
					(Vector2){ 0 },
					WHITE
				); // Drawing BLANK texture, all magic happens on shader
			EndShaderMode(); // Disable our custom shader, return to default shader

			DrawText("BACKGROUND is PAINTED and ANIMATED on SHADER!", 10, 10, 20,
				MAROON);

		EndDrawing();
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	//--------------------------------------------------------------------------------------
	UnloadShader(shader);
	UnloadTexture(texture);

	CloseWindow(); // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

	return 0;
}
// #define MA_IMPLEMENTATION
// #include "miniaudio.h"
//
// #include <stdio.h>
//
// void data_callback(ma_device* pDevice, void* pOutput, const void* pInput,
// ma_uint32 frameCount)
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
