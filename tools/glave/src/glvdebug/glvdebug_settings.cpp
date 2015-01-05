#include "glvdebug_settings.h"
#include <assert.h>

#include <QCoreApplication>

extern "C" {
#include "glv_settings.h"
}

static const unsigned int GLVDEBUG_SETTINGS_FILE_FORMAT_VERSION_1 = 1;
static const unsigned int GLVDEBUG_SETTINGS_FILE_FORMAT_VERSION = GLVDEBUG_SETTINGS_FILE_FORMAT_VERSION_1;

static const char *s_SETTINGS_FILE = "glvdebug_settings.txt";

// declared as extern in header
glvdebug_settings g_settings;
static glvdebug_settings s_default_settings;
glv_SettingGroup* g_pAllSettings = NULL;
unsigned int g_numAllSettings = 0;

glv_SettingInfo g_settings_info[] =
{
    //{ "tltps", "trim_large_trace_prompt_size", GLV_SETTING_UINT, &g_settings.trim_large_trace_prompt_size, &s_default_settings.trim_large_trace_prompt_size, TRUE, "The number of frames in a trace file at which the user should be prompted to trim the trace before loading."},

    { "wl", "window_left", GLV_SETTING_INT, &g_settings.window_position_left, &s_default_settings.window_position_left, TRUE, "Left coordinate of GLVDebug window on startup."},
    { "wt", "window_top", GLV_SETTING_INT, &g_settings.window_position_top, &s_default_settings.window_position_top, TRUE, "Top coordinate of GLVDebug window on startup."},
    { "ww", "window_width", GLV_SETTING_INT, &g_settings.window_size_width, &s_default_settings.window_size_width, TRUE, "Width of GLVDebug window on startup."},
    { "wh", "window_height", GLV_SETTING_INT, &g_settings.window_size_height, &s_default_settings.window_size_height, TRUE, "Height of GLVDebug window on startup."},

    //{ "gsr", "group_state_render", GLV_SETTING_BOOL, &g_settings.groups_state_render, &s_default_settings.groups_state_render, TRUE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    //{ "gppm", "groups_push_pop_markers", GLV_SETTING_BOOL, &g_settings.groups_push_pop_markers, &s_default_settings.groups_push_pop_markers, TRUE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    //{ "gnc", "groups_nested_calls", GLV_SETTING_BOOL, &g_settings.groups_nested_calls, &s_default_settings.groups_nested_calls, TRUE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
};

glv_SettingGroup g_settingGroup =
{
    "glvdebug",
    sizeof(g_settings_info) / sizeof(g_settings_info[0]),
    &g_settings_info[0]
};

QString get_settings_file_path()
{
    return get_sessions_directory() + QString(s_SETTINGS_FILE);
}

QString get_sessions_directory()
{
    return QCoreApplication::applicationDirPath() + "/sessions/";
}

bool initialize_settings(int argc, char* argv[])
{
    bool bSuccess = true;

    // setup default values
    memset(&s_default_settings, 0, sizeof(glvdebug_settings));

    s_default_settings.window_position_left = 0;
    s_default_settings.window_position_top = 0;
    s_default_settings.window_size_width = 1024;
    s_default_settings.window_size_height = 768;

    // initialize settings as defaults
    g_settings = s_default_settings;

    QString settingsFilePath = get_settings_file_path();
    FILE* pFile = fopen(settingsFilePath.toStdString().c_str(), "r");
    if (pFile == NULL)
    {
        glv_LogWarn("Failed to open settings file: '%s'.\n", settingsFilePath.toStdString().c_str());
    }

    // Secondly set options based on settings file
    if (pFile != NULL)
    {
        g_pAllSettings = NULL;
        g_numAllSettings = 0;
        if (glv_SettingGroup_Load_from_file(pFile, &g_pAllSettings, &g_numAllSettings) == -1)
        {
            glv_SettingGroup_print(&g_settingGroup);
            return false;
        }

        glv_SettingGroup_Apply_Overrides(&g_settingGroup, g_pAllSettings, g_numAllSettings);
    }

    glv_SettingGroup_init_from_cmdline(&g_settingGroup, argc, argv, &g_settings.trace_file_to_open);

    // apply settings from settings file and from cmd-line args
    if (glv_SettingGroup_init_from_cmdline(&g_settingGroup, argc, argv, &g_settings.trace_file_to_open) != 0)
    {
        // invalid options specified
        bSuccess = false;
    }

    if (pFile != NULL)
    {
        fclose(pFile);
        pFile = NULL;
    }

    // This would be a good place to validate any "required" settings, but right now there aren't any!

    if (bSuccess == false)
    {
        glv_SettingGroup_print(&g_settingGroup);
        glv_SettingGroup_delete(&g_settingGroup);
    }

    return bSuccess;
}
