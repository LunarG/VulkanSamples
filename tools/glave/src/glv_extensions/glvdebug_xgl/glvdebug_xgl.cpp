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
#include "glvdebug_xgl_qcontroller.h"

#include "glvdebug_controller.h"

static glvdebug_xgl_QController* s_pQController;

#define CREATE_CONTROLLER() if(s_pQController == NULL) { s_pQController = new glvdebug_xgl_QController(); }

extern "C"
{

GLVTRACER_EXPORT bool GLVTRACER_CDECL glvdebug_controller_load_trace_file(glvdebug_trace_file_info* pTraceFileInfo, glvdebug_view* pView)
{
    CREATE_CONTROLLER()
    bool result = false;
    if (s_pQController != NULL)
    {
        result = s_pQController->LoadTraceFile(pTraceFileInfo, pView);
    }
    return result;
}

GLVTRACER_EXPORT void GLVTRACER_CDECL glvdebug_controller_unload_trace_file()
{
    CREATE_CONTROLLER()
    if (s_pQController != NULL)
    {
        s_pQController->UnloadTraceFile();
        delete s_pQController;
        s_pQController = NULL;
    }
}

GLVTRACER_EXPORT glv_trace_packet_header* GLVTRACER_CDECL glvdebug_controller_interpret_trace_packet(glv_trace_packet_header* pHeader)
{
    CREATE_CONTROLLER()
    glv_trace_packet_header* pInterpretedHeader = NULL;
    if (s_pQController != NULL)
    {
        pInterpretedHeader = s_pQController->InterpretTracePacket(pHeader);
    }
    return pInterpretedHeader;
}

}
