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

#include "glvdebug_vk_settings.h"

// declared as extern in header
glvdebug_vk_settings g_vkDebugSettings;
static glvdebug_vk_settings s_defaultVkSettings;

glv_SettingInfo g_settings_info[] =
{
    { "ri", "PrintReplayInfoMsgs", GLV_SETTING_BOOL, &g_vkDebugSettings.printReplayInfoMsgs, &s_defaultVkSettings.printReplayInfoMsgs, TRUE, "Print info messages reported when replaying trace file."},
    { "rw", "PrintReplayWarningMsgs", GLV_SETTING_BOOL, &g_vkDebugSettings.printReplayWarningMsgs, &s_defaultVkSettings.printReplayWarningMsgs, TRUE, "Print warning messages reported when replaying trace file."},
    { "re", "PrintReplayErrorMsgs", GLV_SETTING_BOOL, &g_vkDebugSettings.printReplayErrorMsgs, &s_defaultVkSettings.printReplayErrorMsgs, TRUE, "Print error messages reported when replaying trace file."},
    { "pi", "PauseOnReplayInfo", GLV_SETTING_BOOL, &g_vkDebugSettings.pauseOnReplayInfo, &s_defaultVkSettings.pauseOnReplayInfo, TRUE, "Pause replay if an info message is reported."},
    { "pw", "PauseOnReplayWarning", GLV_SETTING_BOOL, &g_vkDebugSettings.pauseOnReplayWarning, &s_defaultVkSettings.pauseOnReplayWarning, TRUE, "Pause replay if a warning message is reported."},
    { "pe", "PauseOnReplayError", GLV_SETTING_BOOL, &g_vkDebugSettings.pauseOnReplayError, &s_defaultVkSettings.pauseOnReplayError, TRUE, "Pause replay if an error message is reported."},
    { "gf", "GroupByFrame", GLV_SETTING_BOOL, &g_vkDebugSettings.groupByFrame, &s_defaultVkSettings.groupByFrame, TRUE, "Group API calls by frame."},
    { "gt", "GroupByThread", GLV_SETTING_BOOL, &g_vkDebugSettings.groupByThread, &s_defaultVkSettings.groupByThread, TRUE, "Group API calls by the CPU thread Id on which they executed."},
    { "rw", "replay_window_width", GLV_SETTING_INT, &g_vkDebugSettings.replay_window_width, &s_defaultVkSettings.replay_window_width, TRUE, "Width of replay window on startup."},
    { "rh", "replay_window_height", GLV_SETTING_INT, &g_vkDebugSettings.replay_window_height, &s_defaultVkSettings.replay_window_height, TRUE, "Height of replay window on startup."},
    { "sr", "separate_replay_window", GLV_SETTING_BOOL, &g_vkDebugSettings.separate_replay_window, &s_defaultVkSettings.separate_replay_window, TRUE, "Use a separate replay window."},
};

glv_SettingGroup g_vkDebugSettingGroup =
{
    "glvdebug_vk",
    sizeof(g_settings_info) / sizeof(g_settings_info[0]),
    &g_settings_info[0]
};

void initialize_default_settings()
{
    s_defaultVkSettings.printReplayInfoMsgs = FALSE;
    s_defaultVkSettings.printReplayWarningMsgs = TRUE;
    s_defaultVkSettings.printReplayErrorMsgs = TRUE;
    s_defaultVkSettings.pauseOnReplayInfo = FALSE;
    s_defaultVkSettings.pauseOnReplayWarning = FALSE;
    s_defaultVkSettings.pauseOnReplayError = TRUE;
    s_defaultVkSettings.groupByFrame = FALSE;
    s_defaultVkSettings.groupByThread = FALSE;
    s_defaultVkSettings.replay_window_width = 1024;
    s_defaultVkSettings.replay_window_height = 768;
    s_defaultVkSettings.separate_replay_window = FALSE;
};
