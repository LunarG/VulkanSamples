#ifndef GLVREPLAY_MAIN_H
#define GLVREPLAY_MAIN_H

#endif // GLVREPLAY_MAIN_H

typedef struct glvreplay_settings
{
    char* pTraceFilePath;
    BOOL benchmark;
    unsigned int numLoops;
    char* snapshotList;
} glvreplay_settings;
