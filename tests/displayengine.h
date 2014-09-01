#ifndef DISPLAYENGINE_H
#define DISPLAYENGINE_H

#include "xgl.h"
#include "GL/freeglut_std.h"

class DisplayEngine
{
public:
    DisplayEngine();

    void Init(bool enable, int w, int h);
    void Display(XGL_IMAGE image, XGL_GPU_MEMORY image_mem);

    void Reshape(int w, int h);

protected:
    bool m_enable;          // Indicates if calls do any OpenGL or not.
    int m_width, m_height;
};

#endif // DISPLAYENGINE_H
