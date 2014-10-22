/*
 * XGL Tests
 *
 * Copyright (C) 2014 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#ifndef XGLRENDERFRAMEWORK_H
#define XGLRENDERFRAMEWORK_H

#include "xgltestframework.h"

class XglRenderFramework : public XglTestFramework
{
public:
    XglRenderFramework();
    ~XglRenderFramework();

    void CreateQueryPool(XGL_QUERY_TYPE type, XGL_UINT slots,
                         XGL_QUERY_POOL *pPool, XGL_GPU_MEMORY *pMem);
    void DestroyQueryPool(XGL_QUERY_POOL pool, XGL_GPU_MEMORY mem);

    XGL_DEVICE device() {return m_device->device();}
    void CreateShader(XGL_PIPELINE_SHADER_STAGE stage, const char *shader_code, XGL_SHADER *pshader);
    void InitPipeline();
    void InitMesh( XGL_UINT32 numVertices, XGL_GPU_SIZE vbStride, const void* vertices );
    void InitConstantBuffer( int constantCount, int constantSize, const void* data );
    void UpdateConstantBuffer(const void* data);
    void InitViewport(float width, float height);
    void InitViewport();
    void InitRenderTarget();
    void InitFramework();
    void ShutdownFramework();
    void InitState();
    void CreateDefaultPipeline(XGL_PIPELINE* pipeline, XGL_SHADER vs, XGL_SHADER ps);
    void GenerateClearAndPrepareBufferCmds();
    void GenerateBindRenderTargetCmd();
    void GenerateBindStateAndPipelineCmds(XGL_PIPELINE* pipeline);

protected:
    XGL_APPLICATION_INFO            app_info;
    XGL_PHYSICAL_GPU                objs[MAX_GPUS];
    XGL_UINT                        gpu_count;
    XGL_GPU_MEMORY                  m_descriptor_set_mem;
    XGL_GPU_MEMORY                  m_pipe_mem;
    XglDevice                      *m_device;
    XGL_CMD_BUFFER                  m_cmdBuffer;
    XGL_UINT32                      m_numVertices;
    XGL_MEMORY_VIEW_ATTACH_INFO     m_vtxBufferView;
    XGL_MEMORY_VIEW_ATTACH_INFO     m_constantBufferView;
    XGL_GPU_MEMORY                  m_vtxBufferMem;
    XGL_GPU_MEMORY                  m_constantBufferMem;
    XGL_UINT32                      m_numMemRefs;
    XGL_MEMORY_REF                  m_memRefs[5];
    XGL_RASTER_STATE_OBJECT         m_stateRaster;
    XGL_COLOR_BLEND_STATE_OBJECT    m_colorBlend;
    XGL_VIEWPORT_STATE_OBJECT       m_stateViewport;
    XGL_DEPTH_STENCIL_STATE_OBJECT  m_stateDepthStencil;
    XGL_MSAA_STATE_OBJECT           m_stateMsaa;
    XGL_DESCRIPTOR_SET              m_rsrcDescSet;
    XglImage                       *m_renderTarget;
    XGL_FLOAT                       m_width, m_height;
    XGL_FORMAT                      m_render_target_fmt;
    XGL_COLOR_ATTACHMENT_BIND_INFO  m_colorBinding;
    XGL_DEPTH_STENCIL_BIND_INFO     m_depthStencilBinding;

    /*
     * SetUp and TearDown are called by the Google Test framework
     * to initialize a test framework based on this class.
     */
    virtual void SetUp() {
        XGL_RESULT err;

        this->app_info.sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = (const XGL_CHAR *) "base";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = (const XGL_CHAR *) "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = XGL_MAKE_VERSION(0, 22, 0);

        memset(&m_vtxBufferView, 0, sizeof(m_vtxBufferView));
        m_vtxBufferView.sType = XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO;

        memset(&m_constantBufferView, 0, sizeof(m_constantBufferView));
        m_constantBufferView.sType = XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO;

        err = xglInitAndEnumerateGpus(&app_info, NULL,
                                      MAX_GPUS, &this->gpu_count, objs);
        ASSERT_XGL_SUCCESS(err);
        ASSERT_GE(1, this->gpu_count) << "No GPU available";

        m_device = new XglDevice(0, objs[0]);
        m_device->get_device_queue();
    }

    virtual void TearDown() {
        xglInitAndEnumerateGpus(&this->app_info, XGL_NULL_HANDLE, 0, &gpu_count, XGL_NULL_HANDLE);
    }
};

#endif // XGLRENDERFRAMEWORK_H
