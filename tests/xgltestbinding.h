// XGL tests
//
// Copyright (C) 2014 LunarG, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef XGLTESTBINDING_H
#define XGLTESTBINDING_H

#include <vector>

#include "xgl.h"

namespace xgl_testing {

typedef void (*ErrorCallback)(const char *expr, const char *file, unsigned int line, const char *function);
void set_error_callback(ErrorCallback callback);

class PhysicalGpu;
class BaseObject;
class Object;
class DynamicStateObject;
class Device;
class Queue;
class GpuMemory;
class Fence;
class Semaphore;
class Event;
class QueryPool;
class Buffer;
class BufferView;
class Image;
class ImageView;
class ColorAttachmentView;
class DepthStencilView;
class Shader;
class Pipeline;
class PipelineDelta;
class Sampler;
class DescriptorSetLayout;
class DescriptorSetLayoutChain;
class DescriptorSetPool;
class DescriptorSet;
class DynamicVpStateObject;
class DynamicRsStateObject;
class DynamicMsaaStateObject;
class DynamicCbStateObject;
class DynamicDsStateObject;
class CmdBuffer;

class PhysicalGpu {
public:
    explicit PhysicalGpu(XGL_PHYSICAL_GPU gpu) : gpu_(gpu) {}

    const XGL_PHYSICAL_GPU &obj() const { return gpu_; }

    // xglGetGpuInfo()
    XGL_PHYSICAL_GPU_PROPERTIES properties() const;
    XGL_PHYSICAL_GPU_PERFORMANCE performance() const;
    XGL_PHYSICAL_GPU_MEMORY_PROPERTIES memory_properties() const;
    std::vector<XGL_PHYSICAL_GPU_QUEUE_PROPERTIES> queue_properties() const;

    // xglGetProcAddr()
    void *get_proc(const char *name) const { return xglGetProcAddr(gpu_, name); }

    // xglGetExtensionSupport()
    bool has_extension(const char *ext) const { return (xglGetExtensionSupport(gpu_, ext) == XGL_SUCCESS); }
    std::vector<const char *> extensions() const;

    // xglEnumerateLayers()
    std::vector<const char *> layers(std::vector<char> &buf) const;

    // xglGetMultiGpuCompatibility()
    XGL_GPU_COMPATIBILITY_INFO compatibility(const PhysicalGpu &other) const;

private:
    XGL_PHYSICAL_GPU gpu_;
};

class BaseObject {
public:
    const XGL_BASE_OBJECT &obj() const { return obj_; }
    bool initialized() const { return (obj_ != XGL_NULL_HANDLE); }

    // xglGetObjectInfo()
    uint32_t memory_allocation_count() const;
    std::vector<XGL_MEMORY_REQUIREMENTS> memory_requirements() const;

protected:
    explicit BaseObject() : obj_(XGL_NULL_HANDLE), own_obj_(false) {}
    explicit BaseObject(XGL_BASE_OBJECT obj) : obj_(XGL_NULL_HANDLE), own_obj_(false) { init(obj); }

    void init(XGL_BASE_OBJECT obj, bool own);
    void init(XGL_BASE_OBJECT obj) { init(obj, true); }

    void reinit(XGL_BASE_OBJECT obj, bool own);
    void reinit(XGL_BASE_OBJECT obj) { reinit(obj, true); }

    bool own() const { return own_obj_; }

private:
    // base objects are non-copyable
    BaseObject(const BaseObject &);
    BaseObject &operator=(const BaseObject &);

    XGL_BASE_OBJECT obj_;
    bool own_obj_;
};

class Object : public BaseObject {
public:
    const XGL_OBJECT &obj() const { return reinterpret_cast<const XGL_OBJECT &>(BaseObject::obj()); }

    // xglBindObjectMemory()
    void bind_memory(uint32_t alloc_idx, const GpuMemory &mem, XGL_GPU_SIZE mem_offset);
    void unbind_memory(uint32_t alloc_idx);
    void unbind_memory();

    // xglBindObjectMemoryRange()
    void bind_memory(uint32_t alloc_idx, XGL_GPU_SIZE offset, XGL_GPU_SIZE size,
                     const GpuMemory &mem, XGL_GPU_SIZE mem_offset);

    // Unless an object is initialized with init_no_mem(), memories are
    // automatically allocated and bound.  These methods can be used to get
    // the memories (for xglQueueAddMemReference), or to map/unmap the primary memory.
    std::vector<XGL_GPU_MEMORY> memories() const;

    const void *map(XGL_FLAGS flags) const;
          void *map(XGL_FLAGS flags);
    const void *map() const { return map(0); }
          void *map()       { return map(0); }

    void unmap() const;

protected:
    explicit Object() : mem_alloc_count_(0), internal_mems_(NULL), primary_mem_(NULL) {}
    explicit Object(XGL_OBJECT obj) : mem_alloc_count_(0), internal_mems_(NULL), primary_mem_(NULL) { init(obj); }
    ~Object() { cleanup(); }

    void init(XGL_OBJECT obj, bool own);
    void init(XGL_OBJECT obj) { init(obj, true); }

    void reinit(XGL_OBJECT obj, bool own);
    void reinit(XGL_OBJECT obj) { init(obj, true); }

    // allocate and bind internal memories
    void alloc_memory(const Device &dev, bool for_linear_img, bool for_img);
    void alloc_memory(const Device &dev) { alloc_memory(dev, false, false); }
    void alloc_memory(const std::vector<XGL_GPU_MEMORY> &mems);

private:
    void cleanup();

    uint32_t mem_alloc_count_;
    GpuMemory *internal_mems_;
    GpuMemory *primary_mem_;
};

class DynamicStateObject : public Object {
public:
    const XGL_DYNAMIC_STATE_OBJECT &obj() const { return reinterpret_cast<const XGL_DYNAMIC_STATE_OBJECT &>(Object::obj()); }

protected:
    explicit DynamicStateObject() {}
    explicit DynamicStateObject(XGL_DYNAMIC_STATE_OBJECT obj) : Object(obj) {}
};

template<typename T, class C>
class DerivedObject : public C {
public:
    const T &obj() const { return reinterpret_cast<const T &>(C::obj()); }

protected:
    typedef T obj_type;
    typedef C base_type;

    explicit DerivedObject() {}
    explicit DerivedObject(T obj) : C(obj) {}
};

class Device : public DerivedObject<XGL_DEVICE, BaseObject> {
public:
    explicit Device(XGL_PHYSICAL_GPU gpu) : gpu_(gpu) {}
    ~Device();

    // xglCreateDevice()
    void init(const XGL_DEVICE_CREATE_INFO &info);
    void init(bool enable_layers); // all queues, all extensions, etc
    void init() { init(false); };

    const PhysicalGpu &gpu() const { return gpu_; }

    // xglGetDeviceQueue()
    const std::vector<Queue *> &graphics_queues() { return queues_[GRAPHICS]; }
    const std::vector<Queue *> &compute_queues() { return queues_[COMPUTE]; }
    const std::vector<Queue *> &dma_queues() { return queues_[DMA]; }
    uint32_t graphics_queue_node_index_;

    struct Format {
        XGL_FORMAT format;
        XGL_IMAGE_TILING tiling;
        XGL_FLAGS features;
    };
    // xglGetFormatInfo()
    XGL_FORMAT_PROPERTIES format_properties(XGL_FORMAT format);
    const std::vector<Format> &formats() const { return formats_; }

    // xglDeviceWaitIdle()
    void wait();

    // xglWaitForFences()
    XGL_RESULT wait(const std::vector<const Fence *> &fences, bool wait_all, uint64_t timeout);
    XGL_RESULT wait(const Fence &fence) { return wait(std::vector<const Fence *>(1, &fence), true, (uint64_t) -1); }

    // xglBeginDescriptorPoolUpdate()
    // xglEndDescriptorPoolUpdate()
    void begin_descriptor_pool_update(XGL_DESCRIPTOR_UPDATE_MODE mode);
    void end_descriptor_pool_update(CmdBuffer &cmd);

private:
    enum QueueIndex {
        GRAPHICS,
        COMPUTE,
        DMA,
        QUEUE_COUNT,
    };

    void init_queues();
    void init_formats();

    PhysicalGpu gpu_;

    std::vector<Queue *> queues_[QUEUE_COUNT];
    std::vector<Format> formats_;
};

class Queue : public DerivedObject<XGL_QUEUE, BaseObject> {
public:
    explicit Queue(XGL_QUEUE queue) : DerivedObject(queue) {}

    // xglQueueSubmit()
    void submit(const std::vector<const CmdBuffer *> &cmds, Fence &fence);
    void submit(const CmdBuffer &cmd, Fence &fence);
    void submit(const CmdBuffer &cmd);

    // xglQueueAddMemReference()
    // xglQueueRemoveMemReference()
    void add_mem_references(const std::vector<XGL_GPU_MEMORY> &mem_refs);
    void remove_mem_references(const std::vector<XGL_GPU_MEMORY> &mem_refs);

    // xglQueueWaitIdle()
    void wait();

    // xglQueueSignalSemaphore()
    // xglQueueWaitSemaphore()
    void signal_semaphore(Semaphore &sem);
    void wait_semaphore(Semaphore &sem);
};

class GpuMemory : public DerivedObject<XGL_GPU_MEMORY, BaseObject> {
public:
    ~GpuMemory();

    // xglAllocMemory()
    void init(const Device &dev, const XGL_MEMORY_ALLOC_INFO &info);
    // xglPinSystemMemory()
    void init(const Device &dev, size_t size, const void *data);
    // xglOpenSharedMemory()
    void init(const Device &dev, const XGL_MEMORY_OPEN_INFO &info);
    // xglOpenPeerMemory()
    void init(const Device &dev, const XGL_PEER_MEMORY_OPEN_INFO &info);

    void init(XGL_GPU_MEMORY mem) { BaseObject::init(mem, false); }

    // xglSetMemoryPriority()
    void set_priority(XGL_MEMORY_PRIORITY priority);

    // xglMapMemory()
    const void *map(XGL_FLAGS flags) const;
          void *map(XGL_FLAGS flags);
    const void *map() const { return map(0); }
          void *map()       { return map(0); }

    // xglUnmapMemory()
    void unmap() const;

    static XGL_MEMORY_ALLOC_INFO alloc_info(const XGL_MEMORY_REQUIREMENTS &reqs,
                  const XGL_MEMORY_ALLOC_INFO *next_info);
};

class Fence : public DerivedObject<XGL_FENCE, Object> {
public:
    // xglCreateFence()
    void init(const Device &dev, const XGL_FENCE_CREATE_INFO &info);

    // xglGetFenceStatus()
    XGL_RESULT status() const { return xglGetFenceStatus(obj()); }

    static XGL_FENCE_CREATE_INFO create_info(XGL_FENCE_CREATE_FLAGS flags);
    static XGL_FENCE_CREATE_INFO create_info();
};

class Semaphore : public DerivedObject<XGL_SEMAPHORE, Object> {
public:
    // xglCreateSemaphore()
    void init(const Device &dev, const XGL_SEMAPHORE_CREATE_INFO &info);
    // xglOpenSharedSemaphore()
    void init(const Device &dev, const XGL_SEMAPHORE_OPEN_INFO &info);

    static XGL_SEMAPHORE_CREATE_INFO create_info(uint32_t init_count, XGL_FLAGS flags);
};

class Event : public DerivedObject<XGL_EVENT, Object> {
public:
    // xglCreateEvent()
    void init(const Device &dev, const XGL_EVENT_CREATE_INFO &info);

    // xglGetEventStatus()
    // xglSetEvent()
    // xglResetEvent()
    XGL_RESULT status() const { return xglGetEventStatus(obj()); }
    void set();
    void reset();

    static XGL_EVENT_CREATE_INFO create_info(XGL_FLAGS flags);
};

class QueryPool : public DerivedObject<XGL_QUERY_POOL, Object> {
public:
    // xglCreateQueryPool()
    void init(const Device &dev, const XGL_QUERY_POOL_CREATE_INFO &info);

    // xglGetQueryPoolResults()
    XGL_RESULT results(uint32_t start, uint32_t count, size_t size, void *data);

    static XGL_QUERY_POOL_CREATE_INFO create_info(XGL_QUERY_TYPE type, uint32_t slot_count);
};

class Buffer : public DerivedObject<XGL_BUFFER, Object> {
public:
    explicit Buffer() {}
    explicit Buffer(const Device &dev, const XGL_BUFFER_CREATE_INFO &info) { init(dev, info); }
    explicit Buffer(const Device &dev, XGL_GPU_SIZE size) { init(dev, size); }

    // xglCreateBuffer()
    void init(const Device &dev, const XGL_BUFFER_CREATE_INFO &info);
    void init(const Device &dev, XGL_GPU_SIZE size) { init(dev, create_info(size, 0)); }
    void init_no_mem(const Device &dev, const XGL_BUFFER_CREATE_INFO &info);

    static XGL_BUFFER_CREATE_INFO create_info(XGL_GPU_SIZE size, XGL_FLAGS usage);

    XGL_BUFFER_MEMORY_BARRIER buffer_memory_barrier(XGL_FLAGS output_mask, XGL_FLAGS input_mask,
                                                 XGL_GPU_SIZE offset, XGL_GPU_SIZE size) const
    {
        XGL_BUFFER_MEMORY_BARRIER barrier = {};
        barrier.sType = XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.buffer = obj();
        barrier.outputMask = output_mask;
        barrier.inputMask = input_mask;
        barrier.offset = offset;
        barrier.size = size;
        return barrier;
    }
private:
    XGL_BUFFER_CREATE_INFO create_info_;
};

class BufferView : public DerivedObject<XGL_BUFFER_VIEW, Object> {
public:
    // xglCreateBufferView()
    void init(const Device &dev, const XGL_BUFFER_VIEW_CREATE_INFO &info);
};

class Image : public DerivedObject<XGL_IMAGE, Object> {
public:
    explicit Image() : format_features_(0) {}
    explicit Image(const Device &dev, const XGL_IMAGE_CREATE_INFO &info) : format_features_(0) { init(dev, info); }

    // xglCreateImage()
    void init(const Device &dev, const XGL_IMAGE_CREATE_INFO &info);
    void init_no_mem(const Device &dev, const XGL_IMAGE_CREATE_INFO &info);
    // xglOpenPeerImage()
    void init(const Device &dev, const XGL_PEER_IMAGE_OPEN_INFO &info, const XGL_IMAGE_CREATE_INFO &original_info);

    // xglBindImageMemoryRange()
    void bind_memory(uint32_t alloc_idx, const XGL_IMAGE_MEMORY_BIND_INFO &info,
                     const GpuMemory &mem, XGL_GPU_SIZE mem_offset);

    // xglGetImageSubresourceInfo()
    XGL_SUBRESOURCE_LAYOUT subresource_layout(const XGL_IMAGE_SUBRESOURCE &subres) const;

    bool transparent() const;
    bool copyable() const { return (format_features_ & XGL_FORMAT_IMAGE_COPY_BIT); }

    XGL_IMAGE_SUBRESOURCE_RANGE subresource_range(XGL_IMAGE_ASPECT aspect) const { return subresource_range(create_info_, aspect); }
    XGL_EXTENT3D extent() const { return create_info_.extent; }
    XGL_EXTENT3D extent(uint32_t mip_level) const { return extent(create_info_.extent, mip_level); }
    XGL_FORMAT format() const {return create_info_.format;}

    XGL_IMAGE_MEMORY_BARRIER image_memory_barrier(XGL_FLAGS output_mask, XGL_FLAGS input_mask,
                                                  XGL_IMAGE_LAYOUT old_layout,
                                                  XGL_IMAGE_LAYOUT new_layout,
                                                  const XGL_IMAGE_SUBRESOURCE_RANGE &range) const
    {
        XGL_IMAGE_MEMORY_BARRIER barrier = {};
        barrier.sType = XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.outputMask = output_mask;
        barrier.inputMask = input_mask;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.image = obj();
        barrier.subresourceRange = range;
        return barrier;
    }

    static XGL_IMAGE_CREATE_INFO create_info();
    static XGL_IMAGE_SUBRESOURCE subresource(XGL_IMAGE_ASPECT aspect, uint32_t mip_level, uint32_t array_slice);
    static XGL_IMAGE_SUBRESOURCE subresource(const XGL_IMAGE_SUBRESOURCE_RANGE &range, uint32_t mip_level, uint32_t array_slice);
    static XGL_IMAGE_SUBRESOURCE_RANGE subresource_range(XGL_IMAGE_ASPECT aspect, uint32_t base_mip_level, uint32_t mip_levels,
                                                                                  uint32_t base_array_slice, uint32_t array_size);
    static XGL_IMAGE_SUBRESOURCE_RANGE subresource_range(const XGL_IMAGE_CREATE_INFO &info, XGL_IMAGE_ASPECT aspect);
    static XGL_IMAGE_SUBRESOURCE_RANGE subresource_range(const XGL_IMAGE_SUBRESOURCE &subres);

    static XGL_EXTENT2D extent(int32_t width, int32_t height);
    static XGL_EXTENT2D extent(const XGL_EXTENT2D &extent, uint32_t mip_level);
    static XGL_EXTENT2D extent(const XGL_EXTENT3D &extent);

    static XGL_EXTENT3D extent(int32_t width, int32_t height, int32_t depth);
    static XGL_EXTENT3D extent(const XGL_EXTENT3D &extent, uint32_t mip_level);

private:
    void init_info(const Device &dev, const XGL_IMAGE_CREATE_INFO &info);

    XGL_IMAGE_CREATE_INFO create_info_;
    XGL_FLAGS format_features_;
};

class ImageView : public DerivedObject<XGL_IMAGE_VIEW, Object> {
public:
    // xglCreateImageView()
    void init(const Device &dev, const XGL_IMAGE_VIEW_CREATE_INFO &info);
};

class ColorAttachmentView : public DerivedObject<XGL_COLOR_ATTACHMENT_VIEW, Object> {
public:
    // xglCreateColorAttachmentView()
    void init(const Device &dev, const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO &info);
};

class DepthStencilView : public DerivedObject<XGL_DEPTH_STENCIL_VIEW, Object> {
public:
    // xglCreateDepthStencilView()
    void init(const Device &dev, const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO &info);
};

class Shader : public DerivedObject<XGL_SHADER, Object> {
public:
    // xglCreateShader()
    void init(const Device &dev, const XGL_SHADER_CREATE_INFO &info);
    XGL_RESULT init_try(const Device &dev, const XGL_SHADER_CREATE_INFO &info);

    static XGL_SHADER_CREATE_INFO create_info(size_t code_size, const void *code, XGL_FLAGS flags);
};

class Pipeline : public DerivedObject<XGL_PIPELINE, Object> {
public:
    // xglCreateGraphicsPipeline()
    void init(const Device &dev, const XGL_GRAPHICS_PIPELINE_CREATE_INFO &info);
    // xglCreateGraphicsPipelineDerivative()
    void init(const Device &dev, const XGL_GRAPHICS_PIPELINE_CREATE_INFO &info, const XGL_PIPELINE basePipeline);
    // xglCreateComputePipeline()
    void init(const Device &dev, const XGL_COMPUTE_PIPELINE_CREATE_INFO &info);
    // xglLoadPipeline()
    void init(const Device&dev, size_t size, const void *data);
    // xglLoadPipelineDerivative()
    void init(const Device&dev, size_t size, const void *data, XGL_PIPELINE basePipeline);

    // xglStorePipeline()
    size_t store(size_t size, void *data);
};

class Sampler : public DerivedObject<XGL_SAMPLER, Object> {
public:
    // xglCreateSampler()
    void init(const Device &dev, const XGL_SAMPLER_CREATE_INFO &info);
};

class DescriptorSetLayout : public DerivedObject<XGL_DESCRIPTOR_SET_LAYOUT, Object> {
public:
    // xglCreateDescriptorSetLayout()
    void init(const Device &dev, const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO &info);
};

class DescriptorSetLayoutChain : public DerivedObject<XGL_DESCRIPTOR_SET_LAYOUT_CHAIN, Object> {
public:
    // xglCreateDescriptorSetLayoutChain()
    void init(const Device &dev, const std::vector<const DescriptorSetLayout *> &layouts);
};

class DescriptorPool : public DerivedObject<XGL_DESCRIPTOR_POOL, Object> {
public:
    // xglCreateDescriptorPool()
    void init(const Device &dev, XGL_DESCRIPTOR_POOL_USAGE usage,
              uint32_t max_sets, const XGL_DESCRIPTOR_POOL_CREATE_INFO &info);

    // xglResetDescriptorPool()
    void reset();

    // xglAllocDescriptorSets()
    std::vector<DescriptorSet *> alloc_sets(XGL_DESCRIPTOR_SET_USAGE usage, const std::vector<const DescriptorSetLayout *> &layouts);
    std::vector<DescriptorSet *> alloc_sets(XGL_DESCRIPTOR_SET_USAGE usage, const DescriptorSetLayout &layout, uint32_t count);
    DescriptorSet *alloc_sets(XGL_DESCRIPTOR_SET_USAGE usage, const DescriptorSetLayout &layout);

    // xglClearDescriptorSets()
    void clear_sets(const std::vector<DescriptorSet *> &sets);
    void clear_sets(DescriptorSet &set) { clear_sets(std::vector<DescriptorSet *>(1, &set)); }
};

class DescriptorSet : public DerivedObject<XGL_DESCRIPTOR_SET, Object> {
public:
    explicit DescriptorSet(XGL_DESCRIPTOR_SET set) : DerivedObject(set) {}

    // xglUpdateDescriptors()
    void update(const std::vector<const void *> &update_array);

    static XGL_UPDATE_SAMPLERS update(uint32_t binding, uint32_t index, uint32_t count, const XGL_SAMPLER *samplers);
    static XGL_UPDATE_SAMPLERS update(uint32_t binding, uint32_t index, const std::vector<XGL_SAMPLER> &samplers);

    static XGL_UPDATE_SAMPLER_TEXTURES update(uint32_t binding, uint32_t index, uint32_t count, const XGL_SAMPLER_IMAGE_VIEW_INFO *textures);
    static XGL_UPDATE_SAMPLER_TEXTURES update(uint32_t binding, uint32_t index, const std::vector<XGL_SAMPLER_IMAGE_VIEW_INFO> &textures);

    static XGL_UPDATE_IMAGES update(XGL_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, uint32_t count, const XGL_IMAGE_VIEW_ATTACH_INFO *views);
    static XGL_UPDATE_IMAGES update(XGL_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, const std::vector<XGL_IMAGE_VIEW_ATTACH_INFO> &views);

    static XGL_UPDATE_BUFFERS update(XGL_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, uint32_t count, const XGL_BUFFER_VIEW_ATTACH_INFO *views);
    static XGL_UPDATE_BUFFERS update(XGL_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, const std::vector<XGL_BUFFER_VIEW_ATTACH_INFO> &views);

    static XGL_UPDATE_AS_COPY update(XGL_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, uint32_t count, const DescriptorSet &set);

    static XGL_BUFFER_VIEW_ATTACH_INFO attach_info(const BufferView &view);
    static XGL_IMAGE_VIEW_ATTACH_INFO attach_info(const ImageView &view, XGL_IMAGE_LAYOUT layout);
};

class DynamicVpStateObject : public DerivedObject<XGL_DYNAMIC_VP_STATE_OBJECT, DynamicStateObject> {
public:
    // xglCreateDynamicViewportState()
    void init(const Device &dev, const XGL_DYNAMIC_VP_STATE_CREATE_INFO &info);
};

class DynamicRsStateObject : public DerivedObject<XGL_DYNAMIC_RS_STATE_OBJECT, DynamicStateObject> {
public:
    // xglCreateDynamicRasterState()
    void init(const Device &dev, const XGL_DYNAMIC_RS_STATE_CREATE_INFO &info);
};

class DynamicCbStateObject : public DerivedObject<XGL_DYNAMIC_CB_STATE_OBJECT, DynamicStateObject> {
public:
    // xglCreateDynamicColorBlendState()
    void init(const Device &dev, const XGL_DYNAMIC_CB_STATE_CREATE_INFO &info);
};

class DynamicDsStateObject : public DerivedObject<XGL_DYNAMIC_DS_STATE_OBJECT, DynamicStateObject> {
public:
    // xglCreateDynamicDepthStencilState()
    void init(const Device &dev, const XGL_DYNAMIC_DS_STATE_CREATE_INFO &info);
};

class CmdBuffer : public DerivedObject<XGL_CMD_BUFFER, Object> {
public:
    explicit CmdBuffer() {}
    explicit CmdBuffer(const Device &dev, const XGL_CMD_BUFFER_CREATE_INFO &info) { init(dev, info); }

    // xglCreateCommandBuffer()
    void init(const Device &dev, const XGL_CMD_BUFFER_CREATE_INFO &info);

    // xglBeginCommandBuffer()
    void begin(const XGL_CMD_BUFFER_BEGIN_INFO *info);
    void begin(XGL_RENDER_PASS renderpass_obj, XGL_FRAMEBUFFER framebuffer_obj);
    void begin();

    // xglEndCommandBuffer()
    // xglResetCommandBuffer()
    void end();
    void reset();

    static XGL_CMD_BUFFER_CREATE_INFO create_info(uint32_t queueNodeIndex);
};

inline const void *Object::map(XGL_FLAGS flags) const
{
    return (primary_mem_) ? primary_mem_->map(flags) : NULL;
}

inline void *Object::map(XGL_FLAGS flags)
{
    return (primary_mem_) ? primary_mem_->map(flags) : NULL;
}

inline void Object::unmap() const
{
    if (primary_mem_)
        primary_mem_->unmap();
}

inline XGL_MEMORY_ALLOC_INFO GpuMemory::alloc_info(const XGL_MEMORY_REQUIREMENTS &reqs,
                                const XGL_MEMORY_ALLOC_INFO *next_info)
{
    XGL_MEMORY_ALLOC_INFO info = {};
    info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    if (next_info != NULL)
        info.pNext = (void *) next_info;

    info.allocationSize = reqs.size;
    info.memProps = reqs.memProps;
    info.memType = reqs.memType;
    info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;
    return info;
}

inline XGL_BUFFER_CREATE_INFO Buffer::create_info(XGL_GPU_SIZE size, XGL_FLAGS usage)
{
    XGL_BUFFER_CREATE_INFO info = {};
    info.sType = XGL_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    return info;
}

inline XGL_FENCE_CREATE_INFO Fence::create_info(XGL_FENCE_CREATE_FLAGS flags)
{
    XGL_FENCE_CREATE_INFO info = {};
    info.sType = XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = flags;
    return info;
}

inline XGL_FENCE_CREATE_INFO Fence::create_info()
{
    XGL_FENCE_CREATE_INFO info = {};
    info.sType = XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    return info;
}

inline XGL_SEMAPHORE_CREATE_INFO Semaphore::create_info(uint32_t init_count, XGL_FLAGS flags)
{
    XGL_SEMAPHORE_CREATE_INFO info = {};
    info.sType = XGL_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.initialCount = init_count;
    info.flags = flags;
    return info;
}

inline XGL_EVENT_CREATE_INFO Event::create_info(XGL_FLAGS flags)
{
    XGL_EVENT_CREATE_INFO info = {};
    info.sType = XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    info.flags = flags;
    return info;
}

inline XGL_QUERY_POOL_CREATE_INFO QueryPool::create_info(XGL_QUERY_TYPE type, uint32_t slot_count)
{
    XGL_QUERY_POOL_CREATE_INFO info = {};
    info.sType = XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    info.queryType = type;
    info.slots = slot_count;
    return info;
}

inline XGL_IMAGE_CREATE_INFO Image::create_info()
{
    XGL_IMAGE_CREATE_INFO info = {};
    info.sType = XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.extent.width = 1;
    info.extent.height = 1;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arraySize = 1;
    info.samples = 1;
    return info;
}

inline XGL_IMAGE_SUBRESOURCE Image::subresource(XGL_IMAGE_ASPECT aspect, uint32_t mip_level, uint32_t array_slice)
{
    XGL_IMAGE_SUBRESOURCE subres = {};
    subres.aspect = aspect;
    subres.mipLevel = mip_level;
    subres.arraySlice = array_slice;
    return subres;
}

inline XGL_IMAGE_SUBRESOURCE Image::subresource(const XGL_IMAGE_SUBRESOURCE_RANGE &range, uint32_t mip_level, uint32_t array_slice)
{
    return subresource(range.aspect, range.baseMipLevel + mip_level, range.baseArraySlice + array_slice);
}

inline XGL_IMAGE_SUBRESOURCE_RANGE Image::subresource_range(XGL_IMAGE_ASPECT aspect, uint32_t base_mip_level, uint32_t mip_levels,
                                                                                     uint32_t base_array_slice, uint32_t array_size)
{
    XGL_IMAGE_SUBRESOURCE_RANGE range = {};
    range.aspect = aspect;
    range.baseMipLevel = base_mip_level;
    range.mipLevels = mip_levels;
    range.baseArraySlice = base_array_slice;
    range.arraySize = array_size;
    return range;
}

inline XGL_IMAGE_SUBRESOURCE_RANGE Image::subresource_range(const XGL_IMAGE_CREATE_INFO &info, XGL_IMAGE_ASPECT aspect)
{
    return subresource_range(aspect, 0, info.mipLevels, 0, info.arraySize);
}

inline XGL_IMAGE_SUBRESOURCE_RANGE Image::subresource_range(const XGL_IMAGE_SUBRESOURCE &subres)
{
    return subresource_range(subres.aspect, subres.mipLevel, 1, subres.arraySlice, 1);
}

inline XGL_EXTENT2D Image::extent(int32_t width, int32_t height)
{
    XGL_EXTENT2D extent = {};
    extent.width = width;
    extent.height = height;
    return extent;
}

inline XGL_EXTENT2D Image::extent(const XGL_EXTENT2D &extent, uint32_t mip_level)
{
    const int32_t width  = (extent.width  >> mip_level) ? extent.width  >> mip_level : 1;
    const int32_t height = (extent.height >> mip_level) ? extent.height >> mip_level : 1;
    return Image::extent(width, height);
}

inline XGL_EXTENT2D Image::extent(const XGL_EXTENT3D &extent)
{
    return Image::extent(extent.width, extent.height);
}

inline XGL_EXTENT3D Image::extent(int32_t width, int32_t height, int32_t depth)
{
    XGL_EXTENT3D extent = {};
    extent.width = width;
    extent.height = height;
    extent.depth = depth;
    return extent;
}

inline XGL_EXTENT3D Image::extent(const XGL_EXTENT3D &extent, uint32_t mip_level)
{
    const int32_t width  = (extent.width  >> mip_level) ? extent.width  >> mip_level : 1;
    const int32_t height = (extent.height >> mip_level) ? extent.height >> mip_level : 1;
    const int32_t depth  = (extent.depth  >> mip_level) ? extent.depth  >> mip_level : 1;
    return Image::extent(width, height, depth);
}

inline XGL_SHADER_CREATE_INFO Shader::create_info(size_t code_size, const void *code, XGL_FLAGS flags)
{
    XGL_SHADER_CREATE_INFO info = {};
    info.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    info.codeSize = code_size;
    info.pCode = code;
    info.flags = flags;
    return info;
}

inline XGL_BUFFER_VIEW_ATTACH_INFO DescriptorSet::attach_info(const BufferView &view)
{
    XGL_BUFFER_VIEW_ATTACH_INFO info = {};
    info.sType = XGL_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO;
    info.view = view.obj();
    return info;
}

inline XGL_IMAGE_VIEW_ATTACH_INFO DescriptorSet::attach_info(const ImageView &view, XGL_IMAGE_LAYOUT layout)
{
    XGL_IMAGE_VIEW_ATTACH_INFO info = {};
    info.sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO;
    info.view = view.obj();
    info.layout = layout;
    return info;
}

inline XGL_UPDATE_SAMPLERS DescriptorSet::update(uint32_t binding, uint32_t index, uint32_t count, const XGL_SAMPLER *samplers)
{
    XGL_UPDATE_SAMPLERS info = {};
    info.sType = XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS;
    info.binding = binding;
    info.arrayIndex = index;
    info.count = count;
    info.pSamplers = samplers;
    return info;
}

inline XGL_UPDATE_SAMPLERS DescriptorSet::update(uint32_t binding, uint32_t index, const std::vector<XGL_SAMPLER> &samplers)
{
    return update(binding, index, samplers.size(), &samplers[0]);
}

inline XGL_UPDATE_SAMPLER_TEXTURES DescriptorSet::update(uint32_t binding, uint32_t index, uint32_t count, const XGL_SAMPLER_IMAGE_VIEW_INFO *textures)
{
    XGL_UPDATE_SAMPLER_TEXTURES info = {};
    info.sType = XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES;
    info.binding = binding;
    info.arrayIndex = index;
    info.count = count;
    info.pSamplerImageViews = textures;
    return info;
}

inline XGL_UPDATE_SAMPLER_TEXTURES DescriptorSet::update(uint32_t binding, uint32_t index, const std::vector<XGL_SAMPLER_IMAGE_VIEW_INFO> &textures)
{
    return update(binding, index, textures.size(), &textures[0]);
}

inline XGL_UPDATE_IMAGES DescriptorSet::update(XGL_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, uint32_t count,
                                               const XGL_IMAGE_VIEW_ATTACH_INFO *views)
{
    XGL_UPDATE_IMAGES info = {};
    info.sType = XGL_STRUCTURE_TYPE_UPDATE_IMAGES;
    info.descriptorType = type;
    info.binding = binding;
    info.arrayIndex = index;
    info.count = count;
    info.pImageViews = views;
    return info;
}

inline XGL_UPDATE_IMAGES DescriptorSet::update(XGL_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index,
                                               const std::vector<XGL_IMAGE_VIEW_ATTACH_INFO> &views)
{
    return update(type, binding, index, views.size(), &views[0]);
}

inline XGL_UPDATE_BUFFERS DescriptorSet::update(XGL_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, uint32_t count,
                                                const XGL_BUFFER_VIEW_ATTACH_INFO *views)
{
    XGL_UPDATE_BUFFERS info = {};
    info.sType = XGL_STRUCTURE_TYPE_UPDATE_BUFFERS;
    info.descriptorType = type;
    info.binding = binding;
    info.arrayIndex = index;
    info.count = count;
    info.pBufferViews = views;
    return info;
}

inline XGL_UPDATE_BUFFERS DescriptorSet::update(XGL_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index,
                                                const std::vector<XGL_BUFFER_VIEW_ATTACH_INFO> &views)
{
    return update(type, binding, index, views.size(), &views[0]);
}

inline XGL_UPDATE_AS_COPY DescriptorSet::update(XGL_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, uint32_t count, const DescriptorSet &set)
{
    XGL_UPDATE_AS_COPY info = {};
    info.sType = XGL_STRUCTURE_TYPE_UPDATE_AS_COPY;
    info.descriptorType = type;
    info.binding = binding;
    info.arrayElement = index;
    info.count = count;
    info.descriptorSet = set.obj();
    return info;
}

inline XGL_CMD_BUFFER_CREATE_INFO CmdBuffer::create_info(uint32_t queueNodeIndex)
{
    XGL_CMD_BUFFER_CREATE_INFO info = {};
    info.sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
    info.queueNodeIndex = queueNodeIndex;
    return info;
}

}; // namespace xgl_testing

#endif // XGLTESTBINDING_H
