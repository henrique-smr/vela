#include "../vendor/miniaudio/miniaudio.c"
#include "raylib.h"

#include <stdint.h>
#include <stdio.h>


/* Black */
static const Color background = CLITERAL(Color){0x00, 0x00, 0x00, 0xff};

/* White */
static const Color foreground = CLITERAL(Color){0xff, 0xff, 0xff, 0xff};

// Global buffer to store samples (define at the top of the file)
#define MAX_FRAMES 4800
static ma_int32 g_frames[MAX_FRAMES][MA_MAX_CHANNELS];
static size_t g_frames_count = 0;
static size_t g_frames_cursor = 0;


void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
    if (frameCount == 0) return;

    // printf("Received %u frames of audio data.\n", frameCount);

    const ma_int32 *input_samples = (const ma_int32 *)pInput;
    ma_uint32 channels = pDevice->capture.channels;

    // Handle the copy with potential wrapping
    for (ma_uint32 i = 0; i < frameCount; i++) {
        size_t buffer_index = (g_frames_cursor + i) % MAX_FRAMES;
        
        // Copy all channels for this frame
        memcpy(&g_frames[buffer_index][0], 
               &input_samples[i * channels], 
               channels * sizeof(ma_int32));
    }

    // Update cursor with wrapping
    g_frames_cursor = (g_frames_cursor + frameCount) % MAX_FRAMES;

    // Update frame count
    if (g_frames_count < MAX_FRAMES) {
        g_frames_count += frameCount;
        if (g_frames_count > MAX_FRAMES) {
            g_frames_count = MAX_FRAMES;
        }
    }
}
// void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
// 	if (frameCount == 0) return;
//
//     printf("Received %u frames of audio data.\n", frameCount);
//
//     const ma_int32 *input_samples = (const ma_int32 *)pInput;
//     ma_uint32 channels = pDevice->capture.channels;
//
//     if (g_frames_cursor + frameCount <= MAX_FRAMES) {
//         // Simple case: all frames fit without wrapping
//         memcpy(&g_frames[g_frames_cursor], 
//                input_samples, 
//                frameCount * channels * sizeof(ma_int32));
//         g_frames_cursor += frameCount - 1;        if (g_frames_cursor >= MAX_FRAMES) {
//             g_frames_cursor = 0;
//         }
//     } else {
//         // Buffer wrap case: copy in two parts
//         ma_uint32 frames_until_end = MAX_FRAMES - g_frames_cursor;
//         // Copy frames until the end of the buffer
//         memcpy(&g_frames[g_frames_cursor], 
//                input_samples, 
//                frames_until_end * channels * sizeof(ma_int32));
//         // Copy remaining frames to the beginning of the buffer
//         ma_uint32 remaining_frames = frameCount - frames_until_end;
//         memcpy(&g_frames[0], 
//                input_samples + (frames_until_end * channels), 
//                remaining_frames * channels * sizeof(ma_int32));
//         g_frames_cursor = 0;
//         // Debug print for the first few frames after wrap
//         for (ma_uint32 frame = 0; frame < (remaining_frames); frame++) {
//             for (ma_uint32 channel = 0; channel < channels; channel++) {
//                 printf("Frame %d, Channel %d: %d\n", frame, channel, g_frames[frame][channel]);
//             }
//         }
//     }
//
//     // Update frame count
//     if (g_frames_count < MAX_FRAMES) {
//         g_frames_count += frameCount;
//         if (g_frames_count > MAX_FRAMES) {
//             g_frames_count = MAX_FRAMES;
//         }
//     }
// }
// void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
//
//   if (frameCount == 0) return;
//
//
// 	const ma_int32*input_samples = (const ma_int32 *)pInput;
// 	ma_uint32 channels = pDevice->capture.channels;
//
// 	if (g_frames_cursor + frameCount < MAX_FRAMES) {
// 		// Process each frame
// 		for (ma_uint32 frame = 0; frame < frameCount; frame++) {
// 			// Extract samples for current frame (one sample per channel)
//
// 			for (ma_uint32 channel = 0; channel < channels; channel++) {
// 				// frame_samples[channel] = input_samples[frame * channels + channel];
// 				g_frames[g_frames_cursor + frame][channel] = input_samples[frame * channels + channel];
// 			}
// 		}
// 		g_frames_cursor += frameCount - 1;
// 	}
// 	// If we reach the end of the buffer, read the last samples
// 	else {
// 		// Copy the last samples to the beginning of the buffer
// 		for (ma_uint32 frame = 0; frame < MAX_FRAMES - g_frames_cursor; frame++) {
// 			for (ma_uint32 channel = 0; channel < channels; channel++) {
// 				g_frames[g_frames_cursor + frame][channel] = input_samples[(g_frames_cursor + frame) * channels + channel];
// 			}
// 		}
// 		g_frames_cursor = 0; // Reset cursor to the beginning
// 	}
//
// 	if(g_frames_count < MAX_FRAMES) {
// 		g_frames_count += frameCount;
// 	} else {
// 		// If we reach the maximum frames, reset the count
// 		g_frames_count = MAX_FRAMES;
// 	}
// }

void render(ma_int32 buf[MAX_FRAMES][MA_MAX_CHANNELS], int fcount) {
	int width = GetRenderWidth();
	int height = GetRenderHeight();

	float w = (((float)width) / (float)fcount);

	for (int i = 0; i < (fcount); i++) {
		ma_int32 sample = buf[i][1]; // Use the first channel for visualization
		// printf("Sample %d: %d\n", i, sample);

		int h = (int)(((float)height / 2) * (sample/1000000000.0f)); // Scale sample to fit the height

		if (sample > 0) {
			DrawRectangle(i * w, height / 2 - h, 1, h / 5 + 1, foreground);
		} else if (sample < 0) {
			h = -h;
			DrawRectangle(i * w, height / 2 + h - (h / 5 + 1), 1, h / 5 + 1,
					  foreground);
		}
	}
}
int main(int argc, char **argv) {
	// Initialization
	//--------------------------------------------------------------------------------------
	ma_result result;
	ma_device_config deviceConfig;
	ma_device device;

	const int screenWidth = 800;
	const int screenHeight = 450;
	// const int buffer_size = 4800;
	const int fps = 90;

	deviceConfig = ma_device_config_init(ma_device_type_duplex);
	deviceConfig.capture.pDeviceID = NULL;
	deviceConfig.capture.format = ma_format_s32;
	deviceConfig.capture.channels = 2;
	deviceConfig.capture.shareMode = ma_share_mode_shared;
	deviceConfig.playback.pDeviceID = NULL;
	deviceConfig.playback.format = ma_format_s32;
	deviceConfig.playback.channels = 2;
	deviceConfig.dataCallback = data_callback;
	result = ma_device_init(NULL, &deviceConfig, &device);
	if (result != MA_SUCCESS) {
		return result;
	}
	ma_device_start(&device);

	InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

	SetTargetFPS(fps); // Set our game to run at 60 frames-per-second
	//--------------------------------------------------------------------------------------
	// Main loop
	while (!WindowShouldClose()) // Detect window close button or ESC key
	{
		// Update
		//----------------------------------------------------------------------------------
		// TODO: Update your variables here
		//----------------------------------------------------------------------------------

		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();

		ClearBackground(background);

		if (g_frames_count != 0) {

			render(g_frames, g_frames_count);
		} else {
			DrawText("No audio data captured yet.", 10, 10, 20, RED);
		}

		if (IsKeyPressed(KEY_ESCAPE)) {
			CloseWindow(); // Close window and OpenGL context
		}

		EndDrawing();
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	//--------------------------------------------------------------------------------------
	CloseWindow(); // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

	ma_device_uninit(&device);

	(void)argc;
	(void)argv;

	return 0;
}

