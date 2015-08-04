#include "glvdebug_settings.h"
#include "glvdebug_output.h"
#include <assert.h>

#include <QCoreApplication>
#include <QDir>

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
    { "o", "OpenTraceFile", GLV_SETTING_STRING, &g_settings.trace_file_to_open, &s_default_settings.trace_file_to_open, TRUE, "Load the specified trace file when glvdebug is opened."},
    { "wl", "WindowLeft", GLV_SETTING_INT, &g_settings.window_position_left, &s_default_settings.window_position_left, TRUE, "Left coordinate of GLVDebug window on startup."},
    { "wt", "WindowTop", GLV_SETTING_INT, &g_settings.window_position_top, &s_default_settings.window_position_top, TRUE, "Top coordinate of GLVDebug window on startup."},
    { "ww", "WindowWidth", GLV_SETTING_INT, &g_settings.window_size_width, &s_default_settings.window_size_width, TRUE, "Width of GLVDebug window on startup."},
    { "wh", "WindowHeight", GLV_SETTING_INT, &g_settings.window_size_height, &s_default_settings.window_size_height, TRUE, "Height of GLVDebug window on startup."},

    { "", "GenTraceApplication", GLV_SETTING_STRING, &g_settings.gentrace_application, &s_default_settings.gentrace_application, FALSE, "The most recent application path in the 'Generate Trace' dialog."},
    { "", "GenTraceArguments", GLV_SETTING_STRING, &g_settings.gentrace_arguments, &s_default_settings.gentrace_arguments, FALSE, "The most recent application arguments in the 'Generate Trace' dialog."},
    { "", "GenTraceWorkingDir", GLV_SETTING_STRING, &g_settings.gentrace_working_dir, &s_default_settings.gentrace_working_dir, FALSE, "The most recent working directory in the 'Generate Trace' dialog."},
    { "", "GenTraceTracerLib", GLV_SETTING_STRING, &g_settings.gentrace_tracer_lib, &s_default_settings.gentrace_tracer_lib, FALSE, "The most recent tracer library in the 'Generate Trace' dialog."},
    { "", "GenTraceOutputFile", GLV_SETTING_STRING, &g_settings.gentrace_output_file, &s_default_settings.gentrace_output_file, FALSE, "The most recent output trace file in the 'Generate Trace' dialog."},

    { "", "SettingsDialogWidth", GLV_SETTING_INT, &g_settings.settings_dialog_width, &s_default_settings.settings_dialog_width, TRUE, "Width of GLVDebug settings dialog when opened."},
    { "", "SettingsDialogHeight", GLV_SETTING_INT, &g_settings.settings_dialog_height, &s_default_settings.settings_dialog_height, TRUE, "Height of GLVDebug settings dialog when opened."},

    //{ "tltps", "trim_large_trace_prompt_size", GLV_SETTING_UINT, &g_settings.trim_large_trace_prompt_size, &s_default_settings.trim_large_trace_prompt_size, TRUE, "The number of frames in a trace file at which the user should be prompted to trim the trace before loading."},
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

QString glvdebug_get_settings_file_path()
{
    QString result = glvdebug_get_settings_directory() + "/" + QString(s_SETTINGS_FILE);
    return result;
}

QString glvdebug_get_settings_directory()
{
    char * pSettingsPath = glv_platform_get_settings_path();
    QString result = QString(pSettingsPath);
    glv_free(pSettingsPath);
    return result;
}

QString glvdebug_get_sessions_directory()
{
    char * pDataPath = glv_platform_get_data_path();
    QString result = QString(pDataPath) + "/sessions/";
    glv_free(pDataPath);
    return result;
}

bool glvdebug_initialize_settings(int argc, char* argv[])
{
    bool bSuccess = true;

    // setup default values
    memset(&s_default_settings, 0, sizeof(glvdebug_settings));

    s_default_settings.trace_file_to_open = NULL;
    s_default_settings.window_position_left = 0;
    s_default_settings.window_position_top = 0;
    s_default_settings.window_size_width = 1024;
    s_default_settings.window_size_height = 768;

    s_default_settings.gentrace_application = NULL;
    s_default_settings.gentrace_arguments = NULL;
    s_default_settings.gentrace_working_dir = NULL;
    s_default_settings.gentrace_tracer_lib = NULL;
    s_default_settings.gentrace_output_file = NULL;

    // This seems to be a reasonable default size for the dialog.
    s_default_settings.settings_dialog_width = 600;
    s_default_settings.settings_dialog_height = 600;

    // initialize settings as defaults
    g_settings = s_default_settings;

    QString settingsFilePath = glvdebug_get_settings_file_path();
    FILE* pFile = fopen(settingsFilePath.toStdString().c_str(), "r");
    if (pFile == NULL)
    {
        glv_LogWarning("Unable to open settings file: '%s'.", settingsFilePath.toStdString().c_str());
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

        if (g_pAllSettings != NULL && g_numAllSettings > 0)
        {
            glv_SettingGroup_Apply_Overrides(&g_settingGroup, g_pAllSettings, g_numAllSettings);
        }

        fclose(pFile);
        pFile = NULL;
    }

    // apply settings from settings file and from cmd-line args
    if (glv_SettingGroup_init_from_cmdline(&g_settingGroup, argc, argv, &g_settings.trace_file_to_open) != 0)
    {
        // invalid options specified
        bSuccess = false;
    }

    // Merge known glvdebug settings into the loaded settings.
    // This ensures that new known settings are added to the settings dialog
    // and will be re-written to the settings file upon saving.
    glv_SettingGroup_merge(&g_settingGroup, &g_pAllSettings, &g_numAllSettings);

    // This would be a good place to validate any "required" settings, but right now there aren't any!

    if (bSuccess == false)
    {
        glv_SettingGroup_print(&g_settingGroup);
        glv_SettingGroup_delete(&g_settingGroup);
    }

    return bSuccess;
}

void glvdebug_settings_updated()
{
    glv_SettingGroup_update(&g_settingGroup, g_pAllSettings, g_numAllSettings);
}

void glvdebug_save_settings()
{
    QDir settingsDir(glvdebug_get_settings_directory());
    if (settingsDir.mkpath(".") == false)
    {
        QString error = "Failed to create " + settingsDir.path();
        glvdebug_output_error(error);
    }

    QString filepath = glvdebug_get_settings_file_path();
    FILE* pSettingsFile = fopen(filepath.toStdString().c_str(), "w");
    if (pSettingsFile == NULL)
    {
        QString error = "Failed to open settings file for writing: " + filepath;
        glvdebug_output_error(error);
    }
    else
    {
        if (glv_SettingGroup_save(g_pAllSettings, g_numAllSettings, pSettingsFile) == FALSE)
        {
            QString error = "Failed to save settings file: " + filepath;
            glvdebug_output_error(error);
        }

        fclose(pSettingsFile);
    }
}
