// VK tests
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

#ifndef VKTESTBINDING_H
#define VKTESTBINDING_H

#include <vector>

#include "vulkan.h"

namespace vk_testing {

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
    explicit PhysicalGpu(VK_PHYSICAL_GPU gpu) : gpu_(gpu) {}

    const VK_PHYSICAL_GPU &obj() const { return gpu_; }

    // vkGetGpuInfo()
    VK_PHYSICAL_GPU_PROPERTIES properties() const;
    VK_PHYSICAL_GPU_PERFORMANCE performance() const;
    VK_PHYSICAL_GPU_MEMORY_PROPERTIES memory_properties() const;
    std::vector<VK_PHYSICAL_GPU_QUEUE_PROPERTIES> queue_properties() const;

    // vkGetProcAddr()
    void *get_proc(const char *name) const { return vkGetProcAddr(gpu_, name); }

    // vkGetExtensionSupport()
    bool has_extension(const char *ext) const { return (vkGetExtensionSupport(gpu_, ext) == VK_SUCCESS); }
    std::vector<const char *> extensions() const;

    // vkEnumerateLayers()
    std::vector<const char *> layers(std::vector<char> &buf) const;

    // vkGetMultiGpuCompatibility()
    VK_GPU_COMPATIBILITY_INFO compatibility(const PhysicalGpu &other) const;

private:
    VK_PHYSICAL_GPU gpu_;
};

class BaseObject {
public:
    const VK_BASE_OBJECT &obj() const { return obj_; }
    bool initialized() const { return (obj_ != VK_NULL_HANDLE); }

    // vkGetObjectInfo()
    uint32_t memory_allocation_count() const;
    std::vector<VK_MEMORY_REQUIREMENTS> memory_requirements() const;

protected:
    explicit BaseObject() : obj_(VK_NULL_HANDLE), own_obj_(false) {}
    explicit BaseObject(VK_BASE_OBJECT obj) : obj_(VK_NULL_HANDLE), own_obj_(false) { init(obj); }

    void init(VK_BASE_OBJECT obj, bool own);
    void init(VK_BASE_OBJECT obj) { init(obj, true); }

    void reinit(VK_BASE_OBJECT obj, bool own);
    void reinit(VK_BASE_OBJECT obj) { reinit(obj, true); }

    bool own() const { return own_obj_; }

private:
    // base objects are non-copyable
    BaseObject(const BaseObject &);
    BaseObject &operator=(const BaseObject &);

    VK_BASE_OBJECT obj_;
    bool own_obj_;
};

class Object : public BaseObject {
public:
    const VK_OBJECT &obj() const { return reinterpret_cast<const VK_OBJECT &>(BaseObject::obj()); }

    // vkBindObjectMemory()
    void bind_memory(uint32_t alloc_idx, const GpuMemory &mem, VK_GPU_SIZE mem_offset);
    void unbind_memory(uint32_t alloc_idx);
    void unbind_memory();

    // vkBindObjectMemoryRange()
    void bind_memory(uint32_t alloc_idx, VK_GPU_SIZE offset, VK_GPU_SIZE size,
                     const GpuMemory &mem, VK_GPU_SIZE mem_offset);

    // Unless an object is initialized with init_no_mem(), memories are
    // automatically allocated and bound.  These methods can be used to get
    // the memories (for vkQueueAddMemReference), or to map/unmap the primary memory.
    std::vector<VK_GPU_MEMORY> memories() const;

    const void *map(VK_FLAGS flags) const;
          void *map(VK_FLAGS flags);
    const void *map() const { return map(0); }
          void *map()       { return map(0); }

    void unmap() const;

protected:
    explicit Object() : mem_alloc_count_(0), internal_mems_(NULL), primary_mem_(NULL), bound(false) {}
    explicit Object(VK_OBJECT obj) : mem_alloc_count_(0), internal_mems_(NULL), primary_mem_(NULL) { init(obj); }
    ~Object() { cleanup(); }

    void init(VK_OBJECT obj, bool own);
    void init(VK_OBJECT obj) { init(obj, true); }

    void reinit(VK_OBJECT obj, bool own);
    void reinit(VK_OBJECT obj) { init(obj, true); }

    // allocate and bind internal memories
    void alloc_memory(const Device &dev, bool for_linear_img, bool for_img);
    void alloc_memory(const Device &dev) { alloc_memory(dev, false, false); }
    void alloc_memory(const std::vector<VK_GPU_MEMORY> &mems);

private:
    void cleanup();

    uint32_t mem_alloc_count_;
    GpuMemory *internal_mems_;
    GpuMemory *primary_mem_;
    bool bound;
};

class DynamicStateObject : public Object {
public:
    const VK_DYNAMIC_STATE_OBJECT &obj() const { return reinterpret_cast<const VK_DYNAMIC_STATE_OBJECT &>(Object::obj()); }

protected:
    explicit DynamicStateObject() {}
    explicit DynamicStateObject(VK_DYNAMIC_STATE_OBJECT obj) : Object(obj) {}
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

class Device : public DerivedObject<VK_DEVICE, BaseObject> {
public:
    explicit Device(VK_PHYSICAL_GPU gpu) : gpu_(gpu) {}
    ~Device();

    // vkCreateDevice()
    void init(const VkDeviceCreateInfo &info);
    void init(bool enable_layers); // all queues, all extensions, etc
    void init() { init(false); };

    const PhysicalGpu &gpu() const { return gpu_; }

    // vkGetDeviceQueue()
    const std::vector<Queue *> &graphics_queues() { return queues_[GRAPHICS]; }
    const std::vector<Queue *> &compute_queues() { return queues_[COMPUTE]; }
    const std::vector<Queue *> &dma_queues() { return queues_[DMA]; }
    uint32_t graphics_queue_node_index_;

    struct Format {
        VK_FORMAT format;
        VK_IMAGE_TILING tiling;
        VK_FLAGS features;
    };
    // vkGetFormatInfo()
    VK_FORMAT_PROPERTIES format_properties(VK_FORMAT format);
    const std::vector<Format> &formats() const { return formats_; }

    // vkDeviceWaitIdle()
    void wait();

    // vkWaitForFences()
    VK_RESULT wait(const std::vector<const Fence *> &fences, bool wait_all, uint64_t timeout);
    VK_RESULT wait(const Fence &fence) { return wait(std::vector<const Fence *>(1, &fence), true, (uint64_t) -1); }

    // vkBeginDescriptorPoolUpdate()
    // vkEndDescriptorPoolUpdate()
    void begin_descriptor_pool_update(VK_DESCRIPTOR_UPDATE_MODE mode);
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

class Queue : public DerivedObject<VK_QUEUE, BaseObject> {
public:
    explicit Queue(VK_QUEUE queue) : DerivedObject(queue) {}

    // vkQueueSubmit()
    void submit(const std::vector<const CmdBuffer *> &cmds, Fence &fence);
    void submit(const CmdBuffer &cmd, Fence &fence);
    void submit(const CmdBuffer &cmd);

    // vkQueueAddMemReference()
    // vkQueueRemoveMemReference()
    void add_mem_references(const std::vector<VK_GPU_MEMORY> &mem_refs);
    void remove_mem_references(const std::vector<VK_GPU_MEMORY> &mem_refs);

    // vkQueueWaitIdle()
    void wait();

    // vkQueueSignalSemaphore()
    // vkQueueWaitSemaphore()
    void signal_semaphore(Semaphore &sem);
    void wait_semaphore(Semaphore &sem);
};

class GpuMemory : public DerivedObject<VK_GPU_MEMORY, BaseObject> {
public:
    ~GpuMemory();

    // vkAllocMemory()
    void init(const Device &dev, const VkMemoryAllocInfo &info);
    // vkPinSystemMemory()
    void init(const Device &dev, size_t size, const void *data);
    // vkOpenSharedMemory()
    void init(const Device &dev, const VK_MEMORY_OPEN_INFO &info);
    // vkOpenPeerMemory()
    void init(const Device &dev, const VK_PEER_MEMORY_OPEN_INFO &info);

    void init(VK_GPU_MEMORY mem) { BaseObject::init(mem, false); }

    // vkSetMemoryPriority()
    void set_priority(VK_MEMORY_PRIORITY priority);

    // vkMapMemory()
    const void *map(VK_FLAGS flags) const;
          void *map(VK_FLAGS flags);
    const void *map() const { return map(0); }
          void *map()       { return map(0); }

    // vkUnmapMemory()
    void unmap() const;

    static VkMemoryAllocInfo alloc_info(const VK_MEMORY_REQUIREMENTS &reqs,
                  const VkMemoryAllocInfo *next_info);
};

class Fence : public DerivedObject<VK_FENCE, Object> {
public:
    // vkCreateFence()
    void init(const Device &dev, const VK_FENCE_CREATE_INFO &info);

    // vkGetFenceStatus()
    VK_RESULT status() const { return vkGetFenceStatus(obj()); }

    static VK_FENCE_CREATE_INFO create_info(VK_FENCE_CREATE_FLAGS flags);
    static VK_FENCE_CREATE_INFO create_info();
};

class Semaphore : public DerivedObject<VK_SEMAPHORE, Object> {
public:
    // vkCreateSemaphore()
    void init(const Device &dev, const VK_SEMAPHORE_CREATE_INFO &info);
    // vkOpenSharedSemaphore()
    void init(const Device &dev, const VK_SEMAPHORE_OPEN_INFO &info);

    static VK_SEMAPHORE_CREATE_INFO create_info(uint32_t init_count, VK_FLAGS flags);
};

class Event : public DerivedObject<VK_EVENT, Object> {
public:
    // vkCreateEvent()
    void init(const Device &dev, const VK_EVENT_CREATE_INFO &info);

    // vkGetEventStatus()
    // vkSetEvent()
    // vkResetEvent()
    VK_RESULT status() const { return vkGetEventStatus(obj()); }
    void set();
    void reset();

    static VK_EVENT_CREATE_INFO create_info(VK_FLAGS flags);
};

class QueryPool : public DerivedObject<VK_QUERY_POOL, Object> {
public:
    // vkCreateQueryPool()
    void init(const Device &dev, const VK_QUERY_POOL_CREATE_INFO &info);

    // vkGetQueryPoolResults()
    VK_RESULT results(uint32_t start, uint32_t count, size_t size, void *data);

    static VK_QUERY_POOL_CREATE_INFO create_info(VK_QUERY_TYPE type, uint32_t slot_count);
};

class Buffer : public DerivedObject<VK_BUFFER, Object> {
public:
    explicit Buffer() {}
    explicit Buffer(const Device &dev, const VkBufferCreateInfo &info) { init(dev, info); }
    explicit Buffer(const Device &dev, VK_GPU_SIZE size) { init(dev, size); }

    // vkCreateBuffer()
    void init(const Device &dev, const VkBufferCreateInfo &info);
    void init(const Device &dev, VK_GPU_SIZE size) { init(dev, create_info(size, 0)); }
    void init_no_mem(const Device &dev, const VkBufferCreateInfo &info);

    static VkBufferCreateInfo create_info(VK_GPU_SIZE size, VK_FLAGS usage);

    VK_BUFFER_MEMORY_BARRIER buffer_memory_barrier(VK_FLAGS output_mask, VK_FLAGS input_mask,
                                                 VK_GPU_SIZE offset, VK_GPU_SIZE size) const
    {
        VK_BUFFER_MEMORY_BARRIER barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.buffer = obj();
        barrier.outputMask = output_mask;
        barrier.inputMask = input_mask;
        barrier.offset = offset;
        barrier.size = size;
        return barrier;
    }
private:
    VkBufferCreateInfo create_info_;
};

class BufferView : public DerivedObject<VK_BUFFER_VIEW, Object> {
public:
    // vkCreateBufferView()
    void init(const Device &dev, const VkBufferViewCreateInfo &info);
};

class Image : public DerivedObject<VK_IMAGE, Object> {
public:
    explicit Image() : format_features_(0) {}
    explicit Image(const Device &dev, const VK_IMAGE_CREATE_INFO &info) : format_features_(0) { init(dev, info); }

    // vkCreateImage()
    void init(const Device &dev, const VK_IMAGE_CREATE_INFO &info);
    void init_no_mem(const Device &dev, const VK_IMAGE_CREATE_INFO &info);
    // vkOpenPeerImage()
    void init(const Device &dev, const VK_PEER_IMAGE_OPEN_INFO &info, const VK_IMAGE_CREATE_INFO &original_info);

    // vkBindImageMemoryRange()
    void bind_memory(uint32_t alloc_idx, const VK_IMAGE_MEMORY_BIND_INFO &info,
                     const GpuMemory &mem, VK_GPU_SIZE mem_offset);

    // vkGetImageSubresourceInfo()
    VK_SUBRESOURCE_LAYOUT subresource_layout(const VK_IMAGE_SUBRESOURCE &subres) const;

    bool transparent() const;
    bool copyable() const { return (format_features_ & VK_FORMAT_IMAGE_COPY_BIT); }

    VK_IMAGE_SUBRESOURCE_RANGE subresource_range(VK_IMAGE_ASPECT aspect) const { return subresource_range(create_info_, aspect); }
    VK_EXTENT3D extent() const { return create_info_.extent; }
    VK_EXTENT3D extent(uint32_t mip_level) const { return extent(create_info_.extent, mip_level); }
    VK_FORMAT format() const {return create_info_.format;}

    VK_IMAGE_MEMORY_BARRIER image_memory_barrier(VK_FLAGS output_mask, VK_FLAGS input_mask,
                                                  VK_IMAGE_LAYOUT old_layout,
                                                  VK_IMAGE_LAYOUT new_layout,
                                                  const VK_IMAGE_SUBRESOURCE_RANGE &range) const
    {
        VK_IMAGE_MEMORY_BARRIER barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.outputMask = output_mask;
        barrier.inputMask = input_mask;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.image = obj();
        barrier.subresourceRange = range;
        return barrier;
    }

    static VK_IMAGE_CREATE_INFO create_info();
    static VK_IMAGE_SUBRESOURCE subresource(VK_IMAGE_ASPECT aspect, uint32_t mip_level, uint32_t array_slice);
    static VK_IMAGE_SUBRESOURCE subresource(const VK_IMAGE_SUBRESOURCE_RANGE &range, uint32_t mip_level, uint32_t array_slice);
    static VK_IMAGE_SUBRESOURCE_RANGE subresource_range(VK_IMAGE_ASPECT aspect, uint32_t base_mip_level, uint32_t mip_levels,
                                                                                  uint32_t base_array_slice, uint32_t array_size);
    static VK_IMAGE_SUBRESOURCE_RANGE subresource_range(const VK_IMAGE_CREATE_INFO &info, VK_IMAGE_ASPECT aspect);
    static VK_IMAGE_SUBRESOURCE_RANGE subresource_range(const VK_IMAGE_SUBRESOURCE &subres);

    static VK_EXTENT2D extent(int32_t width, int32_t height);
    static VK_EXTENT2D extent(const VK_EXTENT2D &extent, uint32_t mip_level);
    static VK_EXTENT2D extent(const VK_EXTENT3D &extent);

    static VK_EXTENT3D extent(int32_t width, int32_t height, int32_t depth);
    static VK_EXTENT3D extent(const VK_EXTENT3D &extent, uint32_t mip_level);

private:
    void init_info(const Device &dev, const VK_IMAGE_CREATE_INFO &info);

    VK_IMAGE_CREATE_INFO create_info_;
    VK_FLAGS format_features_;
};

class ImageView : public DerivedObject<VK_IMAGE_VIEW, Object> {
public:
    // vkCreateImageView()
    void init(const Device &dev, const VK_IMAGE_VIEW_CREATE_INFO &info);
};

class ColorAttachmentView : public DerivedObject<VK_COLOR_ATTACHMENT_VIEW, Object> {
public:
    // vkCreateColorAttachmentView()
    void init(const Device &dev, const VK_COLOR_ATTACHMENT_VIEW_CREATE_INFO &info);
};

class DepthStencilView : public DerivedObject<VK_DEPTH_STENCIL_VIEW, Object> {
public:
    // vkCreateDepthStencilView()
    void init(const Device &dev, const VK_DEPTH_STENCIL_VIEW_CREATE_INFO &info);
};

class Shader : public DerivedObject<VK_SHADER, Object> {
public:
    // vkCreateShader()
    void init(const Device &dev, const VK_SHADER_CREATE_INFO &info);
    VK_RESULT init_try(const Device &dev, const VK_SHADER_CREATE_INFO &info);

    static VK_SHADER_CREATE_INFO create_info(size_t code_size, const void *code, VK_FLAGS flags);
};

class Pipeline : public DerivedObject<VK_PIPELINE, Object> {
public:
    // vkCreateGraphicsPipeline()
    void init(const Device &dev, const VK_GRAPHICS_PIPELINE_CREATE_INFO &info);
    // vkCreateGraphicsPipelineDerivative()
    void init(const Device &dev, const VK_GRAPHICS_PIPELINE_CREATE_INFO &info, const VK_PIPELINE basePipeline);
    // vkCreateComputePipeline()
    void init(const Device &dev, const VK_COMPUTE_PIPELINE_CREATE_INFO &info);
    // vkLoadPipeline()
    void init(const Device&dev, size_t size, const void *data);
    // vkLoadPipelineDerivative()
    void init(const Device&dev, size_t size, const void *data, VK_PIPELINE basePipeline);

    // vkStorePipeline()
    size_t store(size_t size, void *data);
};

class Sampler : public DerivedObject<VK_SAMPLER, Object> {
public:
    // vkCreateSampler()
    void init(const Device &dev, const VK_SAMPLER_CREATE_INFO &info);
};

class DescriptorSetLayout : public DerivedObject<VK_DESCRIPTOR_SET_LAYOUT, Object> {
public:
    // vkCreateDescriptorSetLayout()
    void init(const Device &dev, const VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO &info);
};

class DescriptorSetLayoutChain : public DerivedObject<VK_DESCRIPTOR_SET_LAYOUT_CHAIN, Object> {
public:
    // vkCreateDescriptorSetLayoutChain()
    void init(const Device &dev, const std::vector<const DescriptorSetLayout *> &layouts);
};

class DescriptorPool : public DerivedObject<VK_DESCRIPTOR_POOL, Object> {
public:
    // vkCreateDescriptorPool()
    void init(const Device &dev, VK_DESCRIPTOR_POOL_USAGE usage,
              uint32_t max_sets, const VK_DESCRIPTOR_POOL_CREATE_INFO &info);

    // vkResetDescriptorPool()
    void reset();

    // vkAllocDescriptorSets()
    std::vector<DescriptorSet *> alloc_sets(VK_DESCRIPTOR_SET_USAGE usage, const std::vector<const DescriptorSetLayout *> &layouts);
    std::vector<DescriptorSet *> alloc_sets(VK_DESCRIPTOR_SET_USAGE usage, const DescriptorSetLayout &layout, uint32_t count);
    DescriptorSet *alloc_sets(VK_DESCRIPTOR_SET_USAGE usage, const DescriptorSetLayout &layout);

    // vkClearDescriptorSets()
    void clear_sets(const std::vector<DescriptorSet *> &sets);
    void clear_sets(DescriptorSet &set) { clear_sets(std::vector<DescriptorSet *>(1, &set)); }
};

class DescriptorSet : public DerivedObject<VK_DESCRIPTOR_SET, Object> {
public:
    explicit DescriptorSet(VK_DESCRIPTOR_SET set) : DerivedObject(set) {}

    // vkUpdateDescriptors()
    void update(const std::vector<const void *> &update_array);

    static VK_UPDATE_SAMPLERS update(uint32_t binding, uint32_t index, uint32_t count, const VK_SAMPLER *samplers);
    static VK_UPDATE_SAMPLERS update(uint32_t binding, uint32_t index, const std::vector<VK_SAMPLER> &samplers);

    static VK_UPDATE_SAMPLER_TEXTURES update(uint32_t binding, uint32_t index, uint32_t count, const VK_SAMPLER_IMAGE_VIEW_INFO *textures);
    static VK_UPDATE_SAMPLER_TEXTURES update(uint32_t binding, uint32_t index, const std::vector<VK_SAMPLER_IMAGE_VIEW_INFO> &textures);

    static VK_UPDATE_IMAGES update(VK_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, uint32_t count, const VK_IMAGE_VIEW_ATTACH_INFO *views);
    static VK_UPDATE_IMAGES update(VK_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, const std::vector<VK_IMAGE_VIEW_ATTACH_INFO> &views);

    static VK_UPDATE_BUFFERS update(VK_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, uint32_t count, const VK_BUFFER_VIEW_ATTACH_INFO *views);
    static VK_UPDATE_BUFFERS update(VK_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, const std::vector<VK_BUFFER_VIEW_ATTACH_INFO> &views);

    static VK_UPDATE_AS_COPY update(VK_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, uint32_t count, const DescriptorSet &set);

    static VK_BUFFER_VIEW_ATTACH_INFO attach_info(const BufferView &view);
    static VK_IMAGE_VIEW_ATTACH_INFO attach_info(const ImageView &view, VK_IMAGE_LAYOUT layout);
};

class DynamicVpStateObject : public DerivedObject<VK_DYNAMIC_VP_STATE_OBJECT, DynamicStateObject> {
public:
    // vkCreateDynamicViewportState()
    void init(const Device &dev, const VK_DYNAMIC_VP_STATE_CREATE_INFO &info);
};

class DynamicRsStateObject : public DerivedObject<VK_DYNAMIC_RS_STATE_OBJECT, DynamicStateObject> {
public:
    // vkCreateDynamicRasterState()
    void init(const Device &dev, const VK_DYNAMIC_RS_STATE_CREATE_INFO &info);
};

class DynamicCbStateObject : public DerivedObject<VK_DYNAMIC_CB_STATE_OBJECT, DynamicStateObject> {
public:
    // vkCreateDynamicColorBlendState()
    void init(const Device &dev, const VK_DYNAMIC_CB_STATE_CREATE_INFO &info);
};

class DynamicDsStateObject : public DerivedObject<VK_DYNAMIC_DS_STATE_OBJECT, DynamicStateObject> {
public:
    // vkCreateDynamicDepthStencilState()
    void init(const Device &dev, const VK_DYNAMIC_DS_STATE_CREATE_INFO &info);
};

class CmdBuffer : public DerivedObject<VK_CMD_BUFFER, Object> {
public:
    explicit CmdBuffer() {}
    explicit CmdBuffer(const Device &dev, const VK_CMD_BUFFER_CREATE_INFO &info) { init(dev, info); }

    // vkCreateCommandBuffer()
    void init(const Device &dev, const VK_CMD_BUFFER_CREATE_INFO &info);

    // vkBeginCommandBuffer()
    void begin(const VK_CMD_BUFFER_BEGIN_INFO *info);
    void begin(VK_RENDER_PASS renderpass_obj, VK_FRAMEBUFFER framebuffer_obj);
    void begin();

    // vkEndCommandBuffer()
    // vkResetCommandBuffer()
    void end();
    void reset();

    static VK_CMD_BUFFER_CREATE_INFO create_info(uint32_t queueNodeIndex);
};

inline const void *Object::map(VK_FLAGS flags) const
{
    return (primary_mem_) ? primary_mem_->map(flags) : NULL;
}

inline void *Object::map(VK_FLAGS flags)
{
    return (primary_mem_) ? primary_mem_->map(flags) : NULL;
}

inline void Object::unmap() const
{
    if (primary_mem_)
        primary_mem_->unmap();
}

inline VkMemoryAllocInfo GpuMemory::alloc_info(const VK_MEMORY_REQUIREMENTS &reqs,
                                const VkMemoryAllocInfo *next_info)
{
    VkMemoryAllocInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    if (next_info != NULL)
        info.pNext = (void *) next_info;

    info.allocationSize = reqs.size;
    info.memProps = reqs.memProps;
    info.memType = reqs.memType;
    info.memPriority = VK_MEMORY_PRIORITY_NORMAL;
    return info;
}

inline VkBufferCreateInfo Buffer::create_info(VK_GPU_SIZE size, VK_FLAGS usage)
{
    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    return info;
}

inline VK_FENCE_CREATE_INFO Fence::create_info(VK_FENCE_CREATE_FLAGS flags)
{
    VK_FENCE_CREATE_INFO info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = flags;
    return info;
}

inline VK_FENCE_CREATE_INFO Fence::create_info()
{
    VK_FENCE_CREATE_INFO info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    return info;
}

inline VK_SEMAPHORE_CREATE_INFO Semaphore::create_info(uint32_t init_count, VK_FLAGS flags)
{
    VK_SEMAPHORE_CREATE_INFO info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.initialCount = init_count;
    info.flags = flags;
    return info;
}

inline VK_EVENT_CREATE_INFO Event::create_info(VK_FLAGS flags)
{
    VK_EVENT_CREATE_INFO info = {};
    info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    info.flags = flags;
    return info;
}

inline VK_QUERY_POOL_CREATE_INFO QueryPool::create_info(VK_QUERY_TYPE type, uint32_t slot_count)
{
    VK_QUERY_POOL_CREATE_INFO info = {};
    info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    info.queryType = type;
    info.slots = slot_count;
    return info;
}

inline VK_IMAGE_CREATE_INFO Image::create_info()
{
    VK_IMAGE_CREATE_INFO info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.extent.width = 1;
    info.extent.height = 1;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arraySize = 1;
    info.samples = 1;
    return info;
}

inline VK_IMAGE_SUBRESOURCE Image::subresource(VK_IMAGE_ASPECT aspect, uint32_t mip_level, uint32_t array_slice)
{
    VK_IMAGE_SUBRESOURCE subres = {};
    subres.aspect = aspect;
    subres.mipLevel = mip_level;
    subres.arraySlice = array_slice;
    return subres;
}

inline VK_IMAGE_SUBRESOURCE Image::subresource(const VK_IMAGE_SUBRESOURCE_RANGE &range, uint32_t mip_level, uint32_t array_slice)
{
    return subresource(range.aspect, range.baseMipLevel + mip_level, range.baseArraySlice + array_slice);
}

inline VK_IMAGE_SUBRESOURCE_RANGE Image::subresource_range(VK_IMAGE_ASPECT aspect, uint32_t base_mip_level, uint32_t mip_levels,
                                                                                     uint32_t base_array_slice, uint32_t array_size)
{
    VK_IMAGE_SUBRESOURCE_RANGE range = {};
    range.aspect = aspect;
    range.baseMipLevel = base_mip_level;
    range.mipLevels = mip_levels;
    range.baseArraySlice = base_array_slice;
    range.arraySize = array_size;
    return range;
}

inline VK_IMAGE_SUBRESOURCE_RANGE Image::subresource_range(const VK_IMAGE_CREATE_INFO &info, VK_IMAGE_ASPECT aspect)
{
    return subresource_range(aspect, 0, info.mipLevels, 0, info.arraySize);
}

inline VK_IMAGE_SUBRESOURCE_RANGE Image::subresource_range(const VK_IMAGE_SUBRESOURCE &subres)
{
    return subresource_range(subres.aspect, subres.mipLevel, 1, subres.arraySlice, 1);
}

inline VK_EXTENT2D Image::extent(int32_t width, int32_t height)
{
    VK_EXTENT2D extent = {};
    extent.width = width;
    extent.height = height;
    return extent;
}

inline VK_EXTENT2D Image::extent(const VK_EXTENT2D &extent, uint32_t mip_level)
{
    const int32_t width  = (extent.width  >> mip_level) ? extent.width  >> mip_level : 1;
    const int32_t height = (extent.height >> mip_level) ? extent.height >> mip_level : 1;
    return Image::extent(width, height);
}

inline VK_EXTENT2D Image::extent(const VK_EXTENT3D &extent)
{
    return Image::extent(extent.width, extent.height);
}

inline VK_EXTENT3D Image::extent(int32_t width, int32_t height, int32_t depth)
{
    VK_EXTENT3D extent = {};
    extent.width = width;
    extent.height = height;
    extent.depth = depth;
    return extent;
}

inline VK_EXTENT3D Image::extent(const VK_EXTENT3D &extent, uint32_t mip_level)
{
    const int32_t width  = (extent.width  >> mip_level) ? extent.width  >> mip_level : 1;
    const int32_t height = (extent.height >> mip_level) ? extent.height >> mip_level : 1;
    const int32_t depth  = (extent.depth  >> mip_level) ? extent.depth  >> mip_level : 1;
    return Image::extent(width, height, depth);
}

inline VK_SHADER_CREATE_INFO Shader::create_info(size_t code_size, const void *code, VK_FLAGS flags)
{
    VK_SHADER_CREATE_INFO info = {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    info.codeSize = code_size;
    info.pCode = code;
    info.flags = flags;
    return info;
}

inline VK_BUFFER_VIEW_ATTACH_INFO DescriptorSet::attach_info(const BufferView &view)
{
    VK_BUFFER_VIEW_ATTACH_INFO info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO;
    info.view = view.obj();
    return info;
}

inline VK_IMAGE_VIEW_ATTACH_INFO DescriptorSet::attach_info(const ImageView &view, VK_IMAGE_LAYOUT layout)
{
    VK_IMAGE_VIEW_ATTACH_INFO info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO;
    info.view = view.obj();
    info.layout = layout;
    return info;
}

inline VK_UPDATE_SAMPLERS DescriptorSet::update(uint32_t binding, uint32_t index, uint32_t count, const VK_SAMPLER *samplers)
{
    VK_UPDATE_SAMPLERS info = {};
    info.sType = VK_STRUCTURE_TYPE_UPDATE_SAMPLERS;
    info.binding = binding;
    info.arrayIndex = index;
    info.count = count;
    info.pSamplers = samplers;
    return info;
}

inline VK_UPDATE_SAMPLERS DescriptorSet::update(uint32_t binding, uint32_t index, const std::vector<VK_SAMPLER> &samplers)
{
    return update(binding, index, samplers.size(), &samplers[0]);
}

inline VK_UPDATE_SAMPLER_TEXTURES DescriptorSet::update(uint32_t binding, uint32_t index, uint32_t count, const VK_SAMPLER_IMAGE_VIEW_INFO *textures)
{
    VK_UPDATE_SAMPLER_TEXTURES info = {};
    info.sType = VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES;
    info.binding = binding;
    info.arrayIndex = index;
    info.count = count;
    info.pSamplerImageViews = textures;
    return info;
}

inline VK_UPDATE_SAMPLER_TEXTURES DescriptorSet::update(uint32_t binding, uint32_t index, const std::vector<VK_SAMPLER_IMAGE_VIEW_INFO> &textures)
{
    return update(binding, index, textures.size(), &textures[0]);
}

inline VK_UPDATE_IMAGES DescriptorSet::update(VK_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, uint32_t count,
                                               const VK_IMAGE_VIEW_ATTACH_INFO *views)
{
    VK_UPDATE_IMAGES info = {};
    info.sType = VK_STRUCTURE_TYPE_UPDATE_IMAGES;
    info.descriptorType = type;
    info.binding = binding;
    info.arrayIndex = index;
    info.count = count;
    info.pImageViews = views;
    return info;
}

inline VK_UPDATE_IMAGES DescriptorSet::update(VK_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index,
                                               const std::vector<VK_IMAGE_VIEW_ATTACH_INFO> &views)
{
    return update(type, binding, index, views.size(), &views[0]);
}

inline VK_UPDATE_BUFFERS DescriptorSet::update(VK_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, uint32_t count,
                                                const VK_BUFFER_VIEW_ATTACH_INFO *views)
{
    VK_UPDATE_BUFFERS info = {};
    info.sType = VK_STRUCTURE_TYPE_UPDATE_BUFFERS;
    info.descriptorType = type;
    info.binding = binding;
    info.arrayIndex = index;
    info.count = count;
    info.pBufferViews = views;
    return info;
}

inline VK_UPDATE_BUFFERS DescriptorSet::update(VK_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index,
                                                const std::vector<VK_BUFFER_VIEW_ATTACH_INFO> &views)
{
    return update(type, binding, index, views.size(), &views[0]);
}

inline VK_UPDATE_AS_COPY DescriptorSet::update(VK_DESCRIPTOR_TYPE type, uint32_t binding, uint32_t index, uint32_t count, const DescriptorSet &set)
{
    VK_UPDATE_AS_COPY info = {};
    info.sType = VK_STRUCTURE_TYPE_UPDATE_AS_COPY;
    info.descriptorType = type;
    info.binding = binding;
    info.arrayElement = index;
    info.count = count;
    info.descriptorSet = set.obj();
    return info;
}

inline VK_CMD_BUFFER_CREATE_INFO CmdBuffer::create_info(uint32_t queueNodeIndex)
{
    VK_CMD_BUFFER_CREATE_INFO info = {};
    info.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
    info.queueNodeIndex = queueNodeIndex;
    return info;
}

}; // namespace vk_testing

#endif // VKTESTBINDING_H
