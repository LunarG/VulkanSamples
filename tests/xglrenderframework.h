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

    XGL_DEVICE device() {return m_device->device();}
    XGL_PHYSICAL_GPU gpu() {return objs[0];}
    void InitViewport(float width, float height);
    void InitViewport();
    void InitRenderTarget();
    void InitFramework();
    void ShutdownFramework();
    void InitState();
    void GenerateClearAndPrepareBufferCmds();
    void GenerateBindRenderTargetCmd();
    void GenerateBindStateAndPipelineCmds();


protected:
    XGL_APPLICATION_INFO            app_info;
    XGL_PHYSICAL_GPU                objs[MAX_GPUS];
    XGL_UINT                        gpu_count;
    XglDevice                      *m_device;
    XGL_CMD_BUFFER                  m_cmdBuffer;
    XGL_MEMORY_REF                  m_memRefs[5];
    XGL_RASTER_STATE_OBJECT         m_stateRaster;
    XGL_COLOR_BLEND_STATE_OBJECT    m_colorBlend;
    XGL_VIEWPORT_STATE_OBJECT       m_stateViewport;
    XGL_DEPTH_STENCIL_STATE_OBJECT  m_stateDepthStencil;
    XGL_MSAA_STATE_OBJECT           m_stateMsaa;
    XglImage                       *m_renderTargets[XGL_MAX_COLOR_ATTACHMENTS];
    XGL_UINT                        m_renderTargetCount;
    XGL_FLOAT                       m_width, m_height;
    XGL_FORMAT                      m_render_target_fmt;
    XGL_COLOR_ATTACHMENT_BIND_INFO  m_colorBindings[XGL_MAX_COLOR_ATTACHMENTS];
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

        err = xglInitAndEnumerateGpus(&app_info, NULL,
                                      MAX_GPUS, &this->gpu_count, objs);
        ASSERT_XGL_SUCCESS(err);
        ASSERT_GE(this->gpu_count, 1) << "No GPU available";

        m_device = new XglDevice(0, objs[0]);
        m_device->get_device_queue();
    }

    virtual void TearDown() {
        xglInitAndEnumerateGpus(&this->app_info, XGL_NULL_HANDLE, 0, &gpu_count, XGL_NULL_HANDLE);
    }
};

class XglCommandBufferObj
{
public:
    XglCommandBufferObj(XglDevice *device);
    ~XglCommandBufferObj();
    XGL_CMD_BUFFER  GetBufferHandle();
    XGL_CMD_BUFFER* GetBufferPointer();

protected:
    XglDevice                      *m_device;
    XGL_CMD_BUFFER_CREATE_INFO      m_cmdInfo;
    XGL_CMD_BUFFER                  m_cmdBuffer;

};

class XglConstantBufferObj
{
public:
    XglConstantBufferObj(XglDevice *device);
    XglConstantBufferObj(XglDevice *device, int constantCount, int constantSize, const void* data);
    ~XglConstantBufferObj();
    void SetMemoryState(XGL_CMD_BUFFER cmdBuffer, XGL_MEMORY_STATE newState);
    void Bind(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_SIZE offset, XGL_UINT binding);
    XGL_MEMORY_VIEW_ATTACH_INFO     m_constantBufferView;
    XGL_GPU_MEMORY                  m_constantBufferMem;

protected:
    XglDevice                      *m_device;
    int                             m_numVertices;
    int                             m_stride;
};

class XglIndexBufferObj : public XglConstantBufferObj
{
public:
    XglIndexBufferObj(XglDevice *device);
    void CreateAndInitBuffer(int numIndexes, XGL_INDEX_TYPE dataFormat, const void* data);
    void Bind(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_SIZE offset);

protected:
    XGL_INDEX_TYPE  m_indexType;
};

class XglTextureObj
{
public:
    XglTextureObj(XglDevice *device);
    ~XglTextureObj();
    void ChangeColors(uint32_t color1, uint32_t color2);
    XGL_IMAGE                  m_texture;
    XGL_IMAGE_VIEW_ATTACH_INFO m_textureViewInfo;
    XGL_GPU_MEMORY             m_textureMem;

protected:
    XglDevice                 *m_device;
    XGL_IMAGE_VIEW             m_textureView;
    int                        m_texHeight;
    int                        m_texWidth;
    int                        m_rowPitch;

};

class XglSamplerObj
{
public:
    XglSamplerObj(XglDevice *device);
    ~XglSamplerObj();
    XGL_SAMPLER m_sampler;

protected:
     XGL_SAMPLER_CREATE_INFO m_samplerCreateInfo;
     XglDevice *m_device;

};

class XglDescriptorSetObj
{
public:
    XglDescriptorSetObj(XglDevice *device);
    ~XglDescriptorSetObj();
    void AttachMemoryView(XglConstantBufferObj* constantBuffer);
    void AttachSampler( XglSamplerObj* sampler);
    void AttachImageView( XglTextureObj* texture);
    void BindCommandBuffer(XGL_CMD_BUFFER commandBuffer);
    XGL_DESCRIPTOR_SLOT_INFO * GetSlotInfo(vector<int>slots, vector<XGL_DESCRIPTOR_SET_SLOT_TYPE>types, vector<XGL_OBJECT>objs );

protected:
    XGL_DESCRIPTOR_SET_CREATE_INFO       m_descriptorInfo;
    XGL_DESCRIPTOR_SET                   m_rsrcDescSet;
    XGL_GPU_MEMORY                       m_descriptor_set_mem;
    XglDevice                           *m_device;
    XGL_DESCRIPTOR_SLOT_INFO            *m_slotInfo;
    int                                  m_nextSlot;
    vector<int>                          m_memorySlots;
    vector<XGL_MEMORY_VIEW_ATTACH_INFO*> m_memoryViews;
    vector<int>                          m_samplerSlots;
    vector<XGL_SAMPLER*>                 m_samplers;
    vector<int>                          m_imageSlots;
    vector<XGL_IMAGE_VIEW_ATTACH_INFO*>  m_imageViews;
};


class XglShaderObj
{
public:
    XglShaderObj(XglDevice *device, const char * shaderText, XGL_PIPELINE_SHADER_STAGE stage, XglRenderFramework *framework);
    ~XglShaderObj();
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* GetStageCreateInfo(XglDescriptorSetObj *descriptorSet);
    void BindShaderEntitySlotToMemory(int slot, XGL_DESCRIPTOR_SET_SLOT_TYPE type, XglConstantBufferObj *constantBuffer);
    void BindShaderEntitySlotToImage(int slot, XGL_DESCRIPTOR_SET_SLOT_TYPE type, XglTextureObj *texture);
    void BindShaderEntitySlotToSampler(int slot, XglSamplerObj *sampler);

protected:
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO stage_info;
    XGL_SHADER m_shader;
    XGL_PIPELINE_SHADER_STAGE m_stage;
    XglDevice *m_device;
    vector<int>    m_memSlots;
    vector<XGL_DESCRIPTOR_SET_SLOT_TYPE> m_memTypes;
    vector<XGL_OBJECT> m_memObjs;
    vector<int>    m_samplerSlots;
    vector<XGL_DESCRIPTOR_SET_SLOT_TYPE> m_samplerTypes;
    vector<XGL_OBJECT> m_samplerObjs;
    vector<int>    m_imageSlots;
    vector<XGL_DESCRIPTOR_SET_SLOT_TYPE> m_imageTypes;
    vector<XGL_OBJECT> m_imageObjs;

};

class XglPipelineObj
{
public:
    XglPipelineObj(XglDevice *device);
    ~XglPipelineObj();
    void BindPipelineCommandBuffer(XGL_CMD_BUFFER m_cmdBuffer, XglDescriptorSetObj *descriptorSet);
    void AddShader(XglShaderObj* shaderObj);
    void AddVertexInputAttribs(XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* vi_attrib, int count);
    void AddVertexInputBindings(XGL_VERTEX_INPUT_BINDING_DESCRIPTION* vi_binding, int count);
    void AddVertexDataBuffer(XglConstantBufferObj* vertexDataBuffer, int binding);
    void SetColorAttachment(XGL_UINT binding, const XGL_PIPELINE_CB_ATTACHMENT_STATE *att);

protected:
    XGL_PIPELINE m_pipeline;
    XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO m_vi_state;
    XGL_PIPELINE_IA_STATE_CREATE_INFO m_ia_state;
    XGL_PIPELINE_RS_STATE_CREATE_INFO m_rs_state;
    XGL_PIPELINE_CB_STATE m_cb_state;
    XGL_PIPELINE_DB_STATE_CREATE_INFO m_db_state;
    XGL_GPU_MEMORY m_pipe_mem;
    XglDevice *m_device;
    vector<XglShaderObj*> m_shaderObjs;
    vector<XglConstantBufferObj*> m_vertexBufferObjs;
    vector<int> m_vertexBufferBindings;
    int m_vertexBufferCount;

};
class XglMemoryRefManager{
public:
    XglMemoryRefManager();
    void AddMemoryRef(XglConstantBufferObj* constantBuffer);
    void AddMemoryRef(XglTextureObj *texture);
    XGL_MEMORY_REF* GetMemoryRefList();
    int GetNumRefs();

protected:
    int m_numRefs;
    vector<XGL_GPU_MEMORY*> m_bufferObjs;

};


#endif // XGLRENDERFRAMEWORK_H
