#ifndef GLVDEBUG_SETTINGS_H
#define GLVDEBUG_SETTINGS_H

#include <QString>

extern "C" {
#include "glv_settings.h"
}

extern glv_SettingGroup* g_pAllSettings;
extern unsigned int g_numAllSettings;

typedef struct glvdebug_settings
{
    char * trace_file_to_open;
    int window_position_left;
    int window_position_top;
    int window_size_width;
    int window_size_height;
    //unsigned int trim_large_trace_prompt_size;

    //bool groups_state_render;
    //bool groups_push_pop_markers;
    //bool groups_nested_calls;
} glvtrace_settings;

extern glvdebug_settings g_settings;
extern glv_SettingGroup g_settingGroup;

bool initialize_settings(int argc, char* argv[]);

QString get_settings_file_path();
QString get_sessions_directory();

#endif // GLVDEBUG_SETTINGS_H
