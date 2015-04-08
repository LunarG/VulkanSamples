/*
 * Vulkan Tests
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

#ifndef VKRENDERFRAMEWORK_H
#define VKRENDERFRAMEWORK_H

#include "vktestframework.h"


class XglDevice : public vk_testing::Device
{
public:
    XglDevice(uint32_t id, VK_PHYSICAL_GPU obj);

    VK_DEVICE device() { return obj(); }
    void get_device_queue();

    uint32_t id;
    VK_PHYSICAL_GPU_PROPERTIES props;
    const VK_PHYSICAL_GPU_QUEUE_PROPERTIES *queue_props;

    VK_QUEUE m_queue;
};

class XglMemoryRefManager
{
public:
    void AddMemoryRefs(vk_testing::Object &vkObject);
    void AddMemoryRefs(vector<VK_GPU_MEMORY> mem);
    void EmitAddMemoryRefs(VK_QUEUE queue);
    void EmitRemoveMemoryRefs(VK_QUEUE queue);
    vector<VK_GPU_MEMORY> mem_refs() const;

protected:
    vector<VK_GPU_MEMORY>      mem_refs_;

};

class XglDepthStencilObj : public vk_testing::Image
{
public:
    XglDepthStencilObj();
    void Init(XglDevice *device, int32_t width, int32_t height);
    bool Initialized();
    VK_DEPTH_STENCIL_BIND_INFO* BindInfo();

protected:
    XglDevice                         *m_device;
    bool                               m_initialized;
    vk_testing::DepthStencilView      m_depthStencilView;
    VK_FORMAT                         m_depth_stencil_fmt;
    VK_DEPTH_STENCIL_BIND_INFO        m_depthStencilBindInfo;
};

class XglRenderFramework : public XglTestFramework
{
public:
    XglRenderFramework();
    ~XglRenderFramework();

    VK_DEVICE device() {return m_device->device();}
    VK_PHYSICAL_GPU gpu() {return objs[0];}
    VK_RENDER_PASS renderPass() {return m_renderPass;}
    VK_FRAMEBUFFER framebuffer() {return m_framebuffer;}
    void InitViewport(float width, float height);
    void InitViewport();
    void InitRenderTarget();
    void InitRenderTarget(uint32_t targets);
    void InitRenderTarget(VK_DEPTH_STENCIL_BIND_INFO *dsBinding);
    void InitRenderTarget(uint32_t targets, VK_DEPTH_STENCIL_BIND_INFO *dsBinding);
    void InitFramework();
    void ShutdownFramework();
    void InitState();


protected:
    VK_APPLICATION_INFO                    app_info;
    VK_INSTANCE                            inst;
    VK_PHYSICAL_GPU                        objs[VK_MAX_PHYSICAL_GPUS];
    uint32_t                                gpu_count;
    XglDevice                              *m_device;
    VK_CMD_BUFFER                          m_cmdBuffer;
    VK_RENDER_PASS                         m_renderPass;
    VK_FRAMEBUFFER                         m_framebuffer;
    VK_DYNAMIC_RS_STATE_OBJECT             m_stateRaster;
    VK_DYNAMIC_CB_STATE_OBJECT             m_colorBlend;
    VK_DYNAMIC_VP_STATE_OBJECT             m_stateViewport;
    VK_DYNAMIC_DS_STATE_OBJECT             m_stateDepthStencil;
    vector<XglImage*>                       m_renderTargets;
    float                                   m_width, m_height;
    VK_FORMAT                              m_render_target_fmt;
    VK_FORMAT                              m_depth_stencil_fmt;
    VK_COLOR_ATTACHMENT_BIND_INFO          m_colorBindings[8];
    VK_CLEAR_COLOR                         m_clear_color;
    float                                   m_depth_clear_color;
    uint32_t                                m_stencil_clear_color;
    XglDepthStencilObj                     *m_depthStencil;
    XglMemoryRefManager                     m_mem_ref_mgr;

    /*
     * SetUp and TearDown are called by the Google Test framework
     * to initialize a test framework based on this class.
     */
    virtual void SetUp() {
        this->app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = "base";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = VK_API_VERSION;

        InitFramework();
    }

    virtual void TearDown() {
        ShutdownFramework();
    }
};

class XglDescriptorSetObj;
class XglIndexBufferObj;
class XglConstantBufferObj;
class XglPipelineObj;
class XglDescriptorSetObj;

class XglCommandBufferObj : public vk_testing::CmdBuffer
{
public:
    XglCommandBufferObj(XglDevice *device);
    VK_CMD_BUFFER GetBufferHandle();
    VK_RESULT BeginCommandBuffer();
    VK_RESULT BeginCommandBuffer(VK_CMD_BUFFER_BEGIN_INFO *pInfo);
    VK_RESULT BeginCommandBuffer(VK_RENDER_PASS renderpass_obj, VK_FRAMEBUFFER framebuffer_obj);
    VK_RESULT EndCommandBuffer();
    void PipelineBarrier(VK_PIPELINE_BARRIER *barrierPtr);
    void AddRenderTarget(XglImage *renderTarget);
    void AddDepthStencil();
    void ClearAllBuffers(VK_CLEAR_COLOR clear_color, float depth_clear_color, uint32_t stencil_clear_color, XglDepthStencilObj *depthStencilObj);
    void PrepareAttachments();
    void AddMemoryRefs(vk_testing::Object &vkObject);
    void AddMemoryRefs(uint32_t ref_count, const VK_GPU_MEMORY *mem);
    void AddMemoryRefs(vector<vk_testing::Object *> images);
    void BindPipeline(XglPipelineObj &pipeline);
    void BindDescriptorSet(XglDescriptorSetObj &descriptorSet);
    void BindVertexBuffer(XglConstantBufferObj *vertexBuffer, uint32_t offset, uint32_t binding);
    void BindIndexBuffer(XglIndexBufferObj *indexBuffer, uint32_t offset);
    void BindStateObject(VK_STATE_BIND_POINT stateBindPoint, VK_DYNAMIC_STATE_OBJECT stateObject);
    void BeginRenderPass(VK_RENDER_PASS renderpass, VK_FRAMEBUFFER framebuffer);
    void EndRenderPass(VK_RENDER_PASS renderpass);
    void Draw(uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount);
    void DrawIndexed(uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount);
    void QueueCommandBuffer();
    void QueueCommandBuffer(VK_FENCE fence);

    XglMemoryRefManager             mem_ref_mgr;

protected:
    XglDevice                      *m_device;
    vector<XglImage*>               m_renderTargets;
};

class XglConstantBufferObj : public vk_testing::Buffer
{
public:
    XglConstantBufferObj(XglDevice *device);
    XglConstantBufferObj(XglDevice *device, int constantCount, int constantSize, const void* data);
    ~XglConstantBufferObj();
    void BufferMemoryBarrier(
        VK_FLAGS outputMask =
            VK_MEMORY_OUTPUT_CPU_WRITE_BIT |
            VK_MEMORY_OUTPUT_SHADER_WRITE_BIT |
            VK_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT |
            VK_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_MEMORY_OUTPUT_COPY_BIT,
        VK_FLAGS inputMask =
            VK_MEMORY_INPUT_CPU_READ_BIT |
            VK_MEMORY_INPUT_INDIRECT_COMMAND_BIT |
            VK_MEMORY_INPUT_INDEX_FETCH_BIT |
            VK_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT |
            VK_MEMORY_INPUT_UNIFORM_READ_BIT |
            VK_MEMORY_INPUT_SHADER_READ_BIT |
            VK_MEMORY_INPUT_COLOR_ATTACHMENT_BIT |
            VK_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_MEMORY_INPUT_COPY_BIT);

    void Bind(VK_CMD_BUFFER cmdBuffer, VK_GPU_SIZE offset, uint32_t binding);

    VK_BUFFER_VIEW_ATTACH_INFO     m_bufferViewInfo;

protected:
    XglDevice                      *m_device;
    vk_testing::BufferView         m_bufferView;
    int                             m_numVertices;
    int                             m_stride;
    XglCommandBufferObj             *m_commandBuffer;
    vk_testing::Fence              m_fence;
};

class XglIndexBufferObj : public XglConstantBufferObj
{
public:
    XglIndexBufferObj(XglDevice *device);
    void CreateAndInitBuffer(int numIndexes, VK_INDEX_TYPE dataFormat, const void* data);
    void Bind(VK_CMD_BUFFER cmdBuffer, VK_GPU_SIZE offset);
    VK_INDEX_TYPE GetIndexType();

protected:
    VK_INDEX_TYPE  m_indexType;
};

class XglImage : public vk_testing::Image
{
public:
    XglImage(XglDevice *dev);
    bool IsCompatible(VK_FLAGS usage, VK_FLAGS features);

public:
    void init(uint32_t w, uint32_t h,
              VK_FORMAT fmt, VK_FLAGS usage,
              VK_IMAGE_TILING tiling=VK_LINEAR_TILING);

    //    void clear( CommandBuffer*, uint32_t[4] );

    void layout( VK_IMAGE_LAYOUT layout )
    {
        m_imageInfo.layout = layout;
    }

    VK_GPU_MEMORY memory() const
    {
        const std::vector<VK_GPU_MEMORY> mems = memories();
        return mems.empty() ? VK_NULL_HANDLE : mems[0];
    }

    void ImageMemoryBarrier(XglCommandBufferObj *cmd,
                            VK_IMAGE_ASPECT aspect,
                            VK_FLAGS output_mask,
                            VK_FLAGS input_mask,
                            VK_IMAGE_LAYOUT image_layout);

    VK_RESULT CopyImage(XglImage &src_image);

    VK_IMAGE image() const
    {
        return obj();
    }

    VK_COLOR_ATTACHMENT_VIEW targetView()
    {
        if (!m_targetView.initialized())
        {
            VK_COLOR_ATTACHMENT_VIEW_CREATE_INFO createView = {
                VK_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
                VK_NULL_HANDLE,
                obj(),
                VK_FMT_B8G8R8A8_UNORM,
                0,
                0,
                1
            };
            m_targetView.init(*m_device, createView);
        }
        return m_targetView.obj();
    }

    void SetLayout(XglCommandBufferObj *cmd_buf, VK_IMAGE_ASPECT aspect, VK_IMAGE_LAYOUT image_layout);
    void SetLayout(VK_IMAGE_ASPECT aspect, VK_IMAGE_LAYOUT image_layout);

    VK_IMAGE_LAYOUT layout() const
    {
        return ( VK_IMAGE_LAYOUT )m_imageInfo.layout;
    }
    uint32_t width() const
    {
        return extent().width;
    }
    uint32_t height() const
    {
        return extent().height;
    }
    XglDevice* device() const
    {
        return m_device;
    }

    VK_RESULT MapMemory(void** ptr);
    VK_RESULT UnmapMemory();

protected:
    XglDevice *m_device;

    vk_testing::ColorAttachmentView m_targetView;
    VK_IMAGE_VIEW_ATTACH_INFO   m_imageInfo;
};

class XglTextureObj : public XglImage
{
public:
    XglTextureObj(XglDevice *device, uint32_t *colors = NULL);
    VK_IMAGE_VIEW_ATTACH_INFO m_textureViewInfo;


protected:
    XglDevice                 *m_device;
    vk_testing::ImageView     m_textureView;
    VK_GPU_SIZE               m_rowPitch;
};

class XglSamplerObj : public vk_testing::Sampler
{
public:
    XglSamplerObj(XglDevice *device);

protected:
     XglDevice *m_device;

};

class XglDescriptorSetObj : public vk_testing::DescriptorPool
{
public:
    XglDescriptorSetObj(XglDevice *device);
    ~XglDescriptorSetObj();

    int AppendDummy();
    int AppendBuffer(VK_DESCRIPTOR_TYPE type, XglConstantBufferObj &constantBuffer);
    int AppendSamplerTexture(XglSamplerObj* sampler, XglTextureObj* texture);
    void CreateVKDescriptorSet(XglCommandBufferObj *cmdBuffer);

    VK_DESCRIPTOR_SET GetDescriptorSetHandle() const;
    VK_DESCRIPTOR_SET_LAYOUT_CHAIN GetLayoutChain() const;

    XglMemoryRefManager                  mem_ref_mgr;

protected:
    XglDevice                           *m_device;
    vector<VK_DESCRIPTOR_TYPE_COUNT>    m_type_counts;
    int                                  m_nextSlot;

    vector<VK_UPDATE_BUFFERS>           m_updateBuffers;

    vector<VK_SAMPLER_IMAGE_VIEW_INFO>  m_samplerTextureInfo;
    vector<VK_UPDATE_SAMPLER_TEXTURES>  m_updateSamplerTextures;

    vk_testing::DescriptorSetLayout     m_layout;
    vk_testing::DescriptorSetLayoutChain m_layout_chain;
    vk_testing::DescriptorSet          *m_set;
};


class XglShaderObj : public vk_testing::Shader
{
public:
    XglShaderObj(XglDevice *device, const char * shaderText, VK_PIPELINE_SHADER_STAGE stage, XglRenderFramework *framework);
    VK_PIPELINE_SHADER_STAGE_CREATE_INFO* GetStageCreateInfo();

protected:
    VK_PIPELINE_SHADER_STAGE_CREATE_INFO stage_info;
    VK_PIPELINE_SHADER_STAGE m_stage;
    XglDevice *m_device;

};

class XglPipelineObj : public vk_testing::Pipeline
{
public:
    XglPipelineObj(XglDevice *device);
    void AddShader(XglShaderObj* shaderObj);
    void AddVertexInputAttribs(VK_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* vi_attrib, int count);
    void AddVertexInputBindings(VK_VERTEX_INPUT_BINDING_DESCRIPTION* vi_binding, int count);
    void AddVertexDataBuffer(XglConstantBufferObj* vertexDataBuffer, int binding);
    void AddColorAttachment(uint32_t binding, const VK_PIPELINE_CB_ATTACHMENT_STATE *att);
    void SetDepthStencil(VK_PIPELINE_DS_STATE_CREATE_INFO *);
    void CreateVKPipeline(XglDescriptorSetObj &descriptorSet);

protected:
    VK_PIPELINE_VERTEX_INPUT_CREATE_INFO m_vi_state;
    VK_PIPELINE_IA_STATE_CREATE_INFO m_ia_state;
    VK_PIPELINE_RS_STATE_CREATE_INFO m_rs_state;
    VK_PIPELINE_CB_STATE_CREATE_INFO m_cb_state;
    VK_PIPELINE_DS_STATE_CREATE_INFO m_ds_state;
    VK_PIPELINE_MS_STATE_CREATE_INFO m_ms_state;
    XglDevice *m_device;
    vector<XglShaderObj*> m_shaderObjs;
    vector<XglConstantBufferObj*> m_vertexBufferObjs;
    vector<int> m_vertexBufferBindings;
    vector<VK_PIPELINE_CB_ATTACHMENT_STATE> m_colorAttachments;
    int m_vertexBufferCount;

};

#endif // VKRENDERFRAMEWORK_H
