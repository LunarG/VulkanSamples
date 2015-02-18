#ifndef GLVREPLAY_MAIN_H
#define GLVREPLAY_MAIN_H

typedef struct glvreplay_settings
{
    char* pTraceFilePath;
    BOOL benchmark;
    unsigned int numLoops;
    char* screenshotList;
} glvreplay_settings;

#endif // GLVREPLAY_MAIN_H
