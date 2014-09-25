//  XGL tests
//
//  Copyright (C) 2014 LunarG, Inc.
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.


// Display image results from tests

#include "displayengine.h"
#include "test_common.h"

DisplayEngine::DisplayEngine()
{
    m_enable = false;
}

void DisplayEngine::Init(bool enable, int w, int h)
{
    m_enable = enable;
    m_width = w;
    m_height = h;

    if (!m_enable) return;  // Do nothing except save info if not enabled

    glutInitWindowSize(w, h);

    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutCreateWindow(NULL);
    Reshape(w, h);
}

void DisplayEngine::Reshape( int w, int h )
{
    m_width = w;
    m_height = h;

    if (!m_enable) return;  // Do nothing except save info if not enabled

    glutReshapeWindow(m_width, m_height);

    glViewport( 0, 0, m_width, m_height );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( 0.0, m_width, 0.0, m_height, 0.0, 2.0 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

//    glScissor(width/4, height/4, width/2, height/2);
}

void DisplayEngine::Display(XGL_IMAGE image, XGL_GPU_MEMORY image_mem)
{
    XGL_RESULT err;

    const XGL_IMAGE_SUBRESOURCE sr = {
        XGL_IMAGE_ASPECT_COLOR, 0, 0
    };
    XGL_SUBRESOURCE_LAYOUT sr_layout;
    XGL_UINT data_size = sizeof(sr_layout);

    if (!m_enable) return;  // Do nothing except save info if not enabled

    err = xglGetImageSubresourceInfo( image, &sr, XGL_INFO_TYPE_SUBRESOURCE_LAYOUT,
                                      &data_size, &sr_layout);
    ASSERT_XGL_SUCCESS( err );
    ASSERT_EQ(data_size, sizeof(sr_layout));

    const char *ptr;

    err = xglMapMemory( image_mem, 0, (XGL_VOID **) &ptr );
    ASSERT_XGL_SUCCESS( err );

    ptr += sr_layout.offset;

    glClearColor(0, 0, 0, 0);
    glClear( GL_COLOR_BUFFER_BIT);
    glRasterPos3f(0, 0, 0);
    glBitmap(0, 0, 0, 0, 0, 0, NULL);
    glDrawPixels(m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, ptr);

    glutSwapBuffers();

    err = xglUnmapMemory( image_mem );
    ASSERT_XGL_SUCCESS( err );
}
