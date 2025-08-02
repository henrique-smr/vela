/*******************************************************************************************
*
*   AudioConfig v1.0.0 - Tool Description
*
*   MODULE USAGE:
*       #define GUI_AUDIO_CONFIG_IMPLEMENTATION
*       #include "gui_audio_config.h"
*
*       INIT: GuiAudioConfigState state = InitGuiAudioConfig();
*       DRAW: GuiAudioConfig(&state);
*
*   LICENSE: Propietary License
*
*   Copyright (c) 2022 raylib technologies. All Rights Reserved.
*
*   Unauthorized copying of this file, via any medium is strictly prohibited
*   This project is proprietary and confidential unless the owner allows
*   usage in any other form by expresely written permission.
*
**********************************************************************************************/

#include "application.h"
#include "raylib.h"
#include <stdio.h>
// WARNING: raygui implementation is expected to be defined before including this header
#undef RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "sds.h"
#include "audio.h"
#include "audio_analysis.h"

#ifndef GUI_AUDIO_CONFIG_H
#define GUI_AUDIO_CONFIG_H

typedef struct {
    Vector2 anchor01;
    
    bool InputDeviceSelectorEditMode;
    int InputDeviceSelectorIndex;
    bool OutputDeviceSelectorEditMode;
    int OutputDeviceSelectorIndex;
    bool SampleRateInputEditMode;
    int SampleRateInputValue;

    const char *AudioConfigPanelText ;
    const char *InputDeviceSelectorContent ;
    const char *OutputDeviceSelectorContent ;
    const char *InputDeviceSelectorLabelText ;
    const char *OutputDeviceSelectorLabelText ;
    const char *SampleRateInputLabelText ;
    const char *SampleRateInputText ;
    const char *AudioConfigSaveButtonText ;
    const char *AudioConfigCancelButtonText ;
	const char *AudioConfigCloseButtonText;

    Rectangle layoutRecs[10];

    // Custom state variables (depend on development software)
    // NOTE: This variables should be added manually if required

} GuiAudioConfigState;


//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
#endif // GUI_AUDIO_CONFIG_H

/***********************************************************************************
*
*   GUI_AUDIO_CONFIG IMPLEMENTATION
*
************************************************************************************/


//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//...



//----------------------------------------------------------------------------------
// Internal Module Functions Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
GuiAudioConfigState InitGuiAudioConfig(AudioConfig *audio_config)
{

	if(audio_config == NULL) {
		printf("[WARN]: Audio configuration is NULL\n");
		return (GuiAudioConfigState){0};
	}

	AudioDevicesInfo devices_info = get_audio_devices_info();

	int r_width = GetRenderWidth();
	int r_height = GetRenderHeight();

	int max_width = 800;
	int border_delta_half = 25; // 25px on each side

	int width = ((r_width < max_width) ? r_width : max_width);
	int height = 328; // Fixed height for the audio config window

	GuiAudioConfigState state = { 0 };

	state.anchor01 = (Vector2){
		border_delta_half + (float)(r_width - width) / 2,
		border_delta_half + (float)(r_height - height) / 2,
	};

	state.InputDeviceSelectorEditMode = false;
	state.InputDeviceSelectorIndex = 0;
	state.OutputDeviceSelectorEditMode = false;
	state.OutputDeviceSelectorIndex = 0;
	state.SampleRateInputEditMode = false;
	state.SampleRateInputValue = audio_config->sample_rate;

	state.AudioConfigPanelText = "MENU";
	state.InputDeviceSelectorLabelText = "INPUT";
	state.OutputDeviceSelectorLabelText = "OUTPUT";
	state.SampleRateInputLabelText = "SAMPLE RATE";
	state.AudioConfigSaveButtonText = "SAVE";
	state.AudioConfigCancelButtonText = "CANCEL";
	state.AudioConfigCloseButtonText = "QUIT APP";


	sds input_device_names = sdsempty();
	for (ma_uint32 i = 0; i < devices_info.capture_device_count; i++) {
		input_device_names = sdscat(input_device_names, devices_info.capture_devices[i].name);
		if (i < devices_info.capture_device_count - 1) {
			input_device_names = sdscat(input_device_names, ";");
		}
		if( audio_config != NULL && audio_config->capture_device_id != NULL &&
					   ma_device_id_equal(&devices_info.capture_devices[i].id, audio_config->capture_device_id)) {
			state.InputDeviceSelectorIndex = i; // Set the index of the input device
		}
	}
	sds output_device_names = sdsempty();
	for (ma_uint32 i = 0; i < devices_info.playback_device_count; i++) {
		output_device_names = sdscat(output_device_names, devices_info.playback_devices[i].name);
		if (i < devices_info.playback_device_count - 1) {
			output_device_names = sdscat(output_device_names, ";");
		}
		if( audio_config != NULL && audio_config->playback_device_id != NULL &&
					   ma_device_id_equal(&devices_info.playback_devices[i].id, audio_config->playback_device_id)) {
			state.OutputDeviceSelectorIndex = i; // Set the index of the output device
		}
	}

	state.InputDeviceSelectorContent= input_device_names;
	state.OutputDeviceSelectorContent = output_device_names;

	state.layoutRecs[0] = (Rectangle){
		state.anchor01.x + 0,
		state.anchor01.y + 0,
		width,
		352,
	};
	state.layoutRecs[1] = (Rectangle){
		state.anchor01.x + border_delta_half,
		state.anchor01.y + border_delta_half + 48,
		width - 2 * border_delta_half,
		32,
	};
	state.layoutRecs[2] = (Rectangle){
		state.anchor01.x + border_delta_half,
		state.anchor01.y + border_delta_half + 128,
		width - 2 * border_delta_half,
		32,
	};
	state.layoutRecs[3] = (Rectangle){
		state.anchor01.x + border_delta_half,
		state.anchor01.y + border_delta_half + 208,
		width - 2 * border_delta_half,
		32,
	};
	state.layoutRecs[4] = (Rectangle){
		state.anchor01.x + border_delta_half,
		state.anchor01.y + border_delta_half + 16,
		120,
		24,
	};
	state.layoutRecs[5] = (Rectangle){
		state.anchor01.x + border_delta_half,
		state.anchor01.y + border_delta_half + 96,
		120,
		24,
	};
	state.layoutRecs[6] = (Rectangle){
		state.anchor01.x + border_delta_half,
		state.anchor01.y + border_delta_half + 176,
		120,
		24,
	};
	//spread buttons evenly across the bottom of the window
	int half_width = (width - 2 * border_delta_half) / 2;
	state.layoutRecs[7] = (Rectangle){
		state.anchor01.x + border_delta_half ,
		state.anchor01.y + border_delta_half + 264,
		120,
		32,
	};
	state.layoutRecs[8] = (Rectangle){
		state.anchor01.x + width - 240 - 2*border_delta_half,
		state.anchor01.y + border_delta_half + 264,
		120,
		32,
	};
	state.layoutRecs[9] = (Rectangle){
		state.anchor01.x + width - border_delta_half - 120,
		state.anchor01.y + border_delta_half + 264,
		120,
		32,
	};

	return state;
}

void AudioConfigSaveButton(GuiAudioConfigState *state, AudioConfig *audio_config) {
	// This function can be used to handle the OK button click event
	// For now, we just print the current state values
	printf("Input Device: %d\n", state->InputDeviceSelectorIndex);
	printf("Output Device: %d\n", state->OutputDeviceSelectorIndex);
	printf("Sample Rate: %d\n", state->SampleRateInputValue);
	AudioDevicesInfo devices_info = get_audio_devices_info();
	// Here you can add code to apply the changes or save the configuration

	if(is_audio_initialized()) {
		close_audio(); // Close the audio device if already initialized
	}
	if(is_audio_analysis_running()) {
		stop_analysis(); // Stop the audio analysis if already running
		close_analysis(); // Close the audio analysis
	}

	AudioAnalysisConfig audio_analysis_config = init_audio_analysis_config();

	audio_config->sample_rate = state->SampleRateInputValue;
	audio_config->capture_device_id = &devices_info.capture_devices[state->InputDeviceSelectorIndex].id;
	audio_config->playback_device_id = &devices_info.playback_devices[state->OutputDeviceSelectorIndex].id;

	if (init_audio(audio_config) != 0) {
		printf("Failed to initialize audio\n");
	} else {
		printf("Audio initialized successfully\n");
	}

	audio_analysis_config.buffer_size = audio_config->buffer_size;
	audio_analysis_config.channels = audio_config->capture_channels;

	if(start_analysis(&audio_analysis_config) != 0) {
		printf("Failed to start audio analysis\n");
	} else {
		printf("Audio analysis started successfully\n");
	}
}

static void AudioConfigCancelButton(GuiAudioConfigState *state,AudioConfig *audio_config, Application *app)
{
	AudioDevicesInfo devices_info = get_audio_devices_info();
	 // This function can be used to handle the Cancel button click event
	 // For now, we just print a message
	 printf("Audio configuration cancelled\n");
	//reset state to match audio_config object ;
	 state->InputDeviceSelectorIndex = 0;
	 state->OutputDeviceSelectorIndex = 0;
	 state->SampleRateInputValue = audio_config->sample_rate;
	 state->InputDeviceSelectorEditMode = false;
	 state->OutputDeviceSelectorEditMode = false;
	 state->SampleRateInputEditMode = false;
	 state->InputDeviceSelectorIndex = 0;

		sds input_device_names = sdsempty();
	for (ma_uint32 i = 0; i < devices_info.capture_device_count; i++) {
		input_device_names = sdscat(input_device_names, devices_info.capture_devices[i].name);
		if (i < devices_info.capture_device_count - 1) {
			input_device_names = sdscat(input_device_names, ";");
		}
		printf("device is equal: %d\n", ma_device_id_equal(&devices_info.capture_devices[i].id, audio_config->capture_device_id));
		if( audio_config != NULL && audio_config->capture_device_id != NULL &&
					   ma_device_id_equal(&devices_info.capture_devices[i].id, audio_config->capture_device_id)) {
			state->InputDeviceSelectorIndex = i; // Set the index of the input device
		}
	}
	sds output_device_names = sdsempty();
	for (ma_uint32 i = 0; i < devices_info.playback_device_count; i++) {
		output_device_names = sdscat(output_device_names, devices_info.playback_devices[i].name);
		if (i < devices_info.playback_device_count - 1) {
			output_device_names = sdscat(output_device_names, ";");
		}
		printf("device is equal: %d\n", ma_device_id_equal(&devices_info.playback_devices[i].id, audio_config->playback_device_id));
		if( audio_config != NULL && audio_config->playback_device_id != NULL &&
					   ma_device_id_equal(&devices_info.playback_devices[i].id, audio_config->playback_device_id)) {
			state->OutputDeviceSelectorIndex = i; // Set the index of the output device
		}
	}
	state->InputDeviceSelectorContent= input_device_names;
	state->OutputDeviceSelectorContent = output_device_names;


	toggle_menu(app); // Toggle the menu visibility

}


void GuiAudioConfig(GuiAudioConfigState *state, AudioConfig *audio_config, Application *app)
{


    if (state->InputDeviceSelectorEditMode || state->OutputDeviceSelectorEditMode) GuiLock();

    GuiPanel(state->layoutRecs[0], state->AudioConfigPanelText);
    if (GuiValueBox(state->layoutRecs[3], state->SampleRateInputText, &state->SampleRateInputValue, 0, 500000, state->SampleRateInputEditMode)) state->SampleRateInputEditMode = !state->SampleRateInputEditMode;
    GuiLabel(state->layoutRecs[4], state->InputDeviceSelectorLabelText);
    GuiLabel(state->layoutRecs[5], state->OutputDeviceSelectorLabelText);
    GuiLabel(state->layoutRecs[6], state->SampleRateInputLabelText);
    if (GuiButton(state->layoutRecs[9], state->AudioConfigSaveButtonText)) AudioConfigSaveButton(state, audio_config); 
    if (GuiButton(state->layoutRecs[8], state->AudioConfigCancelButtonText)) AudioConfigCancelButton(state, audio_config, app); 
	 if (GuiButton(state->layoutRecs[7], state->AudioConfigCloseButtonText)) close_application(app); // Close the audio config window
    if (GuiDropdownBox(state->layoutRecs[2], state->OutputDeviceSelectorContent, &state->OutputDeviceSelectorIndex, state->OutputDeviceSelectorEditMode)) state->OutputDeviceSelectorEditMode = !state->OutputDeviceSelectorEditMode;
    if (GuiDropdownBox(state->layoutRecs[1], state->InputDeviceSelectorContent, &state->InputDeviceSelectorIndex, state->InputDeviceSelectorEditMode)) state->InputDeviceSelectorEditMode = !state->InputDeviceSelectorEditMode;
    
	GuiUnlock();
}

