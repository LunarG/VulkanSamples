#ifndef VKREPLAY__MAIN_H
#define VKREPLAY__MAIN_H

typedef struct vkreplayer_settings
{
    char* pTraceFilePath;
    unsigned int numLoops;
    int loopStartFrame;
    int loopEndFrame;
    char* screenshotList;
} vkreplayer_settings;

#endif // VKREPLAY__MAIN_H
