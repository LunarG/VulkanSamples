/**************************************************************************
 *
 * Copyright 2014 Valve Software. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *************************************************************************/

#include "glvdebug_xgl_settings.h"

// declared as extern in header
glvdebug_xgl_settings g_xglDebugSettings;
static glvdebug_xgl_settings s_defaultXglSettings;

glv_SettingInfo g_settings_info[] =
{
    { "ri", "PrintReplayInfoMsgs", GLV_SETTING_BOOL, &g_xglDebugSettings.printReplayInfoMsgs, &s_defaultXglSettings.printReplayInfoMsgs, TRUE, "Print info messages reported when replaying trace file."},
    { "rw", "PrintReplayWarningMsgs", GLV_SETTING_BOOL, &g_xglDebugSettings.printReplayWarningMsgs, &s_defaultXglSettings.printReplayWarningMsgs, TRUE, "Print warning messages reported when replaying trace file."},
    { "re", "PrintReplayErrorMsgs", GLV_SETTING_BOOL, &g_xglDebugSettings.printReplayErrorMsgs, &s_defaultXglSettings.printReplayErrorMsgs, TRUE, "Print error messages reported when replaying trace file."},
    { "pi", "PauseOnReplayInfo", GLV_SETTING_BOOL, &g_xglDebugSettings.pauseOnReplayInfo, &s_defaultXglSettings.pauseOnReplayInfo, TRUE, "Pause replay if an info message is reported."},
    { "pw", "PauseOnReplayWarning", GLV_SETTING_BOOL, &g_xglDebugSettings.pauseOnReplayWarning, &s_defaultXglSettings.pauseOnReplayWarning, TRUE, "Pause replay if a warning message is reported."},
    { "pe", "PauseOnReplayError", GLV_SETTING_BOOL, &g_xglDebugSettings.pauseOnReplayError, &s_defaultXglSettings.pauseOnReplayError, TRUE, "Pause replay if an error message is reported."},
};

glv_SettingGroup g_xglDebugSettingGroup =
{
    "glvdebug_xgl",
    sizeof(g_settings_info) / sizeof(g_settings_info[0]),
    &g_settings_info[0]
};

void initialize_default_settings()
{
    s_defaultXglSettings.printReplayInfoMsgs = FALSE;
    s_defaultXglSettings.printReplayWarningMsgs = TRUE;
    s_defaultXglSettings.printReplayErrorMsgs = TRUE;
    s_defaultXglSettings.pauseOnReplayInfo = FALSE;
    s_defaultXglSettings.pauseOnReplayWarning = FALSE;
    s_defaultXglSettings.pauseOnReplayError = TRUE;
};
