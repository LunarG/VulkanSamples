#ifndef GLVDEBUG_SETTINGS_H
#define GLVDEBUG_SETTINGS_H

#include <QString>

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

bool initialize_settings(int argc, char* argv[]);

#endif // GLVDEBUG_SETTINGS_H
