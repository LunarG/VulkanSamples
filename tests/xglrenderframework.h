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

class XglDevice : public xgl_testing::Device
{
public:
    XglDevice(uint32_t id, XGL_PHYSICAL_GPU obj);

    XGL_DEVICE device() { return obj(); }
    void get_device_queue();

    uint32_t id;
    XGL_PHYSICAL_GPU_PROPERTIES props;
    const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *queue_props;

    XGL_QUEUE m_queue;
};


class XglRenderFramework : public XglTestFramework
{
public:
    XglRenderFramework();
    ~XglRenderFramework();

    XGL_DEVICE device() {return m_device->device();}
    XGL_PHYSICAL_GPU gpu() {return objs[0];}
    XGL_RENDER_PASS renderPass() {return m_renderPass;}
    void InitViewport(float width, float height);
    void InitViewport();
    void InitRenderTarget();
    void InitFramework();
    void ShutdownFramework();
    void InitState();


protected:
    XGL_APPLICATION_INFO                    app_info;
    XGL_INSTANCE                            inst;
    XGL_PHYSICAL_GPU                        objs[XGL_MAX_PHYSICAL_GPUS];
    uint32_t                                gpu_count;
    XglDevice                              *m_device;
    XGL_CMD_BUFFER                          m_cmdBuffer;
    XGL_RENDER_PASS                         m_renderPass;
    XGL_MEMORY_REF                          m_memRefs[5];
    XGL_DYNAMIC_RS_STATE_OBJECT             m_stateRaster;
    XGL_DYNAMIC_CB_STATE_OBJECT             m_colorBlend;
    XGL_DYNAMIC_VP_STATE_OBJECT             m_stateViewport;
    XGL_DYNAMIC_DS_STATE_OBJECT             m_stateDepthStencil;
    vector<XglImage*>                       m_renderTargets;
    uint32_t                                m_renderTargetCount;
    float                                   m_width, m_height;
    XGL_FORMAT                              m_render_target_fmt;
    XGL_COLOR_ATTACHMENT_BIND_INFO          m_colorBindings[8];
    XGL_DEPTH_STENCIL_BIND_INFO             m_depthStencilBinding;

    /*
     * SetUp and TearDown are called by the Google Test framework
     * to initialize a test framework based on this class.
     */
    virtual void SetUp() {
        this->app_info.sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = "base";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = XGL_API_VERSION;

        InitFramework();
    }

    virtual void TearDown() {
        ShutdownFramework();
    }
};

class XglIndexBufferObj;
class XglConstantBufferObj;

class XglCommandBufferObj : public xgl_testing::CmdBuffer
{
public:
    XglCommandBufferObj(XglDevice *device);
    XGL_CMD_BUFFER GetBufferHandle();
    XGL_RESULT BeginCommandBuffer(XGL_CMD_BUFFER_BEGIN_INFO *pInfo);
    XGL_RESULT BeginCommandBuffer(XGL_RENDER_PASS renderpass_obj);
    XGL_RESULT BeginCommandBuffer();
    XGL_RESULT EndCommandBuffer();
    void PipelineBarrier(XGL_PIPELINE_BARRIER *barrierPtr);
    void AddRenderTarget(XglImage *renderTarget);
    void AddDepthStencil();
    void ClearAllBuffers();
    void ClearAllBuffers(XGL_DEPTH_STENCIL_BIND_INFO *depthStencilBinding, XGL_IMAGE depthStencilImage);
    void PrepareAttachments();
    void BindPipeline(XGL_PIPELINE pipeline);
    void BindDescriptorSet(XGL_DESCRIPTOR_SET descriptorSet);
    void BindVertexBuffer(XglConstantBufferObj *vertexBuffer, uint32_t offset, uint32_t binding);
    void BindIndexBuffer(XglIndexBufferObj *indexBuffer, uint32_t offset);
    void BindStateObject(XGL_STATE_BIND_POINT stateBindPoint, XGL_DYNAMIC_STATE_OBJECT stateObject);
    void Draw(uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount);
    void DrawIndexed(uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount);
    void QueueCommandBuffer(XGL_MEMORY_REF *memRefs, uint32_t numMemRefs);

protected:
    XglDevice                      *m_device;
    vector<XglImage*>               m_renderTargets;
    uint32_t                        m_renderTargetCount;

};

class XglConstantBufferObj : public xgl_testing::Buffer
{
public:
    XglConstantBufferObj(XglDevice *device);
    XglConstantBufferObj(XglDevice *device, int constantCount, int constantSize, const void* data);
    void BufferMemoryBarrier(
        XGL_FLAGS outputMask =
            XGL_MEMORY_OUTPUT_CPU_WRITE_BIT |
            XGL_MEMORY_OUTPUT_SHADER_WRITE_BIT |
            XGL_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT |
            XGL_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
            XGL_MEMORY_OUTPUT_COPY_BIT,
        XGL_FLAGS inputMask =
            XGL_MEMORY_INPUT_CPU_READ_BIT |
            XGL_MEMORY_INPUT_INDIRECT_COMMAND_BIT |
            XGL_MEMORY_INPUT_INDEX_FETCH_BIT |
            XGL_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT |
            XGL_MEMORY_INPUT_UNIFORM_READ_BIT |
            XGL_MEMORY_INPUT_SHADER_READ_BIT |
            XGL_MEMORY_INPUT_COLOR_ATTACHMENT_BIT |
            XGL_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
            XGL_MEMORY_INPUT_COPY_BIT);

    void Bind(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_SIZE offset, uint32_t binding);

    XGL_BUFFER_VIEW_ATTACH_INFO     m_bufferViewInfo;

protected:
    XglDevice                      *m_device;
    xgl_testing::BufferView         m_bufferView;
    int                             m_numVertices;
    int                             m_stride;
    XglCommandBufferObj             *m_commandBuffer;
    xgl_testing::Fence              m_fence;
};

class XglIndexBufferObj : public XglConstantBufferObj
{
public:
    XglIndexBufferObj(XglDevice *device);
    void CreateAndInitBuffer(int numIndexes, XGL_INDEX_TYPE dataFormat, const void* data);
    void Bind(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_SIZE offset);
    XGL_INDEX_TYPE GetIndexType();

protected:
    XGL_INDEX_TYPE  m_indexType;
};

class XglImage : public xgl_testing::Image
{
public:
    XglImage(XglDevice *dev);

public:
    void init( uint32_t w, uint32_t h,
                     XGL_FORMAT fmt, XGL_FLAGS usage,
                     XGL_IMAGE_TILING tiling=XGL_LINEAR_TILING);

    //    void clear( CommandBuffer*, uint32_t[4] );
    //    void prepare( CommandBuffer*, XGL_IMAGE_STATE );

    void layout( XGL_IMAGE_LAYOUT layout )
    {
        m_imageInfo.layout = layout;
    }
    XGL_GPU_MEMORY memory() const
    {
        const std::vector<XGL_GPU_MEMORY> mems = memories();
        return mems.empty() ? XGL_NULL_HANDLE : mems[0];
    }


    XGL_IMAGE image() const
    {
        return obj();
    }
    XGL_COLOR_ATTACHMENT_VIEW targetView()const
    {
        return m_targetView.obj();
    }

    XGL_IMAGE_LAYOUT layout() const
    {
        return ( XGL_IMAGE_LAYOUT )m_imageInfo.layout;
    }
    uint32_t width() const
    {
        return extent().width;
    }
    uint32_t height() const
    {
        return extent().height;
    }

    XGL_RESULT MapMemory(void** ptr);
    XGL_RESULT UnmapMemory();

protected:
    XglDevice *m_device;

    xgl_testing::ColorAttachmentView m_targetView;
    XGL_IMAGE_VIEW_ATTACH_INFO   m_imageInfo;
};

class XglTextureObj : public xgl_testing::Image
{
public:
    XglTextureObj(XglDevice *device);
    void ChangeColors(uint32_t color1, uint32_t color2);
    XGL_IMAGE_VIEW_ATTACH_INFO m_textureViewInfo;

protected:
    XglDevice                 *m_device;
    xgl_testing::ImageView     m_textureView;
    XGL_GPU_SIZE               m_rowPitch;
};

class XglSamplerObj : public xgl_testing::Sampler
{
public:
    XglSamplerObj(XglDevice *device);

protected:
     XglDevice *m_device;

};

class XglDescriptorSetObj : public xgl_testing::DescriptorRegion
{
public:
    XglDescriptorSetObj(XglDevice *device);
    ~XglDescriptorSetObj();

    int AppendDummy();
    int AppendBuffer(XGL_DESCRIPTOR_TYPE type, XglConstantBufferObj* constantBuffer);
    int AppendSamplerTexture(XglSamplerObj* sampler, XglTextureObj* texture);
    void CreateXGLDescriptorSet(XglCommandBufferObj *cmdBuffer);

    XGL_DESCRIPTOR_SET GetDescriptorSetHandle();
    XGL_DESCRIPTOR_SET_LAYOUT GetLayout();

protected:
    XglDevice                           *m_device;
    vector<XGL_DESCRIPTOR_TYPE_COUNT>    m_type_counts;
    int                                  m_nextSlot;

    vector<const XGL_BUFFER_VIEW_ATTACH_INFO *> m_bufferInfo;
    vector<XGL_UPDATE_BUFFERS>           m_updateBuffers;

    vector<XGL_SAMPLER_IMAGE_VIEW_INFO>  m_samplerTextureInfo;
    vector<XGL_UPDATE_SAMPLER_TEXTURES>  m_updateSamplerTextures;

    xgl_testing::DescriptorSetLayout     m_layout;
    xgl_testing::DescriptorSet          *m_set;
};


class XglShaderObj : public xgl_testing::Shader
{
public:
    XglShaderObj(XglDevice *device, const char * shaderText, XGL_PIPELINE_SHADER_STAGE stage, XglRenderFramework *framework);
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* GetStageCreateInfo();

protected:
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO stage_info;
    XGL_PIPELINE_SHADER_STAGE m_stage;
    XglDevice *m_device;

};

class XglPipelineObj : public xgl_testing::Pipeline
{
public:
    XglPipelineObj(XglDevice *device);
    void BindPipelineCommandBuffer(XGL_CMD_BUFFER m_cmdBuffer, XglDescriptorSetObj *descriptorSet);
    void AddShader(XglShaderObj* shaderObj);
    void AddVertexInputAttribs(XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* vi_attrib, int count);
    void AddVertexInputBindings(XGL_VERTEX_INPUT_BINDING_DESCRIPTION* vi_binding, int count);
    void AddVertexDataBuffer(XglConstantBufferObj* vertexDataBuffer, int binding);
    void AddColorAttachment(uint32_t binding, const XGL_PIPELINE_CB_ATTACHMENT_STATE *att);
    void SetDepthStencil(XGL_PIPELINE_DS_STATE_CREATE_INFO *);
    void CreateXGLPipeline(XglDescriptorSetObj *descriptorSet);
    XGL_PIPELINE GetPipelineHandle();

protected:
    XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO m_vi_state;
    XGL_PIPELINE_IA_STATE_CREATE_INFO m_ia_state;
    XGL_PIPELINE_RS_STATE_CREATE_INFO m_rs_state;
    XGL_PIPELINE_CB_STATE_CREATE_INFO m_cb_state;
    XGL_PIPELINE_DS_STATE_CREATE_INFO m_ds_state;
    XGL_PIPELINE_MS_STATE_CREATE_INFO m_ms_state;
    XglDevice *m_device;
    vector<XglShaderObj*> m_shaderObjs;
    vector<XglConstantBufferObj*> m_vertexBufferObjs;
    vector<int> m_vertexBufferBindings;
    vector<XGL_PIPELINE_CB_ATTACHMENT_STATE> m_colorAttachments;
    int m_vertexBufferCount;

};
class XglMemoryRefManager{
public:
    XglMemoryRefManager();
    void AddMemoryRef(XglConstantBufferObj* constantBuffer);
    void AddMemoryRef(XglTextureObj *texture);
    void AddMemoryRef(XGL_GPU_MEMORY *mem, uint32_t refCount);
    void AddRTMemoryRefs(vector<XglImage *>images, uint32_t rtCount);
    XGL_MEMORY_REF* GetMemoryRefList();
    int GetNumRefs();

protected:
    int m_numRefs;
    vector<XGL_GPU_MEMORY> m_bufferObjs;

};


#endif // XGLRENDERFRAMEWORK_H
