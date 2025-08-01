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

#include "raylib.h"
#include <stdio.h>
// WARNING: raygui implementation is expected to be defined before including this header
#undef RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <string.h>     // Required for: strcpy()

#ifndef GUI_AUDIO_CONFIG_H
#define GUI_AUDIO_CONFIG_H

typedef struct {
    bool InputDeviceSelectorEditMode;
    int InputDeviceSelectorIndex;
    bool OutputDeviceSelectorEditMode;
    int OutputDeviceSelectorIndex;
    bool SampleRateInputEditMode;
    int SampleRateInputValue;

    Rectangle layoutRecs[6];

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
GuiAudioConfigState InitGuiAudioConfig(void);
void GuiAudioConfig(GuiAudioConfigState *state);
void GuiAudio_onOkButton(GuiAudioConfigState *state);

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
GuiAudioConfigState InitGuiAudioConfig(void)
{
    GuiAudioConfigState state = { 0 };

    state.InputDeviceSelectorEditMode = false;
    state.InputDeviceSelectorIndex = 0;
    state.OutputDeviceSelectorEditMode = false;
    state.OutputDeviceSelectorIndex = 0;
    state.SampleRateInputEditMode = false;
    state.SampleRateInputValue = 0;

    state.layoutRecs[0] = (Rectangle){ 352, 184, 296, 24 };
    state.layoutRecs[1] = (Rectangle){ 352, 216, 296, 24 };
    state.layoutRecs[2] = (Rectangle){ 432, 264, 216, 24 };
    state.layoutRecs[3] = (Rectangle){ 352, 112, 120, 24 };
    state.layoutRecs[4] = (Rectangle){ 352, 184, 120, 24 };
    state.layoutRecs[5] = (Rectangle){ 360, 312, 288, 24 };

    // Custom variables initialization

    return state;
}

void GuiAudioConfig(GuiAudioConfigState *state)
{
    const char *InputDeviceSelectorText = "ONE;TWO;THREE";
    const char *OutputDeviceSelectorText = "ONE;TWO;THREE";
	 const char *SampleRateInputText = "Sample Rate";
    const char *Label003Text = "Input";
    const char *Label004Text = "Output";
    const char *OkButtonText = "Ok";
    
    if (state->InputDeviceSelectorEditMode || state->OutputDeviceSelectorEditMode) GuiLock();

    if (GuiValueBox(state->layoutRecs[2], SampleRateInputText, &state->SampleRateInputValue, 0, 100, state->SampleRateInputEditMode)) state->SampleRateInputEditMode = !state->SampleRateInputEditMode;
    GuiLabel(state->layoutRecs[3], Label003Text);
    GuiLabel(state->layoutRecs[4], Label004Text);
    if (GuiButton(state->layoutRecs[5], OkButtonText)) GuiAudio_onOkButton(state); 
    if (GuiDropdownBox(state->layoutRecs[0], InputDeviceSelectorText, &state->InputDeviceSelectorIndex, state->InputDeviceSelectorEditMode)) state->InputDeviceSelectorEditMode = !state->InputDeviceSelectorEditMode;
    if (GuiDropdownBox(state->layoutRecs[1], OutputDeviceSelectorText, &state->OutputDeviceSelectorIndex, state->OutputDeviceSelectorEditMode)) state->OutputDeviceSelectorEditMode = !state->OutputDeviceSelectorEditMode;
    
    GuiUnlock();
}

