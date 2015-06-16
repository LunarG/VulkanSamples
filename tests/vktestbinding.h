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
class PipelineLayout;
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
    explicit PhysicalGpu(VkPhysicalDevice gpu) : gpu_(gpu) {}

    const VkPhysicalDevice &obj() const { return gpu_; }

    // vkGetPhysicalDeviceInfo()
    VkPhysicalDeviceProperties properties() const;
    VkPhysicalDevicePerformance performance() const;
    VkPhysicalDeviceMemoryProperties memory_properties() const;
    std::vector<VkPhysicalDeviceQueueProperties> queue_properties() const;


    // vkGetGlobalExtensionInfo()
    std::vector<VkExtensionProperties> extensions() const;

    // vkEnumerateLayers()
    std::vector<const char *> layers(std::vector<char> &buf) const;

    // vkGetMultiDeviceCompatibility()
    VkPhysicalDeviceCompatibilityInfo compatibility(const PhysicalGpu &other) const;

private:
    void add_extension_dependencies(uint32_t dependency_count,
                                    VkExtensionProperties *depencency_props,
                                    std::vector<VkExtensionProperties> &ext_list);
    VkPhysicalDevice gpu_;
};

class BaseObject {
public:
    const VkObject &obj() const { return obj_; }
    VkObjectType type() const { return object_type_; }
    bool initialized() const { return (obj_ != VK_NULL_HANDLE); }

protected:
    explicit BaseObject() :
        object_type_((VkObjectType) 0), obj_(VK_NULL_HANDLE), own_obj_(false){}
    explicit BaseObject(VkObject obj, VkObjectType object_type) :
        object_type_(object_type), obj_(obj), own_obj_(false){}

    void init(VkObject obj, VkObjectType object_type, bool own);
    void init(VkObject obj, VkObjectType object_type) { init(obj, object_type, true); }

    void reinit(VkObject obj, VkObjectType object_type, bool own);
    void reinit(VkObject obj, VkObjectType object_type) { reinit(obj, object_type, true); }

    bool own() const { return own_obj_; }

private:
    // base objects are non-copyable
    BaseObject(const BaseObject &);
    BaseObject &operator=(const BaseObject &);

    VkObjectType object_type_;
    VkObject obj_;
    bool own_obj_;
};

class Object : public BaseObject {
public:
    const VkObject &obj() const { return reinterpret_cast<const VkObject &>(BaseObject::obj()); }

    // vkGetObjectInfo()
    uint32_t memory_allocation_count() const;
    std::vector<VkMemoryRequirements> memory_requirements() const;

    // vkBindObjectMemory()
    void bind_memory(const GpuMemory &mem, VkDeviceSize mem_offset);

    // Unless an object is initialized with init_no_mem(), memories are
    // automatically allocated and bound.  These methods can be used to
    // map/unmap the primary memory.
    std::vector<VkDeviceMemory> memories() const;

    const void *map(VkFlags flags) const;
          void *map(VkFlags flags);
    const void *map() const { return map(0); }
          void *map()       { return map(0); }

    void unmap() const;
    const Device* dev_;

protected:
    explicit Object() :
        mem_alloc_count_(0), internal_mems_(NULL),
        primary_mem_(NULL), bound(false) {}
    explicit Object(const Device &dev, VkObject obj, VkObjectType object_type) :
        dev_(&dev),
        mem_alloc_count_(0), internal_mems_(NULL),
        primary_mem_(NULL), bound(false) { init(obj, object_type); }
    ~Object() { cleanup(); }

    void init(VkObject obj, VkObjectType object_type, bool own);
    void init(VkObject obj, VkObjectType object_type) { init(obj, object_type, true); }

    void reinit(VkObject obj, VkObjectType object_type, bool own);
    void reinit(VkObject obj, VkObjectType object_type) { init(obj, object_type, true); }

    // allocate and bind internal memories
    void alloc_memory();
    void alloc_memory(VkMemoryPropertyFlags &reqs);
    void alloc_memory(const std::vector<VkDeviceMemory> &mems);

private:
    void cleanup();

    uint32_t mem_alloc_count_;
    GpuMemory *internal_mems_;
    GpuMemory *primary_mem_;
    bool bound;
};

class DynamicStateObject : public Object {
public:
    const VkDynamicStateObject &obj() const { return reinterpret_cast<const VkDynamicStateObject &>(Object::obj()); }

protected:
    explicit DynamicStateObject() : Object() {}
};

template<typename T, class C, VkObjectType V>
class DerivedObject : public C {
public:
    const T &obj() const { return reinterpret_cast<const T &>(C::obj()); }

protected:
    typedef T obj_type;
    typedef C base_type;

    explicit DerivedObject() {}
    explicit DerivedObject(T obj) : C(obj, V) {}
    explicit DerivedObject(const Device &dev, T obj) : C(dev, obj, V) {}
};

class Device : public DerivedObject<VkDevice, BaseObject, VK_OBJECT_TYPE_DEVICE> {
public:
    explicit Device(VkPhysicalDevice gpu) : gpu_(gpu) {}
    ~Device();

    VkDevice device() const { return obj(); }

    // vkCreateDevice()
    void init(const VkDeviceCreateInfo &info);
    void init(std::vector<VkExtensionProperties> extensions); // all queues, all extensions, etc
    void init() { std::vector<VkExtensionProperties> extensions; init(extensions); };

    const PhysicalGpu &gpu() const { return gpu_; }

    // vkGetDeviceProcAddr()
    void *get_proc(const char *name) const { return vkGetDeviceProcAddr(obj(), name); }

    // vkGetDeviceQueue()
    const std::vector<Queue *> &graphics_queues() const { return queues_[GRAPHICS]; }
    const std::vector<Queue *> &compute_queues() { return queues_[COMPUTE]; }
    const std::vector<Queue *> &dma_queues() { return queues_[DMA]; }
    uint32_t graphics_queue_node_index_;

    struct Format {
        VkFormat format;
        VkImageTiling tiling;
        VkFlags features;
    };
    // vkGetFormatInfo()
    VkFormatProperties format_properties(VkFormat format);
    const std::vector<Format> &formats() const { return formats_; }

    // vkDeviceWaitIdle()
    void wait();

    // vkWaitForFences()
    VkResult wait(const std::vector<const Fence *> &fences, bool wait_all, uint64_t timeout);
    VkResult wait(const Fence &fence) { return wait(std::vector<const Fence *>(1, &fence), true, (uint64_t) -1); }

    // vkUpdateDescriptorSets()
    VkResult update_descriptor_sets(const std::vector<VkWriteDescriptorSet> &writes, const std::vector<VkCopyDescriptorSet> &copies);
    VkResult update_descriptor_sets(const std::vector<VkWriteDescriptorSet> &writes) { return update_descriptor_sets(writes, std::vector<VkCopyDescriptorSet>()); }

    static VkWriteDescriptorSet write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                     VkDescriptorType type, uint32_t count, const VkDescriptorInfo *descriptors);
    static VkWriteDescriptorSet write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                     VkDescriptorType type, const std::vector<VkDescriptorInfo> &descriptors);

    static VkCopyDescriptorSet copy_descriptor_set(const DescriptorSet &src_set, uint32_t src_binding, uint32_t src_array_element,
                                                   const DescriptorSet &dst_set, uint32_t dst_binding, uint32_t dst_array_element,
                                                   uint32_t count);

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

class Queue : public DerivedObject<VkQueue, BaseObject, VK_OBJECT_TYPE_QUEUE> {
public:
    explicit Queue(VkQueue queue) : DerivedObject(queue) {}

    // vkQueueSubmit()
    void submit(const std::vector<const CmdBuffer *> &cmds, Fence &fence);
    void submit(const CmdBuffer &cmd, Fence &fence);
    void submit(const CmdBuffer &cmd);

    // vkQueueAddMemReferences()
    // vkQueueRemoveMemReferences()
    void add_mem_references(const std::vector<VkDeviceMemory> &mem_refs);
    void remove_mem_references(const std::vector<VkDeviceMemory> &mem_refs);

    // vkQueueWaitIdle()
    void wait();

    // vkQueueSignalSemaphore()
    // vkQueueWaitSemaphore()
    void signal_semaphore(Semaphore &sem);
    void wait_semaphore(Semaphore &sem);
};

/* Note: This needs to be BaseObject so that we don't try to destroy
 * the object when the object is device memory.
 */
class GpuMemory : public DerivedObject<VkDeviceMemory, BaseObject, VK_OBJECT_TYPE_DEVICE_MEMORY> {
public:
    ~GpuMemory();

    // vkAllocMemory()
    void init(const Device &dev, const VkMemoryAllocInfo &info);
    // vkOpenSharedMemory()
    void init(const Device &dev, const VkMemoryOpenInfo &info);
    // vkOpenPeerMemory()
    void init(const Device &dev, const VkPeerMemoryOpenInfo &info);

    void init(const Device &dev, VkDeviceMemory mem);

    // vkMapMemory()
    const void *map(VkFlags flags) const;
          void *map(VkFlags flags);
    const void *map() const { return map(0); }
          void *map()       { return map(0); }

    // vkUnmapMemory()
    void unmap() const;

    static VkMemoryAllocInfo alloc_info(const VkMemoryRequirements &reqs,
                  const VkMemoryAllocInfo *next_info);
private:
    const Device* dev_;
};

class Fence : public DerivedObject<VkFence, Object, VK_OBJECT_TYPE_FENCE> {
public:
    // vkCreateFence()
    void init(const Device &dev, const VkFenceCreateInfo &info);

    // vkGetFenceStatus()
    VkResult status() const { return vkGetFenceStatus(dev_->obj(), obj()); }

    static VkFenceCreateInfo create_info(VkFenceCreateFlags flags);
    static VkFenceCreateInfo create_info();
};

class Semaphore : public DerivedObject<VkSemaphore, Object, VK_OBJECT_TYPE_SEMAPHORE> {
public:
    // vkCreateSemaphore()
    void init(const Device &dev, const VkSemaphoreCreateInfo &info);
    // vkOpenSharedSemaphore()
    void init(const Device &dev, const VkSemaphoreOpenInfo &info);

    static VkSemaphoreCreateInfo create_info(uint32_t init_count, VkFlags flags);
};

class Event : public DerivedObject<VkEvent, Object, VK_OBJECT_TYPE_EVENT> {
public:
    // vkCreateEvent()
    void init(const Device &dev, const VkEventCreateInfo &info);

    // vkGetEventStatus()
    // vkSetEvent()
    // vkResetEvent()
    VkResult status() const { return vkGetEventStatus(dev_->obj(), obj()); }
    void set();
    void reset();

    static VkEventCreateInfo create_info(VkFlags flags);
};

class QueryPool : public DerivedObject<VkQueryPool, Object, VK_OBJECT_TYPE_QUERY_POOL> {
public:
    // vkCreateQueryPool()
    void init(const Device &dev, const VkQueryPoolCreateInfo &info);

    // vkGetQueryPoolResults()
    VkResult results(uint32_t start, uint32_t count, size_t size, void *data);

    static VkQueryPoolCreateInfo create_info(VkQueryType type, uint32_t slot_count);
};

class Buffer : public DerivedObject<VkBuffer, Object, VK_OBJECT_TYPE_BUFFER> {
public:
    explicit Buffer() {}
    explicit Buffer(const Device &dev, const VkBufferCreateInfo &info) { init(dev, info); }
    explicit Buffer(const Device &dev, VkDeviceSize size) { init(dev, size); }

    // vkCreateBuffer()
    void init(const Device &dev, const VkBufferCreateInfo &info);
    void init(const Device &dev, VkDeviceSize size) { init(dev, create_info(size, 0)); }
    void init(const Device &dev, VkDeviceSize size, VkMemoryPropertyFlags &reqs) { init(dev, create_info(size, 0), reqs); }
    void init(const Device &dev, const VkBufferCreateInfo &info, VkMemoryPropertyFlags &reqs);
    void init_no_mem(const Device &dev, const VkBufferCreateInfo &info);

    // vkQueueBindSparseBufferMemory()
    void bind_memory(VkDeviceSize offset, VkDeviceSize size,
                     const GpuMemory &mem, VkDeviceSize mem_offset);

    static VkBufferCreateInfo create_info(VkDeviceSize size, VkFlags usage);

    VkBufferMemoryBarrier buffer_memory_barrier(VkFlags output_mask, VkFlags input_mask,
                                                 VkDeviceSize offset, VkDeviceSize size) const
    {
        VkBufferMemoryBarrier barrier = {};
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

class BufferView : public DerivedObject<VkBufferView, Object, VK_OBJECT_TYPE_BUFFER_VIEW> {
public:
    // vkCreateBufferView()
    void init(const Device &dev, const VkBufferViewCreateInfo &info);
};

class Image : public DerivedObject<VkImage, Object, VK_OBJECT_TYPE_IMAGE> {
public:
    explicit Image() : format_features_(0) {}
    explicit Image(const Device &dev, const VkImageCreateInfo &info) : format_features_(0) { init(dev, info); }

    // vkCreateImage()
    void init(const Device &dev, const VkImageCreateInfo &info);
    void init(const Device &dev, const VkImageCreateInfo &info, VkMemoryPropertyFlags &reqs);
    void init_no_mem(const Device &dev, const VkImageCreateInfo &info);
    // vkOpenPeerImage()
    void init(const Device &dev, const VkPeerImageOpenInfo &info, const VkImageCreateInfo &original_info);

    // vkQueueBindSparseImageMemory()
    void bind_memory(const Device &dev, const VkImageMemoryBindInfo &info,
                     const GpuMemory &mem, VkDeviceSize mem_offset);

    // vkGetImageSubresourceInfo()
    VkSubresourceLayout subresource_layout(const VkImageSubresource &subres) const;

    bool transparent() const;
    bool copyable() const { return (format_features_ & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT); }

    VkImageSubresourceRange subresource_range(VkImageAspect aspect) const { return subresource_range(create_info_, aspect); }
    VkExtent3D extent() const { return create_info_.extent; }
    VkExtent3D extent(uint32_t mip_level) const { return extent(create_info_.extent, mip_level); }
    VkFormat format() const {return create_info_.format;}

    VkImageMemoryBarrier image_memory_barrier(VkFlags output_mask, VkFlags input_mask,
                                                  VkImageLayout old_layout,
                                                  VkImageLayout new_layout,
                                                  const VkImageSubresourceRange &range) const
    {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.outputMask = output_mask;
        barrier.inputMask = input_mask;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.image = obj();
        barrier.subresourceRange = range;
        return barrier;
    }

    static VkImageCreateInfo create_info();
    static VkImageSubresource subresource(VkImageAspect aspect, uint32_t mip_level, uint32_t array_slice);
    static VkImageSubresource subresource(const VkImageSubresourceRange &range, uint32_t mip_level, uint32_t array_slice);
    static VkImageSubresourceRange subresource_range(VkImageAspect aspect, uint32_t base_mip_level, uint32_t mip_levels,
                                                                                  uint32_t base_array_slice, uint32_t array_size);
    static VkImageSubresourceRange subresource_range(const VkImageCreateInfo &info, VkImageAspect aspect);
    static VkImageSubresourceRange subresource_range(const VkImageSubresource &subres);

    static VkExtent2D extent(int32_t width, int32_t height);
    static VkExtent2D extent(const VkExtent2D &extent, uint32_t mip_level);
    static VkExtent2D extent(const VkExtent3D &extent);

    static VkExtent3D extent(int32_t width, int32_t height, int32_t depth);
    static VkExtent3D extent(const VkExtent3D &extent, uint32_t mip_level);

private:
    void init_info(const Device &dev, const VkImageCreateInfo &info);

    VkImageCreateInfo create_info_;
    VkFlags format_features_;
};

class ImageView : public DerivedObject<VkImageView, Object, VK_OBJECT_TYPE_IMAGE_VIEW> {
public:
    // vkCreateImageView()
    void init(const Device &dev, const VkImageViewCreateInfo &info);
};

class ColorAttachmentView : public DerivedObject<VkColorAttachmentView, Object, VK_OBJECT_TYPE_COLOR_ATTACHMENT_VIEW> {
public:
    // vkCreateColorAttachmentView()
    void init(const Device &dev, const VkColorAttachmentViewCreateInfo &info);
};

class DepthStencilView : public DerivedObject<VkDepthStencilView, Object, VK_OBJECT_TYPE_DEPTH_STENCIL_VIEW> {
public:
    // vkCreateDepthStencilView()
    void init(const Device &dev, const VkDepthStencilViewCreateInfo &info);
};

class Shader : public DerivedObject<VkShader, Object, VK_OBJECT_TYPE_SHADER> {
public:
    // vkCreateShader()
    void init(const Device &dev, const VkShaderCreateInfo &info);
    VkResult init_try(const Device &dev, const VkShaderCreateInfo &info);

    static VkShaderCreateInfo create_info(size_t code_size, const void *code, VkFlags flags);
};

class Pipeline : public DerivedObject<VkPipeline, Object, VK_OBJECT_TYPE_PIPELINE> {
public:
    // vkCreateGraphicsPipeline()
    void init(const Device &dev, const VkGraphicsPipelineCreateInfo &info);
    // vkCreateGraphicsPipelineDerivative()
    void init(const Device &dev, const VkGraphicsPipelineCreateInfo &info, const VkPipeline basePipeline);
    // vkCreateComputePipeline()
    void init(const Device &dev, const VkComputePipelineCreateInfo &info);
    // vkLoadPipeline()
    void init(const Device&dev, size_t size, const void *data);
    // vkLoadPipelineDerivative()
    void init(const Device&dev, size_t size, const void *data, VkPipeline basePipeline);

    // vkCreateGraphicsPipeline with error return
    VkResult init_try(const Device &dev, const VkGraphicsPipelineCreateInfo &info);

    // vkStorePipeline()
    size_t store(size_t size, void *data);
};

class Sampler : public DerivedObject<VkSampler, Object, VK_OBJECT_TYPE_SAMPLER> {
public:
    // vkCreateSampler()
    void init(const Device &dev, const VkSamplerCreateInfo &info);
};

class DescriptorSetLayout : public DerivedObject<VkDescriptorSetLayout, Object, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT> {
public:
    // vkCreateDescriptorSetLayout()
    void init(const Device &dev, const VkDescriptorSetLayoutCreateInfo &info);
};

class PipelineLayout : public DerivedObject<VkPipelineLayout, Object, VK_OBJECT_TYPE_PIPELINE_LAYOUT> {
public:
    // vCreatePipelineLayout()
    void init(const Device &dev, VkPipelineLayoutCreateInfo &info, const std::vector<const DescriptorSetLayout *> &layouts);
};

class DescriptorPool : public DerivedObject<VkDescriptorPool, Object, VK_OBJECT_TYPE_DESCRIPTOR_POOL> {
public:
    // vkCreateDescriptorPool()
    void init(const Device &dev, VkDescriptorPoolUsage usage,
              uint32_t max_sets, const VkDescriptorPoolCreateInfo &info);

    // vkResetDescriptorPool()
    void reset();

    // vkAllocDescriptorSets()
    std::vector<DescriptorSet *> alloc_sets(const Device &dev, VkDescriptorSetUsage usage, const std::vector<const DescriptorSetLayout *> &layouts);
    std::vector<DescriptorSet *> alloc_sets(const Device &dev, VkDescriptorSetUsage usage, const DescriptorSetLayout &layout, uint32_t count);
    DescriptorSet *alloc_sets(const Device &dev, VkDescriptorSetUsage usage, const DescriptorSetLayout &layout);

    // vkClearDescriptorSets()
    void clear_sets(const std::vector<DescriptorSet *> &sets);
    void clear_sets(DescriptorSet &set) { clear_sets(std::vector<DescriptorSet *>(1, &set)); }
};

class DescriptorSet : public DerivedObject<VkDescriptorSet, Object, VK_OBJECT_TYPE_DESCRIPTOR_SET> {
public:
    explicit DescriptorSet() : DerivedObject() {}
    explicit DescriptorSet(const Device &dev, VkDescriptorSet set) : DerivedObject(dev, set) {}
};

class DynamicVpStateObject : public DerivedObject<VkDynamicVpState, DynamicStateObject, VK_OBJECT_TYPE_DYNAMIC_VP_STATE> {
public:
    // vkCreateDynamicViewportState()
    void init(const Device &dev, const VkDynamicVpStateCreateInfo &info);
};

class DynamicRsStateObject : public DerivedObject<VkDynamicRsState, DynamicStateObject, VK_OBJECT_TYPE_DYNAMIC_RS_STATE> {
public:
    // vkCreateDynamicRasterState()
    void init(const Device &dev, const VkDynamicRsStateCreateInfo &info);
};

class DynamicCbStateObject : public DerivedObject<VkDynamicCbState, DynamicStateObject, VK_OBJECT_TYPE_DYNAMIC_CB_STATE> {
public:
    // vkCreateDynamicColorBlendState()
    void init(const Device &dev, const VkDynamicCbStateCreateInfo &info);
};

class DynamicDsStateObject : public DerivedObject<VkDynamicDsState, DynamicStateObject, VK_OBJECT_TYPE_DYNAMIC_DS_STATE> {
public:
    // vkCreateDynamicDepthStencilState()
    void init(const Device &dev, const VkDynamicDsStateCreateInfo &info);
};

class CmdBuffer : public DerivedObject<VkCmdBuffer, Object, VK_OBJECT_TYPE_COMMAND_BUFFER> {
public:
    explicit CmdBuffer() {}
    explicit CmdBuffer(const Device &dev, const VkCmdBufferCreateInfo &info) { init(dev, info); }

    // vkCreateCommandBuffer()
    void init(const Device &dev, const VkCmdBufferCreateInfo &info);

    // vkBeginCommandBuffer()
    void begin(const VkCmdBufferBeginInfo *info);
    void begin(VkRenderPass renderpass_obj, VkFramebuffer framebuffer_obj);
    void begin();

    // vkEndCommandBuffer()
    // vkResetCommandBuffer()
    void end();
    void reset();

    static VkCmdBufferCreateInfo create_info(uint32_t queueNodeIndex);
};

inline const void *Object::map(VkFlags flags) const
{
    return (primary_mem_) ? primary_mem_->map(flags) : NULL;
}

inline void *Object::map(VkFlags flags)
{
    return (primary_mem_) ? primary_mem_->map(flags) : NULL;
}

inline void Object::unmap() const
{
    if (primary_mem_)
        primary_mem_->unmap();
}

inline VkMemoryAllocInfo GpuMemory::alloc_info(const VkMemoryRequirements &reqs,
                                const VkMemoryAllocInfo *next_info)
{
    VkMemoryAllocInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    if (next_info != NULL)
        info.pNext = (void *) next_info;

    info.allocationSize = reqs.size;
    info.memProps = reqs.memPropsRequired;
    return info;
}

inline VkBufferCreateInfo Buffer::create_info(VkDeviceSize size, VkFlags usage)
{
    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    return info;
}

inline VkFenceCreateInfo Fence::create_info(VkFenceCreateFlags flags)
{
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = flags;
    return info;
}

inline VkFenceCreateInfo Fence::create_info()
{
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    return info;
}

inline VkSemaphoreCreateInfo Semaphore::create_info(uint32_t init_count, VkFlags flags)
{
    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.initialCount = init_count;
    info.flags = flags;
    return info;
}

inline VkEventCreateInfo Event::create_info(VkFlags flags)
{
    VkEventCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    info.flags = flags;
    return info;
}

inline VkQueryPoolCreateInfo QueryPool::create_info(VkQueryType type, uint32_t slot_count)
{
    VkQueryPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    info.queryType = type;
    info.slots = slot_count;
    return info;
}

inline VkImageCreateInfo Image::create_info()
{
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.extent.width = 1;
    info.extent.height = 1;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arraySize = 1;
    info.samples = 1;
    return info;
}

inline VkImageSubresource Image::subresource(VkImageAspect aspect, uint32_t mip_level, uint32_t array_slice)
{
    VkImageSubresource subres = {};
    subres.aspect = aspect;
    subres.mipLevel = mip_level;
    subres.arraySlice = array_slice;
    return subres;
}

inline VkImageSubresource Image::subresource(const VkImageSubresourceRange &range, uint32_t mip_level, uint32_t array_slice)
{
    return subresource(range.aspect, range.baseMipLevel + mip_level, range.baseArraySlice + array_slice);
}

inline VkImageSubresourceRange Image::subresource_range(VkImageAspect aspect, uint32_t base_mip_level, uint32_t mip_levels,
                                                                                     uint32_t base_array_slice, uint32_t array_size)
{
    VkImageSubresourceRange range = {};
    range.aspect = aspect;
    range.baseMipLevel = base_mip_level;
    range.mipLevels = mip_levels;
    range.baseArraySlice = base_array_slice;
    range.arraySize = array_size;
    return range;
}

inline VkImageSubresourceRange Image::subresource_range(const VkImageCreateInfo &info, VkImageAspect aspect)
{
    return subresource_range(aspect, 0, info.mipLevels, 0, info.arraySize);
}

inline VkImageSubresourceRange Image::subresource_range(const VkImageSubresource &subres)
{
    return subresource_range(subres.aspect, subres.mipLevel, 1, subres.arraySlice, 1);
}

inline VkExtent2D Image::extent(int32_t width, int32_t height)
{
    VkExtent2D extent = {};
    extent.width = width;
    extent.height = height;
    return extent;
}

inline VkExtent2D Image::extent(const VkExtent2D &extent, uint32_t mip_level)
{
    const int32_t width  = (extent.width  >> mip_level) ? extent.width  >> mip_level : 1;
    const int32_t height = (extent.height >> mip_level) ? extent.height >> mip_level : 1;
    return Image::extent(width, height);
}

inline VkExtent2D Image::extent(const VkExtent3D &extent)
{
    return Image::extent(extent.width, extent.height);
}

inline VkExtent3D Image::extent(int32_t width, int32_t height, int32_t depth)
{
    VkExtent3D extent = {};
    extent.width = width;
    extent.height = height;
    extent.depth = depth;
    return extent;
}

inline VkExtent3D Image::extent(const VkExtent3D &extent, uint32_t mip_level)
{
    const int32_t width  = (extent.width  >> mip_level) ? extent.width  >> mip_level : 1;
    const int32_t height = (extent.height >> mip_level) ? extent.height >> mip_level : 1;
    const int32_t depth  = (extent.depth  >> mip_level) ? extent.depth  >> mip_level : 1;
    return Image::extent(width, height, depth);
}

inline VkShaderCreateInfo Shader::create_info(size_t code_size, const void *code, VkFlags flags)
{
    VkShaderCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    info.codeSize = code_size;
    info.pCode = code;
    info.flags = flags;
    return info;
}

inline VkWriteDescriptorSet Device::write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                         VkDescriptorType type, uint32_t count, const VkDescriptorInfo *descriptors)
{
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.destSet = set.obj();
    write.destBinding = binding;
    write.destArrayElement = array_element;
    write.count = count;
    write.descriptorType = type;
    write.pDescriptors = descriptors;
    return write;
}

inline VkWriteDescriptorSet Device::write_descriptor_set(const DescriptorSet &set, uint32_t binding, uint32_t array_element,
                                                         VkDescriptorType type, const std::vector<VkDescriptorInfo> &descriptors)
{
    return write_descriptor_set(set, binding, array_element, type, descriptors.size(), &descriptors[0]);
}

inline VkCopyDescriptorSet Device::copy_descriptor_set(const DescriptorSet &src_set, uint32_t src_binding, uint32_t src_array_element,
                                                       const DescriptorSet &dst_set, uint32_t dst_binding, uint32_t dst_array_element,
                                                       uint32_t count)
{
    VkCopyDescriptorSet copy = {};
    copy.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
    copy.srcSet = src_set.obj();
    copy.srcBinding = src_binding;
    copy.srcArrayElement = src_array_element;
    copy.destSet = dst_set.obj();
    copy.destBinding = dst_binding;
    copy.destArrayElement = dst_array_element;
    copy.count = count;

    return copy;
}

inline VkCmdBufferCreateInfo CmdBuffer::create_info(uint32_t queueNodeIndex)
{
    VkCmdBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
    info.queueNodeIndex = queueNodeIndex;
    return info;
}

}; // namespace vk_testing

#endif // VKTESTBINDING_H
