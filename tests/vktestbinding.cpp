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
#include "vktestbinding.h"

namespace {

#define DERIVED_OBJECT_INIT(create_func, ...)                       \
    do {                                                            \
        obj_type obj;                                               \
        if (EXPECT(create_func(__VA_ARGS__, &obj) == VK_SUCCESS))  \
            base_type::init(obj);                                   \
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
std::vector<T> get_info(VkPhysicalGpu gpu, VkPhysicalGpuInfoType type, size_t min_elems)
{
    std::vector<T> info;
    size_t size;
    if (EXPECT(vkGetGpuInfo(gpu, type, &size, NULL) == VK_SUCCESS && size % sizeof(T) == 0)) {
        info.resize(size / sizeof(T));
        if (!EXPECT(vkGetGpuInfo(gpu, type, &size, &info[0]) == VK_SUCCESS && size == info.size() * sizeof(T)))
            info.clear();
    }

    if (info.size() < min_elems)
        info.resize(min_elems);

    return info;
}

template<typename T>
std::vector<T> get_info(VkBaseObject obj, VkObjectInfoType type, size_t min_elems)
{
    std::vector<T> info;
    size_t size;
    if (EXPECT(vkGetObjectInfo(obj, type, &size, NULL) == VK_SUCCESS && size % sizeof(T) == 0)) {
        info.resize(size / sizeof(T));
        if (!EXPECT(vkGetObjectInfo(obj, type, &size, &info[0]) == VK_SUCCESS && size == info.size() * sizeof(T)))
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

VkPhysicalGpuProperties PhysicalGpu::properties() const
{
    return get_info<VkPhysicalGpuProperties>(gpu_, VK_INFO_TYPE_PHYSICAL_GPU_PROPERTIES, 1)[0];
}

VkPhysicalGpuPerformance PhysicalGpu::performance() const
{
    return get_info<VkPhysicalGpuPerformance>(gpu_, VK_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE, 1)[0];
}

std::vector<VkPhysicalGpuQueueProperties> PhysicalGpu::queue_properties() const
{
    return get_info<VkPhysicalGpuQueueProperties>(gpu_, VK_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES, 0);
}

VkPhysicalGpuMemoryProperties PhysicalGpu::memory_properties() const
{
    return get_info<VkPhysicalGpuMemoryProperties>(gpu_, VK_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES, 1)[0];
}

std::vector<const char *> PhysicalGpu::layers(std::vector<char> &buf) const
{
    const size_t max_layer_count = 16;
    const size_t max_string_size = 256;

    buf.resize(max_layer_count * max_string_size);

    std::vector<const char *> layers;
    layers.reserve(max_layer_count);
    for (size_t i = 0; i < max_layer_count; i++)
        layers.push_back(&buf[0] + max_string_size * i);

    char * const *out = const_cast<char * const *>(&layers[0]);
    size_t count;
    if (!EXPECT(vkEnumerateLayers(gpu_, max_layer_count, max_string_size, &count, out, NULL) == VK_SUCCESS))
        count = 0;
    layers.resize(count);

    return layers;
}

std::vector<const char *> PhysicalGpu::extensions() const
{
    static const char *known_exts[] = {
        "VK_WSI_X11",
    };

    std::vector<const char *> exts;
    for (int i = 0; i < sizeof(known_exts) / sizeof(known_exts[0]); i++) {
        VkResult err = vkGetExtensionSupport(gpu_, known_exts[i]);
        if (err == VK_SUCCESS)
            exts.push_back(known_exts[i]);
    }

    return exts;
}

VkGpuCompatibilityInfo PhysicalGpu::compatibility(const PhysicalGpu &other) const
{
    VkGpuCompatibilityInfo data;
    if (!EXPECT(vkGetMultiGpuCompatibility(gpu_, other.gpu_, &data) == VK_SUCCESS))
        memset(&data, 0, sizeof(data));

    return data;
}

void BaseObject::init(VkBaseObject obj, bool own)
{
    EXPECT(!initialized());
    reinit(obj, own);
}

void BaseObject::reinit(VkBaseObject obj, bool own)
{
    obj_ = obj;
    own_obj_ = own;
}

uint32_t BaseObject::memory_allocation_count() const
{
    return get_info<uint32_t>(obj_, VK_INFO_TYPE_MEMORY_ALLOCATION_COUNT, 1)[0];
}

std::vector<VkMemoryRequirements> BaseObject::memory_requirements() const
{
    VkResult err;
    uint32_t num_allocations = 0;
    size_t num_alloc_size = sizeof(num_allocations);
    err = vkGetObjectInfo(obj_, VK_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                           &num_alloc_size, &num_allocations);
    EXPECT(err == VK_SUCCESS && num_alloc_size == sizeof(num_allocations));
    std::vector<VkMemoryRequirements> info =
        get_info<VkMemoryRequirements>(obj_, VK_INFO_TYPE_MEMORY_REQUIREMENTS, 0);
    EXPECT(info.size() == num_allocations);
    if (info.size() == 1 && !info[0].size)
        info.clear();

    return info;
}

void Object::init(VkObject obj, bool own)
{
    BaseObject::init(obj, own);
    mem_alloc_count_ = memory_allocation_count();
}

void Object::reinit(VkObject obj, bool own)
{
    cleanup();
    BaseObject::reinit(obj, own);
    mem_alloc_count_ = memory_allocation_count();
}

void Object::cleanup()
{
    if (!initialized())
        return;

    if(bound) {
        unbind_memory();
    }

    if (internal_mems_) {
        delete[] internal_mems_;
        internal_mems_ = NULL;
        primary_mem_ = NULL;
    }

    mem_alloc_count_ = 0;

    if (own())
        EXPECT(vkDestroyObject(obj()) == VK_SUCCESS);
}

void Object::bind_memory(uint32_t alloc_idx, const GpuMemory &mem, VkGpuSize mem_offset)
{
    bound = true;
    EXPECT(vkBindObjectMemory(obj(), alloc_idx, mem.obj(), mem_offset) == VK_SUCCESS);
}

void Object::bind_memory(uint32_t alloc_idx, VkGpuSize offset, VkGpuSize size,
                         const GpuMemory &mem, VkGpuSize mem_offset)
{
    bound = true;
    EXPECT(!alloc_idx && vkBindObjectMemoryRange(obj(), 0, offset, size, mem.obj(), mem_offset) == VK_SUCCESS);
}

void Object::unbind_memory(uint32_t alloc_idx)
{
    EXPECT(vkBindObjectMemory(obj(), alloc_idx, VK_NULL_HANDLE, 0) == VK_SUCCESS);
}

void Object::unbind_memory()
{
    for (uint32_t i = 0; i < mem_alloc_count_; i++)
        unbind_memory(i);
}

void Object::alloc_memory(const Device &dev)
{
    if (!EXPECT(!internal_mems_) || !mem_alloc_count_)
        return;

    internal_mems_ = new GpuMemory[mem_alloc_count_];

    const std::vector<VkMemoryRequirements> mem_reqs = memory_requirements();
    VkMemoryAllocInfo info, *next_info = NULL;

    for (int i = 0; i < mem_reqs.size(); i++) {
        info = GpuMemory::alloc_info(mem_reqs[i], next_info);
        primary_mem_ = &internal_mems_[i];
        internal_mems_[i].init(dev, info);
        bind_memory(i, internal_mems_[i], 0);
    }
}

void Object::alloc_memory(const std::vector<VkGpuMemory> &mems)
{
    if (!EXPECT(!internal_mems_) || !mem_alloc_count_)
        return;

    internal_mems_ = new GpuMemory[mem_alloc_count_];

    const std::vector<VkMemoryRequirements> mem_reqs = memory_requirements();
    if (!EXPECT(mem_reqs.size() == mems.size()))
        return;

    for (int i = 0; i < mem_reqs.size(); i++) {
        primary_mem_ = &internal_mems_[i];

        internal_mems_[i].init(mems[i]);
        bind_memory(i, internal_mems_[i], 0);
    }
}

std::vector<VkGpuMemory> Object::memories() const
{
    std::vector<VkGpuMemory> mems;
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

void Device::init(bool enable_layers)
{
    // request all queues
    const std::vector<VkPhysicalGpuQueueProperties> queue_props = gpu_.queue_properties();
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

    VkLayerCreateInfo layer_info = {};
    layer_info.sType = VK_STRUCTURE_TYPE_LAYER_CREATE_INFO;

    std::vector<const char *> layers;
    std::vector<char> layer_buf;
    // request all layers
    if (enable_layers) {
        layers = gpu_.layers(layer_buf);
        layer_info.layerCount = layers.size();
        layer_info.ppActiveLayerNames = &layers[0];
    }

    const std::vector<const char *> exts = gpu_.extensions();

    VkDeviceCreateInfo dev_info = {};
    dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_info.pNext = (enable_layers) ? static_cast<void *>(&layer_info) : NULL;
    dev_info.queueRecordCount = queue_info.size();
    dev_info.pRequestedQueues = &queue_info[0];
    dev_info.extensionCount = exts.size();
    dev_info.ppEnabledExtensionNames = &exts[0];
    dev_info.flags = VK_DEVICE_CREATE_VALIDATION_BIT;

    init(dev_info);
}

void Device::init(const VkDeviceCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateDevice, gpu_.obj(), &info);

    init_queues();
    init_formats();
}

void Device::init_queues()
{
    VkResult err;
    size_t data_size;
    uint32_t queue_node_count;

    err = vkGetGpuInfo(gpu_.obj(), VK_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES,
                        &data_size, NULL);
    EXPECT(err == VK_SUCCESS);

    queue_node_count = data_size / sizeof(VkPhysicalGpuQueueProperties);
    EXPECT(queue_node_count >= 1);

    VkPhysicalGpuQueueProperties queue_props[queue_node_count];

    err = vkGetGpuInfo(gpu_.obj(), VK_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES,
                        &data_size, queue_props);
    EXPECT(err == VK_SUCCESS);

    for (int i = 0; i < queue_node_count; i++) {
        VkQueue queue;

        for (int j = 0; j < queue_props[i].queueCount; j++) {
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

    EXPECT(!queues_[GRAPHICS].empty() || !queues_[COMPUTE].empty());
}

void Device::init_formats()
{
    for (int f = VK_FMT_BEGIN_RANGE; f <= VK_FMT_END_RANGE; f++) {
        const VkFormat fmt = static_cast<VkFormat>(f);
        const VkFormatProperties props = format_properties(fmt);

        if (props.linearTilingFeatures) {
            const Format tmp = { fmt, VK_LINEAR_TILING, props.linearTilingFeatures };
            formats_.push_back(tmp);
        }

        if (props.optimalTilingFeatures) {
            const Format tmp = { fmt, VK_OPTIMAL_TILING, props.optimalTilingFeatures };
            formats_.push_back(tmp);
        }
    }

    EXPECT(!formats_.empty());
}

VkFormatProperties Device::format_properties(VkFormat format)
{
    const VkFormatInfoType type = VK_INFO_TYPE_FORMAT_PROPERTIES;
    VkFormatProperties data;
    size_t size = sizeof(data);
    if (!EXPECT(vkGetFormatInfo(obj(), format, type, &size, &data) == VK_SUCCESS && size == sizeof(data)))
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

void Device::begin_descriptor_pool_update(VkDescriptorUpdateMode mode)
{
    EXPECT(vkBeginDescriptorPoolUpdate(obj(), mode) == VK_SUCCESS);
}

void Device::end_descriptor_pool_update(CmdBuffer &cmd)
{
    EXPECT(vkEndDescriptorPoolUpdate(obj(), cmd.obj()) == VK_SUCCESS);
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

void Queue::add_mem_references(const std::vector<VkGpuMemory> &mem_refs)
{
    for (int i = 0; i < mem_refs.size(); i++) {
        EXPECT(vkQueueAddMemReference(obj(), mem_refs[i]) == VK_SUCCESS);
    }
}

void Queue::remove_mem_references(const std::vector<VkGpuMemory> &mem_refs)
{
    for (int i = 0; i < mem_refs.size(); i++) {
        EXPECT(vkQueueRemoveMemReference(obj(), mem_refs[i]) == VK_SUCCESS);
    }
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
        EXPECT(vkFreeMemory(obj()) == VK_SUCCESS);
}

void GpuMemory::init(const Device &dev, const VkMemoryAllocInfo &info)
{
    DERIVED_OBJECT_INIT(vkAllocMemory, dev.obj(), &info);
}

void GpuMemory::init(const Device &dev, size_t size, const void *data)
{
    DERIVED_OBJECT_INIT(vkPinSystemMemory, dev.obj(), data, size);
}

void GpuMemory::init(const Device &dev, const VkMemoryOpenInfo &info)
{
    DERIVED_OBJECT_INIT(vkOpenSharedMemory, dev.obj(), &info);
}

void GpuMemory::init(const Device &dev, const VkPeerMemoryOpenInfo &info)
{
    DERIVED_OBJECT_INIT(vkOpenPeerMemory, dev.obj(), &info);
}

void GpuMemory::set_priority(VkMemoryPriority priority)
{
    EXPECT(vkSetMemoryPriority(obj(), priority) == VK_SUCCESS);
}

const void *GpuMemory::map(VkFlags flags) const
{
    void *data;
    if (!EXPECT(vkMapMemory(obj(), flags, &data) == VK_SUCCESS))
        data = NULL;

    return data;
}

void *GpuMemory::map(VkFlags flags)
{
    void *data;
    if (!EXPECT(vkMapMemory(obj(), flags, &data) == VK_SUCCESS))
        data = NULL;

    return data;
}

void GpuMemory::unmap() const
{
    EXPECT(vkUnmapMemory(obj()) == VK_SUCCESS);
}

void Fence::init(const Device &dev, const VkFenceCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateFence, dev.obj(), &info);
    alloc_memory(dev);
}

void Semaphore::init(const Device &dev, const VkSemaphoreCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateSemaphore, dev.obj(), &info);
    alloc_memory(dev);
}

void Semaphore::init(const Device &dev, const VkSemaphoreOpenInfo &info)
{
    DERIVED_OBJECT_INIT(vkOpenSharedSemaphore, dev.obj(), &info);
}

void Event::init(const Device &dev, const VkEventCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateEvent, dev.obj(), &info);
    alloc_memory(dev);
}

void Event::set()
{
    EXPECT(vkSetEvent(obj()) == VK_SUCCESS);
}

void Event::reset()
{
    EXPECT(vkResetEvent(obj()) == VK_SUCCESS);
}

void QueryPool::init(const Device &dev, const VkQueryPoolCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateQueryPool, dev.obj(), &info);
    alloc_memory(dev);
}

VkResult QueryPool::results(uint32_t start, uint32_t count, size_t size, void *data)
{
    size_t tmp = size;
    VkResult err = vkGetQueryPoolResults(obj(), start, count, &tmp, data);
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
    alloc_memory(dev);
}

void Buffer::init_no_mem(const Device &dev, const VkBufferCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateBuffer, dev.obj(), &info);
    create_info_ = info;
}

void BufferView::init(const Device &dev, const VkBufferViewCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateBufferView, dev.obj(), &info);
    alloc_memory(dev);
}

void Image::init(const Device &dev, const VkImageCreateInfo &info)
{
    init_no_mem(dev, info);
    alloc_memory(dev);
}

void Image::init_no_mem(const Device &dev, const VkImageCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateImage, dev.obj(), &info);
    init_info(dev, info);
}

void Image::init(const Device &dev, const VkPeerImageOpenInfo &info, const VkImageCreateInfo &original_info)
{
    VkImage img;
    VkGpuMemory mem;
    EXPECT(vkOpenPeerImage(dev.obj(), &info, &img, &mem) == VK_SUCCESS);
    Object::init(img);

    init_info(dev, original_info);
    alloc_memory(std::vector<VkGpuMemory>(1, mem));
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

void Image::bind_memory(uint32_t alloc_idx, const VkImageMemoryBindInfo &info,
                        const GpuMemory &mem, VkGpuSize mem_offset)
{
    EXPECT(!alloc_idx && vkBindImageMemoryRange(obj(), 0, &info, mem.obj(), mem_offset) == VK_SUCCESS);
}

VkSubresourceLayout Image::subresource_layout(const VkImageSubresource &subres) const
{
    const VkSubresourceInfoType type = VK_INFO_TYPE_SUBRESOURCE_LAYOUT;
    VkSubresourceLayout data;
    size_t size = sizeof(data);
    if (!EXPECT(vkGetImageSubresourceInfo(obj(), &subres, type, &size, &data) == VK_SUCCESS && size == sizeof(data)))
        memset(&data, 0, sizeof(data));

    return data;
}

bool Image::transparent() const
{
    return (create_info_.tiling == VK_LINEAR_TILING &&
            create_info_.samples == 1 &&
            !(create_info_.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                    VK_IMAGE_USAGE_DEPTH_STENCIL_BIT)));
}

void ImageView::init(const Device &dev, const VkImageViewCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateImageView, dev.obj(), &info);
    alloc_memory(dev);
}

void ColorAttachmentView::init(const Device &dev, const VkColorAttachmentViewCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateColorAttachmentView, dev.obj(), &info);
    alloc_memory(dev);
}

void DepthStencilView::init(const Device &dev, const VkDepthStencilViewCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateDepthStencilView, dev.obj(), &info);
    alloc_memory(dev);
}

void Shader::init(const Device &dev, const VkShaderCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateShader, dev.obj(), &info);
}

VkResult Shader::init_try(const Device &dev, const VkShaderCreateInfo &info)
{
    VkShader sh;
    VkResult err = vkCreateShader(dev.obj(), &info, &sh);
    if (err == VK_SUCCESS)
        Object::init(sh);

    return err;
}

void Pipeline::init(const Device &dev, const VkGraphicsPipelineCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateGraphicsPipeline, dev.obj(), &info);
    alloc_memory(dev);
}

void Pipeline::init(
        const Device &dev,
        const VkGraphicsPipelineCreateInfo &info,
        const VkPipeline basePipeline)
{
    DERIVED_OBJECT_INIT(vkCreateGraphicsPipelineDerivative, dev.obj(), &info, basePipeline);
    alloc_memory(dev);
}

void Pipeline::init(const Device &dev, const VkComputePipelineCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateComputePipeline, dev.obj(), &info);
    alloc_memory(dev);
}

void Pipeline::init(const Device&dev, size_t size, const void *data)
{
    DERIVED_OBJECT_INIT(vkLoadPipeline, dev.obj(), size, data);
    alloc_memory(dev);
}

void Pipeline::init(
        const Device&dev,
        size_t size,
        const void *data,
        const VkPipeline basePipeline)
{
    DERIVED_OBJECT_INIT(vkLoadPipelineDerivative, dev.obj(), size, data, basePipeline);
    alloc_memory(dev);
}

size_t Pipeline::store(size_t size, void *data)
{
    if (!EXPECT(vkStorePipeline(obj(), &size, data) == VK_SUCCESS))
        size = 0;

    return size;
}

void Sampler::init(const Device &dev, const VkSamplerCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateSampler, dev.obj(), &info);
    alloc_memory(dev);
}

void DescriptorSetLayout::init(const Device &dev, const VkDescriptorSetLayoutCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateDescriptorSetLayout, dev.obj(), &info);
    alloc_memory(dev);
}

void DescriptorSetLayoutChain::init(const Device &dev, const std::vector<const DescriptorSetLayout *> &layouts)
{
    const std::vector<VkDescriptorSetLayout> layout_objs = make_objects<VkDescriptorSetLayout>(layouts);

    DERIVED_OBJECT_INIT(vkCreateDescriptorSetLayoutChain, dev.obj(), layout_objs.size(), &layout_objs[0]);
    alloc_memory(dev);
}

void DescriptorPool::init(const Device &dev, VkDescriptorPoolUsage usage,
                          uint32_t max_sets, const VkDescriptorPoolCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateDescriptorPool, dev.obj(), usage, max_sets, &info);
    alloc_memory(dev);
}

void DescriptorPool::reset()
{
    EXPECT(vkResetDescriptorPool(obj()) == VK_SUCCESS);
}

std::vector<DescriptorSet *> DescriptorPool::alloc_sets(VkDescriptorSetUsage usage, const std::vector<const DescriptorSetLayout *> &layouts)
{
    const std::vector<VkDescriptorSetLayout> layout_objs = make_objects<VkDescriptorSetLayout>(layouts);

    std::vector<VkDescriptorSet> set_objs;
    set_objs.resize(layout_objs.size());

    uint32_t set_count;
    VkResult err = vkAllocDescriptorSets(obj(), usage, layout_objs.size(), &layout_objs[0], &set_objs[0], &set_count);
    if (err == VK_SUCCESS)
        EXPECT(set_count == set_objs.size());
    set_objs.resize(set_count);

    std::vector<DescriptorSet *> sets;
    sets.reserve(set_count);
    for (std::vector<VkDescriptorSet>::const_iterator it = set_objs.begin(); it != set_objs.end(); it++) {
        // do descriptor sets need memories bound?
        sets.push_back(new DescriptorSet(*it));
    }

    return sets;
}

std::vector<DescriptorSet *> DescriptorPool::alloc_sets(VkDescriptorSetUsage usage, const DescriptorSetLayout &layout, uint32_t count)
{
    return alloc_sets(usage, std::vector<const DescriptorSetLayout *>(count, &layout));
}

DescriptorSet *DescriptorPool::alloc_sets(VkDescriptorSetUsage usage, const DescriptorSetLayout &layout)
{
    std::vector<DescriptorSet *> set = alloc_sets(usage, layout, 1);
    return (set.empty()) ? NULL : set[0];
}

void DescriptorPool::clear_sets(const std::vector<DescriptorSet *> &sets)
{
    const std::vector<VkDescriptorSet> set_objs = make_objects<VkDescriptorSet>(sets);
    vkClearDescriptorSets(obj(), set_objs.size(), &set_objs[0]);
}

void DescriptorSet::update(const std::vector<const void *> &update_array)
{
    vkUpdateDescriptors(obj(), update_array.size(), const_cast<const void **>(&update_array[0]));
}

void DynamicVpStateObject::init(const Device &dev, const VkDynamicVpStateCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateDynamicViewportState, dev.obj(), &info);
    alloc_memory(dev);
}

void DynamicRsStateObject::init(const Device &dev, const VkDynamicRsStateCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateDynamicRasterState, dev.obj(), &info);
    alloc_memory(dev);
}

void DynamicCbStateObject::init(const Device &dev, const VkDynamicCbStateCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateDynamicColorBlendState, dev.obj(), &info);
    alloc_memory(dev);
}

void DynamicDsStateObject::init(const Device &dev, const VkDynamicDsStateCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateDynamicDepthStencilState, dev.obj(), &info);
    alloc_memory(dev);
}

void CmdBuffer::init(const Device &dev, const VkCmdBufferCreateInfo &info)
{
    DERIVED_OBJECT_INIT(vkCreateCommandBuffer, dev.obj(), &info);
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

    info.flags = VK_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT |
          VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT;
    info.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;
    info.pNext = &graphics_cmd_buf_info;

    begin(&info);
}

void CmdBuffer::begin()
{
    VkCmdBufferBeginInfo info = {};
    info.flags = VK_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT |
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
