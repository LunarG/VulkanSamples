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

#include <iostream>
#include <string.h> // memset(), memcmp()
#include <assert.h>
#include "vktestbinding.h"

namespace {

#define DERIVED_OBJECT_TYPE_INIT(create_func, dev, vk_object_type, ...)         \
    do {                                                                        \
        obj_type obj;                                                           \
        dev_ = &dev;                                                        \
        if (EXPECT(create_func(dev.obj(), __VA_ARGS__, &obj) == VK_SUCCESS))    \
            base_type::init(obj, vk_object_type);                               \
    } while (0)

#define STRINGIFY(x) #x
#define EXPECT(expr) ((expr) ? true : expect_failure(STRINGIFY(expr), __FILE__, __LINE__, __FUNCTION__))


vk_testing::ErrorCallback error_callback;

bool expect_failure(const char *expr, const char *file, unsigned int line, const char *function)
{
    if (error_callback) {
        error_callback(expr, file, line, function);
    } else {
        std::cerr << file << ":" << line << ": " << function <<
            ": Expectation `" << expr << "' failed.\n";
    }

    return false;
}

template<class T, class S>
std::vector<T> make_objects(const std::vector<S> &v)
{
    std::vector<T> objs;
    objs.reserve(v.size());
    for (typename std::vector<S>::const_iterator it = v.begin(); it != v.end(); it++)
        objs.push_back((*it)->obj());
    return objs;
}

template<typename T>
std::vector<T> get_info(VkPhysicalDevice gpu, VkPhysicalDeviceInfoType type, size_t min_elems)
{
    std::vector<T> info;
    size_t size;
    if (EXPECT(vkGetPhysicalDeviceInfo(gpu, type, &size, NULL) == VK_SUCCESS && size % sizeof(T) == 0)) {
        info.resize(size / sizeof(T));
        if (!EXPECT(vkGetPhysicalDeviceInfo(gpu, type, &size, &info[0]) == VK_SUCCESS && size == info.size() * sizeof(T)))
            info.clear();
    }

    if (info.size() < min_elems)
        info.resize(min_elems);

    return info;
}

template<typename T>
std::vector<T> get_info(VkDevice device, VkObjectType object_type, VkObject obj, VkObjectInfoType type, size_t min_elems)
{
    std::vector<T> info;
    size_t size;
    if (EXPECT(vkGetObjectInfo(device, object_type, obj, type, &size, NULL) == VK_SUCCESS && size % sizeof(T) == 0)) {
        info.resize(size / sizeof(T));
        if (!EXPECT(vkGetObjectInfo(device, object_type, obj, type, &size, &info[0]) == VK_SUCCESS && size == info.size() * sizeof(T)))
            info.clear();
    }

    if (info.size() < min_elems)
        info.resize(min_elems);

    return info;
}

} // namespace

namespace vk_testing {

void set_error_callback(ErrorCallback callback)
{
    error_callback = callback;
}

VkPhysicalDeviceProperties PhysicalGpu::properties() const
{
    return get_info<VkPhysicalDeviceProperties>(gpu_, VK_PHYSICAL_DEVICE_INFO_TYPE_PROPERTIES, 1)[0];
}

VkPhysicalDevicePerformance PhysicalGpu::performance() const
{
    return get_info<VkPhysicalDevicePerformance>(gpu_, VK_PHYSICAL_DEVICE_INFO_TYPE_PERFORMANCE, 1)[0];
}

std::vector<VkPhysicalDeviceQueueProperties> PhysicalGpu::queue_properties() const
{
    return get_info<VkPhysicalDeviceQueueProperties>(gpu_, VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES, 0);
}

VkPhysicalDeviceMemoryProperties PhysicalGpu::memory_properties() const
{
    return get_info<VkPhysicalDeviceMemoryProperties>(gpu_, VK_PHYSICAL_DEVICE_INFO_TYPE_MEMORY_PROPERTIES, 1)[0];
}

void PhysicalGpu::add_extension_dependencies(
        uint32_t dependency_count,
        VkExtensionProperties *depencency_props,
        std::vector<VkExtensionProperties> &ext_list)
{
    for (uint32_t i = 0; i < dependency_count; i++) {

    }
}

std::vector<VkExtensionProperties> PhysicalGpu::extensions() const
{
    // Extensions to enable
    static const char *known_exts[] = {
        "VK_WSI_LunarG",
    };
    std::vector<VkExtensionProperties> exts;
    size_t extSize = sizeof(uint32_t);
    uint32_t extCount = 0;
    if (!EXPECT(vkGetGlobalExtensionInfo(VK_EXTENSION_INFO_TYPE_COUNT, 0, &extSize, &extCount) == VK_SUCCESS))
        return exts;

    VkExtensionProperties extProp;
    extSize = sizeof(VkExtensionProperties);
    // TODO : Need to update this if/when we have more than 1 extension to enable
    for (uint32_t i = 0; i < extCount; i++) {
        if (!EXPECT(vkGetGlobalExtensionInfo(VK_EXTENSION_INFO_TYPE_PROPERTIES, i, &extSize, &extProp) == VK_SUCCESS))
            return exts;

        if (!strcmp(known_exts[0], extProp.name))
            exts.push_back(extProp);
    }

    return exts;
}

void BaseObject::init(VkObject obj, VkObjectType type, bool own)
{
    EXPECT(!initialized());
    reinit(obj, type, own);
}

void BaseObject::reinit(VkObject obj, VkObjectType type, bool own)
{
    obj_ = obj;
    object_type_ = type;
    own_obj_ = own;
}

uint32_t Object::memory_allocation_count() const
{
    return 1;
}

std::vector<VkMemoryRequirements> Object::memory_requirements() const
{
    uint32_t num_allocations = 1;
    std::vector<VkMemoryRequirements> info =
        get_info<VkMemoryRequirements>(dev_->obj(), type(), obj(), VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS, 0);
    EXPECT(info.size() == num_allocations);
    if (info.size() == 1 && !info[0].size)
        info.clear();

    return info;
}

void Object::init(VkObject obj, VkObjectType object_type, bool own)
{
    BaseObject::init(obj, object_type, own);
    mem_alloc_count_ = memory_allocation_count();
}

void Object::reinit(VkObject obj, VkObjectType object_type, bool own)
{
    cleanup();
    BaseObject::reinit(obj, object_type, own);
    mem_alloc_count_ = memory_allocation_count();
}

void Object::cleanup()
{
    if (!initialized())
        return;

    if (own())
        EXPECT(vkDestroyObject(dev_->obj(), type(), obj()) == VK_SUCCESS);

    if (internal_mems_) {
        delete[] internal_mems_;
        internal_mems_ = NULL;
        primary_mem_ = NULL;
    }

    mem_alloc_count_ = 0;
}

void Object::bind_memory(const GpuMemory &mem, VkDeviceSize mem_offset)
{
    bound = true;
    EXPECT(vkBindObjectMemory(dev_->obj(), type(), obj(), mem.obj(), mem_offset) == VK_SUCCESS);
}

void Object::alloc_memory()
{
    if (!EXPECT(!internal_mems_) || !mem_alloc_count_)
        return;

    internal_mems_ = new GpuMemory[mem_alloc_count_];

    const std::vector<VkMemoryRequirements> mem_reqs = memory_requirements();
    VkMemoryAllocInfo info, *next_info = NULL;

    for (int i = 0; i < mem_reqs.size(); i++) {
        info = GpuMemory::alloc_info(mem_reqs[i], next_info);
        primary_mem_ = &internal_mems_[i];
        internal_mems_[i].init(*dev_, info);
        bind_memory(internal_mems_[i], 0);
    }
}

void Object::alloc_memory(VkMemoryPropertyFlags &reqs)
{
    if (!EXPECT(!internal_mems_) || !mem_alloc_count_)
        return;

    internal_mems_ = new GpuMemory[mem_alloc_count_];

    std::vector<VkMemoryRequirements> mem_reqs = memory_requirements();
    VkMemoryAllocInfo info, *next_info = NULL;

    for (int i = 0; i < mem_reqs.size(); i++) {
        mem_reqs[i].memPropsRequired |= reqs;
        info = GpuMemory::alloc_info(mem_reqs[i], next_info);
        primary_mem_ = &internal_mems_[i];
        internal_mems_[i].init(*dev_, info);
        bind_memory(internal_mems_[i], 0);
    }
}

void Object::alloc_memory(const std::vector<VkDeviceMemory> &mems)
{
    if (!EXPECT(!internal_mems_) || !mem_alloc_count_)
        return;

    internal_mems_ = new GpuMemory[mem_alloc_count_];

    const std::vector<VkMemoryRequirements> mem_reqs = memory_requirements();
    if (!EXPECT(mem_reqs.size() == mems.size()))
        return;

    for (int i = 0; i < mem_reqs.size(); i++) {
        primary_mem_ = &internal_mems_[i];

        internal_mems_[i].init(*dev_, mems[i]);
        bind_memory(internal_mems_[i], 0);
    }
}

std::vector<VkDeviceMemory> Object::memories() const
{
    std::vector<VkDeviceMemory> mems;
    if (internal_mems_) {
        mems.reserve(mem_alloc_count_);
        for (uint32_t i = 0; i < mem_alloc_count_; i++)
            mems.push_back(internal_mems_[i].obj());
    }

    return mems;
}

Device::~Device()
{
    if (!initialized())
        return;

    for (int i = 0; i < QUEUE_COUNT; i++) {
        for (std::vector<Queue *>::iterator it = queues_[i].begin(); it != queues_[i].end(); it++)
            delete *it;
        queues_[i].clear();
    }

    EXPECT(vkDestroyDevice(obj()) == VK_SUCCESS);
}

void Device::init(std::vector<VkExtensionProperties> extensions)
{
    // request all queues
    const std::vector<VkPhysicalDeviceQueueProperties> queue_props = gpu_.queue_properties();
    std::vector<VkDeviceQueueCreateInfo> queue_info;
    queue_info.reserve(queue_props.size());
    for (int i = 0; i < queue_props.size(); i++) {
        VkDeviceQueueCreateInfo qi = {};
        qi.queueNodeIndex = i;
        qi.queueCount = queue_props[i].queueCount;
        if (queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_queue_node_index_ = i;
        }
        queue_info.push_back(qi);
    }

    VkDeviceCreateInfo dev_info = {};
    dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_info.pNext = NULL;
    dev_info.queueRecordCount = queue_info.size();
    dev_info.pRequestedQueues = &queue_info[0];
    dev_info.extensionCount = extensions.size();
    dev_info.pEnabledExtensions = &extensions[0];
    dev_info.flags = 0;

    init(dev_info);
}

void Device::init(const VkDeviceCreateInfo &info)
{
    VkDevice obj;
    if (EXPECT(vkCreateDevice(gpu_.obj(), &info, &obj) == VK_SUCCESS)) {
        base_type::init(obj, VK_OBJECT_TYPE_DEVICE);
    }

    init_queues();
    init_formats();
}

void Device::init_queues()
{
    VkResult err;
    size_t data_size;
    uint32_t queue_node_count;

    err = vkGetPhysicalDeviceInfo(gpu_.obj(), VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES,
                        &data_size, NULL);
    EXPECT(err == VK_SUCCESS);

    queue_node_count = data_size / sizeof(VkPhysicalDeviceQueueProperties);
    EXPECT(queue_node_count >= 1);

    VkPhysicalDeviceQueueProperties* queue_props = new VkPhysicalDeviceQueueProperties[queue_node_count];

    err = vkGetPhysicalDeviceInfo(gpu_.obj(), VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES,
                        &data_size, queue_props);
    EXPECT(err == VK_SUCCESS);

    for (uint32_t i = 0; i < queue_node_count; i++) {
        VkQueue queue;

        for (uint32_t j = 0; j < queue_props[i].queueCount; j++) {
            // TODO: Need to add support for separate MEMMGR and work queues, including synchronization
            err = vkGetDeviceQueue(obj(), i, j, &queue);
            EXPECT(err == VK_SUCCESS);

            if (queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queues_[GRAPHICS].push_back(new Queue(queue));
            }

            if (queue_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                queues_[COMPUTE].push_back(new Queue(queue));
            }

            if (queue_props[i].queueFlags & VK_QUEUE_DMA_BIT) {
                queues_[DMA].push_back(new Queue(queue));
            }
        }
    }

    delete[] queue_props;

    EXPECT(!queues_[GRAPHICS].empty() || !queues_[COMPUTE].empty());
}

void Device::init_formats()
{
    for (int f = VK_FORMAT_BEGIN_RANGE; f <= VK_FORMAT_END_RANGE; f++) {
        const VkFormat fmt = static_cast<VkFormat>(f);
        const VkFormatProperties props = format_properties(fmt);

        if (props.linearTilingFeatures) {
            const Format tmp = { fmt, VK_IMAGE_TILING_LINEAR, props.linearTilingFeatures };
            formats_.push_back(tmp);
        }

        if (props.optimalTilingFeatures) {
            const Format tmp = { fmt, VK_IMAGE_TILING_OPTIMAL, props.optimalTilingFeatures };
            formats_.push_back(tmp);
        }
    }

    EXPECT(!formats_.empty());
}

VkFormatProperties Device::format_properties(VkFormat format)
{
    VkFormatProperties data;
    if (!EXPECT(vkGetPhysicalDeviceFormatInfo(gpu().obj(), format, &data) == VK_SUCCESS))
        memset(&data, 0, sizeof(data));

    return data;
}

void Device::wait()
{
    EXPECT(vkDeviceWaitIdle(obj()) == VK_SUCCESS);
}

VkResult Device::wait(const std::vector<const Fence *> &fences, bool wait_all, uint64_t timeout)
{
    const std::vector<VkFence> fence_objs = make_objects<VkFence>(fences);
    VkResult err = vkWaitForFences(obj(), fence_objs.size(), &fence_objs[0], wait_all, timeout);
    EXPECT(err == VK_SUCCESS || err == VK_TIMEOUT);

    return err;
}

VkResult Device::update_descriptor_sets(const std::vector<VkWriteDescriptorSet> &writes, const std::vector<VkCopyDescriptorSet> &copies)
{
    return vkUpdateDescriptorSets(obj(), writes.size(), &writes[0], copies.size(), &copies[0]);
}

void Queue::submit(const std::vector<const CmdBuffer *> &cmds, Fence &fence)
{
    const std::vector<VkCmdBuffer> cmd_objs = make_objects<VkCmdBuffer>(cmds);
    EXPECT(vkQueueSubmit(obj(), cmd_objs.size(), &cmd_objs[0], fence.obj()) == VK_SUCCESS);
}

void Queue::submit(const CmdBuffer &cmd, Fence &fence)
{
    submit(std::vector<const CmdBuffer*>(1, &cmd), fence);
}

void Queue::submit(const CmdBuffer &cmd)
{
    Fence fence;
    submit(cmd, fence);
}

void Queue::wait()
{
    EXPECT(vkQueueWaitIdle(obj()) == VK_SUCCESS);
}

void Queue::signal_semaphore(Semaphore &sem)
{
    EXPECT(vkQueueSignalSemaphore(obj(), sem.obj()) == VK_SUCCESS);
}

void Queue::wait_semaphore(Semaphore &sem)
{
    EXPECT(vkQueueWaitSemaphore(obj(), sem.obj()) == VK_SUCCESS);
}

GpuMemory::~GpuMemory()
{
    if (initialized() && own())
        EXPECT(vkFreeMemory(dev_->obj(), obj()) == VK_SUCCESS);
}

void GpuMemory::init(const Device &dev, const VkMemoryAllocInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkAllocMemory, dev, VK_OBJECT_TYPE_DEVICE_MEMORY, &info);
}

void GpuMemory::init(const Device &dev, VkDeviceMemory mem)
{
    dev_ = &dev;
    BaseObject::init(mem, VK_OBJECT_TYPE_DEVICE_MEMORY, false);
}

const void *GpuMemory::map(VkFlags flags) const
{
    void *data;
    if (!EXPECT(vkMapMemory(dev_->obj(), obj(), 0 ,0, flags, &data) == VK_SUCCESS))
        data = NULL;

    return data;
}

void *GpuMemory::map(VkFlags flags)
{
    void *data;
    if (!EXPECT(vkMapMemory(dev_->obj(), obj(), 0, 0, flags, &data) == VK_SUCCESS))
        data = NULL;

    return data;
}

void GpuMemory::unmap() const
{
    EXPECT(vkUnmapMemory(dev_->obj(), obj()) == VK_SUCCESS);
}

void Fence::init(const Device &dev, const VkFenceCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateFence, dev, VK_OBJECT_TYPE_FENCE, &info);
    alloc_memory();
}

void Semaphore::init(const Device &dev, const VkSemaphoreCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateSemaphore, dev, VK_OBJECT_TYPE_SEMAPHORE, &info);
    alloc_memory();
}

void Event::init(const Device &dev, const VkEventCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateEvent, dev, VK_OBJECT_TYPE_EVENT, &info);
    alloc_memory();
}

void Event::set()
{
    EXPECT(vkSetEvent(dev_->obj(), obj()) == VK_SUCCESS);
}

void Event::reset()
{
    EXPECT(vkResetEvent(dev_->obj(), obj()) == VK_SUCCESS);
}

void QueryPool::init(const Device &dev, const VkQueryPoolCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateQueryPool, dev, VK_OBJECT_TYPE_QUERY_POOL, &info);
    alloc_memory();
}

VkResult QueryPool::results(uint32_t start, uint32_t count, size_t size, void *data)
{
    size_t tmp = size;
    VkResult err = vkGetQueryPoolResults(dev_->obj(), obj(), start, count, &tmp, data, 0);
    if (err == VK_SUCCESS) {
        if (!EXPECT(tmp == size))
            memset(data, 0, size);
    } else {
        EXPECT(err == VK_NOT_READY);
    }

    return err;
}

void Buffer::init(const Device &dev, const VkBufferCreateInfo &info)
{
    init_no_mem(dev, info);
    alloc_memory();
}

void Buffer::init(const Device &dev, const VkBufferCreateInfo &info, VkMemoryPropertyFlags &reqs)
{
    init_no_mem(dev, info);
    alloc_memory(reqs);
}

void Buffer::init_no_mem(const Device &dev, const VkBufferCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateBuffer, dev, VK_OBJECT_TYPE_BUFFER, &info);
    create_info_ = info;
}

void Buffer::bind_memory(VkDeviceSize offset, VkDeviceSize size,
                         const GpuMemory &mem, VkDeviceSize mem_offset)
{
    VkQueue queue = dev_->graphics_queues()[0]->obj();
    EXPECT(vkQueueBindSparseBufferMemory(queue, obj(), offset, size, mem.obj(), mem_offset) == VK_SUCCESS);
}

void BufferView::init(const Device &dev, const VkBufferViewCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateBufferView, dev, VK_OBJECT_TYPE_BUFFER_VIEW, &info);
    alloc_memory();
}

void Image::init(const Device &dev, const VkImageCreateInfo &info)
{
    init_no_mem(dev, info);
    alloc_memory();
}

void Image::init(const Device &dev, const VkImageCreateInfo &info, VkMemoryPropertyFlags &reqs)
{
    init_no_mem(dev, info);
    alloc_memory(reqs);
}

void Image::init_no_mem(const Device &dev, const VkImageCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateImage, dev, VK_OBJECT_TYPE_IMAGE, &info);
    init_info(dev, info);
}

void Image::init_info(const Device &dev, const VkImageCreateInfo &info)
{
    create_info_ = info;

    for (std::vector<Device::Format>::const_iterator it = dev.formats().begin(); it != dev.formats().end(); it++) {
        if (memcmp(&it->format, &create_info_.format, sizeof(it->format)) == 0 && it->tiling == create_info_.tiling) {
            format_features_ = it->features;
            break;
        }
    }
}

void Image::bind_memory(const Device &dev, const VkImageMemoryBindInfo &info,
                        const GpuMemory &mem, VkDeviceSize mem_offset)
{
    VkQueue queue = dev.graphics_queues()[0]->obj();
    EXPECT(vkQueueBindSparseImageMemory(queue, obj(), &info, mem.obj(), mem_offset) == VK_SUCCESS);
}

VkSubresourceLayout Image::subresource_layout(const VkImageSubresource &subres) const
{
    const VkSubresourceInfoType type = VK_SUBRESOURCE_INFO_TYPE_LAYOUT;
    VkSubresourceLayout data;
    size_t size = sizeof(data);
    if (!EXPECT(vkGetImageSubresourceInfo(dev_->obj(), obj(), &subres, type, &size, &data) == VK_SUCCESS && size == sizeof(data)))
        memset(&data, 0, sizeof(data));

    return data;
}

bool Image::transparent() const
{
    return (create_info_.tiling == VK_IMAGE_TILING_LINEAR &&
            create_info_.samples == 1 &&
            !(create_info_.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                    VK_IMAGE_USAGE_DEPTH_STENCIL_BIT)));
}

void ImageView::init(const Device &dev, const VkImageViewCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateImageView, dev, VK_OBJECT_TYPE_IMAGE_VIEW, &info);
    alloc_memory();
}

void ColorAttachmentView::init(const Device &dev, const VkColorAttachmentViewCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateColorAttachmentView, dev, VK_OBJECT_TYPE_COLOR_ATTACHMENT_VIEW, &info);
    alloc_memory();
}

void DepthStencilView::init(const Device &dev, const VkDepthStencilViewCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateDepthStencilView, dev, VK_OBJECT_TYPE_DEPTH_STENCIL_VIEW, &info);
    alloc_memory();
}

void Shader::init(const Device &dev, const VkShaderCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateShader, dev, VK_OBJECT_TYPE_SHADER, &info);
}

VkResult Shader::init_try(const Device &dev, const VkShaderCreateInfo &info)
{
    /*
     * Note: Cannot use DERIVED_OBJECT_TYPE_INIT as we need the
     * return code.
     */
    VkShader sh;
    dev_ = &dev;
    VkResult err = vkCreateShader(dev.obj(), &info, &sh);
    if (err == VK_SUCCESS)
        Object::init(sh, VK_OBJECT_TYPE_SHADER);

    return err;
}

void Pipeline::init(const Device &dev, const VkGraphicsPipelineCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateGraphicsPipeline, dev, VK_OBJECT_TYPE_PIPELINE, &info);
    alloc_memory();
}

VkResult Pipeline::init_try(const Device &dev, const VkGraphicsPipelineCreateInfo &info)
{
    VkPipeline pipe;
    dev_ = &dev;
    VkResult err = vkCreateGraphicsPipeline(dev.obj(), &info, &pipe);
    if (err == VK_SUCCESS) {
        Object::init(pipe, VK_OBJECT_TYPE_PIPELINE);
        alloc_memory();
    }

    return err;
}

void Pipeline::init(
        const Device &dev,
        const VkGraphicsPipelineCreateInfo &info,
        const VkPipeline basePipeline)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateGraphicsPipelineDerivative, dev, VK_OBJECT_TYPE_PIPELINE, &info, basePipeline);
    alloc_memory();
}

void Pipeline::init(const Device &dev, const VkComputePipelineCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateComputePipeline, dev, VK_OBJECT_TYPE_PIPELINE, &info);
    alloc_memory();
}

void Pipeline::init(const Device&dev, size_t size, const void *data)
{
    DERIVED_OBJECT_TYPE_INIT(vkLoadPipeline, dev, VK_OBJECT_TYPE_PIPELINE, size, data);
    alloc_memory();
}

void Pipeline::init(
        const Device&dev,
        size_t size,
        const void *data,
        const VkPipeline basePipeline)
{
    DERIVED_OBJECT_TYPE_INIT(vkLoadPipelineDerivative, dev, VK_OBJECT_TYPE_PIPELINE, size, data, basePipeline);
    alloc_memory();
}

size_t Pipeline::store(size_t size, void *data)
{
    if (!EXPECT(vkStorePipeline(dev_->obj(), obj(), &size, data) == VK_SUCCESS))
        size = 0;

    return size;
}

void Sampler::init(const Device &dev, const VkSamplerCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateSampler, dev, VK_OBJECT_TYPE_SAMPLER, &info);
    alloc_memory();
}

void DescriptorSetLayout::init(const Device &dev, const VkDescriptorSetLayoutCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateDescriptorSetLayout, dev, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, &info);
    alloc_memory();
}

void PipelineLayout::init(const Device &dev, VkPipelineLayoutCreateInfo &info, const std::vector<const DescriptorSetLayout *> &layouts)
{
    const std::vector<VkDescriptorSetLayout> layout_objs = make_objects<VkDescriptorSetLayout>(layouts);
    info.pSetLayouts = &layout_objs[0];

    DERIVED_OBJECT_TYPE_INIT(vkCreatePipelineLayout, dev, VK_OBJECT_TYPE_PIPELINE_LAYOUT, &info);
    alloc_memory();
}

void DescriptorPool::init(const Device &dev, VkDescriptorPoolUsage usage,
                          uint32_t max_sets, const VkDescriptorPoolCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateDescriptorPool, dev, VK_OBJECT_TYPE_DESCRIPTOR_POOL, usage, max_sets, &info);
    alloc_memory();
}

void DescriptorPool::reset()
{
    EXPECT(vkResetDescriptorPool(dev_->obj(), obj()) == VK_SUCCESS);
}

std::vector<DescriptorSet *> DescriptorPool::alloc_sets(const Device &dev, VkDescriptorSetUsage usage, const std::vector<const DescriptorSetLayout *> &layouts)
{
    const std::vector<VkDescriptorSetLayout> layout_objs = make_objects<VkDescriptorSetLayout>(layouts);

    std::vector<VkDescriptorSet> set_objs;
    set_objs.resize(layout_objs.size());

    uint32_t set_count;
    VkResult err = vkAllocDescriptorSets(dev_->obj(), obj(), usage, layout_objs.size(), &layout_objs[0], &set_objs[0], &set_count);
    if (err == VK_SUCCESS)
        EXPECT(set_count == set_objs.size());
    set_objs.resize(set_count);

    std::vector<DescriptorSet *> sets;
    sets.reserve(set_count);
    for (std::vector<VkDescriptorSet>::const_iterator it = set_objs.begin(); it != set_objs.end(); it++) {
        // do descriptor sets need memories bound?
        DescriptorSet *descriptorSet = new DescriptorSet(dev, *it);
        sets.push_back(descriptorSet);
    }
    return sets;
}

std::vector<DescriptorSet *> DescriptorPool::alloc_sets(const Device &dev, VkDescriptorSetUsage usage, const DescriptorSetLayout &layout, uint32_t count)
{
    return alloc_sets(dev, usage, std::vector<const DescriptorSetLayout *>(count, &layout));
}

DescriptorSet *DescriptorPool::alloc_sets(const Device &dev, VkDescriptorSetUsage usage, const DescriptorSetLayout &layout)
{
    std::vector<DescriptorSet *> set = alloc_sets(dev, usage, layout, 1);
    return (set.empty()) ? NULL : set[0];
}

void DynamicVpStateObject::init(const Device &dev, const VkDynamicVpStateCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateDynamicViewportState, dev, VK_OBJECT_TYPE_DYNAMIC_VP_STATE, &info);
    alloc_memory();
}

void DynamicRsStateObject::init(const Device &dev, const VkDynamicRsStateCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateDynamicRasterState, dev, VK_OBJECT_TYPE_DYNAMIC_RS_STATE, &info);
    alloc_memory();
}

void DynamicCbStateObject::init(const Device &dev, const VkDynamicCbStateCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateDynamicColorBlendState, dev, VK_OBJECT_TYPE_DYNAMIC_CB_STATE, &info);
    alloc_memory();
}

void DynamicDsStateObject::init(const Device &dev, const VkDynamicDsStateCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateDynamicDepthStencilState, dev, VK_OBJECT_TYPE_DYNAMIC_DS_STATE, &info);
    alloc_memory();
}

void CmdBuffer::init(const Device &dev, const VkCmdBufferCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateCommandBuffer, dev, VK_OBJECT_TYPE_COMMAND_BUFFER, &info);
}

void CmdBuffer::begin(const VkCmdBufferBeginInfo *info)
{
    EXPECT(vkBeginCommandBuffer(obj(), info) == VK_SUCCESS);
}

void CmdBuffer::begin(VkRenderPass renderpass_obj, VkFramebuffer framebuffer_obj)
{
    VkCmdBufferBeginInfo info = {};
    VkCmdBufferGraphicsBeginInfo graphics_cmd_buf_info = {};
    graphics_cmd_buf_info.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO;
    graphics_cmd_buf_info.pNext = NULL;
    graphics_cmd_buf_info.renderPassContinue.renderPass = renderpass_obj;
    graphics_cmd_buf_info.renderPassContinue.framebuffer = framebuffer_obj;

    info.flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT |
          VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT;
    info.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;
    info.pNext = &graphics_cmd_buf_info;

    begin(&info);
}

void CmdBuffer::begin()
{
    VkCmdBufferBeginInfo info = {};
    info.flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT |
          VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT;
    info.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;

    begin(&info);
}

void CmdBuffer::end()
{
    EXPECT(vkEndCommandBuffer(obj()) == VK_SUCCESS);
}

void CmdBuffer::reset()
{
    EXPECT(vkResetCommandBuffer(obj()) == VK_SUCCESS);
}

}; // namespace vk_testing
