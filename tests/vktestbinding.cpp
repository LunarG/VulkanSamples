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

#define NON_DISPATCHABLE_HANDLE_INIT(create_func, dev, ...)                         \
    do {                                                                            \
        handle_type handle;                                                         \
        if (EXPECT(create_func(dev.handle(), __VA_ARGS__, &handle) == VK_SUCCESS))  \
            NonDispHandle::init(dev.handle(), handle);                              \
    } while (0)

#define NON_DISPATCHABLE_HANDLE_DTOR(cls, destroy_func, ...)                        \
    cls::~cls()                                                                     \
    {                                                                               \
        if (initialized())                                                          \
            EXPECT(destroy_func(device(), __VA_ARGS__, handle()) == VK_SUCCESS);    \
    }

#define DERIVED_OBJECT_TYPE_INIT(create_func, dev, vk_object_type, ...)         \
    do {                                                                        \
        obj_type obj;                                                           \
        dev_ = &dev;                                                        \
        if (EXPECT(create_func(dev.handle(), __VA_ARGS__, &obj) == VK_SUCCESS)) \
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
std::vector<T> make_handles(const std::vector<S> &v)
{
    std::vector<T> handles;
    handles.reserve(v.size());
    for (typename std::vector<S>::const_iterator it = v.begin(); it != v.end(); it++)
        handles.push_back((*it)->handle());
    return handles;
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
std::vector<T> get_memory_reqs(VkDevice device, VkObjectType obj_type, VkObject obj, size_t min_elems)
{
    std::vector<T> info;

    info.resize((min_elems > 0)?min_elems:1);
    if (!EXPECT(vkGetObjectMemoryRequirements(device, obj_type, obj, &info[0]) == VK_SUCCESS))
        info.clear();

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

VkPhysicalDeviceProperties PhysicalDevice::properties() const
{
    VkPhysicalDeviceProperties info;

    EXPECT(vkGetPhysicalDeviceProperties(handle(), &info) == VK_SUCCESS);

    return info;
}

VkPhysicalDevicePerformance PhysicalDevice::performance() const
{
    VkPhysicalDevicePerformance info;

    EXPECT(vkGetPhysicalDevicePerformance(handle(), &info) == VK_SUCCESS);

    return info;
}

std::vector<VkPhysicalDeviceQueueProperties> PhysicalDevice::queue_properties() const
{
    std::vector<VkPhysicalDeviceQueueProperties> info;
    uint32_t count;

    if (EXPECT(vkGetPhysicalDeviceQueueCount(handle(), &count) == VK_SUCCESS)) {
        info.resize(count);
        if (!EXPECT(vkGetPhysicalDeviceQueueProperties(handle(), count, &info[0]) == VK_SUCCESS))
            info.clear();
    }

    return info;
}

VkPhysicalDeviceMemoryProperties PhysicalDevice::memory_properties() const
{
    VkPhysicalDeviceMemoryProperties info;

    EXPECT(vkGetPhysicalDeviceMemoryProperties(handle(), &info) == VK_SUCCESS);


    return info;
}

/*
 * Return list of Global layers available
 */
std::vector<VkLayerProperties> GetGlobalLayers()
{
    VkResult err;
    std::vector<VkLayerProperties> layers;
    uint32_t layer_count;

    do {
        layer_count = 0;
        err = vkGetGlobalLayerProperties(&layer_count, NULL);

        if (err == VK_SUCCESS) {
            layers.reserve(layer_count);
            err = vkGetGlobalLayerProperties(&layer_count, &layers[0]);
        }
    } while (err == VK_INCOMPLETE);

    assert(err == VK_SUCCESS);

    return layers;
}

/*
 * Return list of Global extensions provided by the ICD / Loader
 */
std::vector<VkExtensionProperties> GetGlobalExtensions()
{
    return GetGlobalExtensions(NULL);
}

/*
 * Return list of Global extensions provided by the specified layer
 * If pLayerName is NULL, will return extensions implemented by the loader / ICDs
 */
std::vector<VkExtensionProperties> GetGlobalExtensions(const char *pLayerName)
{
    std::vector<VkExtensionProperties> exts;
    uint32_t ext_count;
    VkResult err;

    do {
        ext_count = 0;
        err = vkGetGlobalExtensionProperties(pLayerName, &ext_count, NULL);

        if (err == VK_SUCCESS) {
            exts.resize(ext_count);
            err = vkGetGlobalExtensionProperties(pLayerName, &ext_count, &exts[0]);
        }
    } while (err == VK_INCOMPLETE);

    assert(err == VK_SUCCESS);

    return exts;
}

/*
 * Return list of PhysicalDevice extensions provided by the ICD / Loader
 */
std::vector<VkExtensionProperties> PhysicalDevice::extensions() const
{
    return extensions(NULL);
}

/*
 * Return list of PhysicalDevice extensions provided by the specified layer
 * If pLayerName is NULL, will return extensions for ICD / loader.
 */
std::vector<VkExtensionProperties> PhysicalDevice::extensions(const char *pLayerName) const
{
    std::vector<VkExtensionProperties> exts;
    VkResult err;

    do {
        uint32_t extCount = 0;
        err = vkGetPhysicalDeviceExtensionProperties(handle(), pLayerName, &extCount, NULL);

        if (err == VK_SUCCESS) {
            exts.reserve(extCount);
            err = vkGetPhysicalDeviceExtensionProperties(handle(), pLayerName, &extCount, &exts[0]);
        }
    } while (err == VK_INCOMPLETE);

    assert(err == VK_SUCCESS);

    return exts;
}

VkResult PhysicalDevice::set_memory_type(const uint32_t type_bits, VkMemoryAllocInfo *info, const VkFlags properties) const
{
     uint32_t type_mask = type_bits;
     // Search memtypes to find first index with those properties
     for (uint32_t i = 0; i < 32; i++) {
         if ((type_mask & 1) == 1) {
             // Type is available, does it match user properties?
             if ((memory_properties_.memoryTypes[i].propertyFlags & properties) == properties) {
                 info->memoryTypeIndex = i;
                 return VK_SUCCESS;
             }
         }
         type_mask >>= 1;
     }
     // No memory types matched, return failure
     return VK_UNSUPPORTED;
}

/*
 * Return list of PhysicalDevice layers
 */
std::vector<VkLayerProperties> PhysicalDevice::layers() const
{
    std::vector<VkLayerProperties> layer_props;
    VkResult err;

    do {
        uint32_t layer_count = 0;
        err = vkGetPhysicalDeviceLayerProperties(handle(), &layer_count, NULL);

        if (err == VK_SUCCESS) {
            layer_props.reserve(layer_count);
            err = vkGetPhysicalDeviceLayerProperties(handle(), &layer_count, &layer_props[0]);
        }
    } while (err == VK_INCOMPLETE);

    assert(err == VK_SUCCESS);

    return layer_props;
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
        get_memory_reqs<VkMemoryRequirements>(dev_->handle(), type(), obj(), 0);
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
        EXPECT(vkDestroyObject(dev_->handle(), type(), obj()) == VK_SUCCESS);

    if (internal_mems_) {
        delete[] internal_mems_;
        internal_mems_ = NULL;
        primary_mem_ = NULL;
    }

    mem_alloc_count_ = 0;
}

void Object::bind_memory(const DeviceMemory &mem, VkDeviceSize mem_offset)
{
    bound = true;
    EXPECT(vkBindObjectMemory(dev_->handle(), type(), obj(), mem.handle(), mem_offset) == VK_SUCCESS);
}

void Object::alloc_memory()
{
    if (!EXPECT(!internal_mems_) || !mem_alloc_count_)
        return;

    internal_mems_ = new DeviceMemory[mem_alloc_count_];

    const std::vector<VkMemoryRequirements> mem_reqs = memory_requirements();
    VkMemoryAllocInfo info;

    for (int i = 0; i < mem_reqs.size(); i++) {
        info = DeviceMemory::alloc_info(mem_reqs[i].size, 0);
        dev_->phy().set_memory_type(mem_reqs[i].memoryTypeBits, &info, 0);
        primary_mem_ = &internal_mems_[i];
        internal_mems_[i].init(*dev_, info);
        bind_memory(internal_mems_[i], 0);
    }
}

void Object::alloc_memory(VkMemoryPropertyFlags &reqs)
{
    if (!EXPECT(!internal_mems_) || !mem_alloc_count_)
        return;

    internal_mems_ = new DeviceMemory[mem_alloc_count_];

    std::vector<VkMemoryRequirements> mem_reqs = memory_requirements();
    VkMemoryAllocInfo info;

    for (int i = 0; i < mem_reqs.size(); i++) {
        info = DeviceMemory::alloc_info(mem_reqs[i].size, 0);
        dev_->phy().set_memory_type(mem_reqs[i].memoryTypeBits, &info, reqs);
        primary_mem_ = &internal_mems_[i];
        internal_mems_[i].init(*dev_, info);
        bind_memory(internal_mems_[i], 0);
    }
}

std::vector<VkDeviceMemory> Object::memories() const
{
    std::vector<VkDeviceMemory> mems;
    if (internal_mems_) {
        mems.reserve(mem_alloc_count_);
        for (uint32_t i = 0; i < mem_alloc_count_; i++)
            mems.push_back(internal_mems_[i].handle());
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

    EXPECT(vkDestroyDevice(handle()) == VK_SUCCESS);
}

void Device::init(std::vector<const char *> &layers, std::vector<const char *> &extensions)
{
    // request all queues
    const std::vector<VkPhysicalDeviceQueueProperties> queue_props = phy_.queue_properties();
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
    dev_info.layerCount = layers.size();
    dev_info.ppEnabledLayerNames = &layers[0];
    dev_info.extensionCount = extensions.size();
    dev_info.ppEnabledExtensionNames = &extensions[0];
    dev_info.flags = 0;

    init(dev_info);
}

void Device::init(const VkDeviceCreateInfo &info)
{
    VkDevice dev;

    if (EXPECT(vkCreateDevice(phy_.handle(), &info, &dev) == VK_SUCCESS))
        Handle::init(dev);

    init_queues();
    init_formats();
}

void Device::init_queues()
{
    VkResult err;
    uint32_t queue_node_count;

    err = vkGetPhysicalDeviceQueueCount(phy_.handle(), &queue_node_count);
    EXPECT(err == VK_SUCCESS);
    EXPECT(queue_node_count >= 1);

    VkPhysicalDeviceQueueProperties* queue_props = new VkPhysicalDeviceQueueProperties[queue_node_count];

    err = vkGetPhysicalDeviceQueueProperties(phy_.handle(), queue_node_count, queue_props);
    EXPECT(err == VK_SUCCESS);

    for (uint32_t i = 0; i < queue_node_count; i++) {
        VkQueue queue;

        for (uint32_t j = 0; j < queue_props[i].queueCount; j++) {
            // TODO: Need to add support for separate MEMMGR and work queues, including synchronization
            err = vkGetDeviceQueue(handle(), i, j, &queue);
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
    if (!EXPECT(vkGetPhysicalDeviceFormatInfo(phy().handle(), format, &data) == VK_SUCCESS))
        memset(&data, 0, sizeof(data));

    return data;
}

void Device::wait()
{
    EXPECT(vkDeviceWaitIdle(handle()) == VK_SUCCESS);
}

VkResult Device::wait(const std::vector<const Fence *> &fences, bool wait_all, uint64_t timeout)
{
    const std::vector<VkFence> fence_handles = make_handles<VkFence>(fences);
    VkResult err = vkWaitForFences(handle(), fence_handles.size(), &fence_handles[0], wait_all, timeout);
    EXPECT(err == VK_SUCCESS || err == VK_TIMEOUT);

    return err;
}

VkResult Device::update_descriptor_sets(const std::vector<VkWriteDescriptorSet> &writes, const std::vector<VkCopyDescriptorSet> &copies)
{
    return vkUpdateDescriptorSets(handle(), writes.size(), &writes[0], copies.size(), &copies[0]);
}

void Queue::submit(const std::vector<const CmdBuffer *> &cmds, Fence &fence)
{
    const std::vector<VkCmdBuffer> cmd_objs = make_objects<VkCmdBuffer>(cmds);
    EXPECT(vkQueueSubmit(handle(), cmd_objs.size(), &cmd_objs[0], fence.handle()) == VK_SUCCESS);
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
    EXPECT(vkQueueWaitIdle(handle()) == VK_SUCCESS);
}

void Queue::signal_semaphore(Semaphore &sem)
{
    EXPECT(vkQueueSignalSemaphore(handle(), sem.handle()) == VK_SUCCESS);
}

void Queue::wait_semaphore(Semaphore &sem)
{
    EXPECT(vkQueueWaitSemaphore(handle(), sem.handle()) == VK_SUCCESS);
}

DeviceMemory::~DeviceMemory()
{
    if (initialized())
        EXPECT(vkFreeMemory(device(), handle()) == VK_SUCCESS);
}

void DeviceMemory::init(const Device &dev, const VkMemoryAllocInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkAllocMemory, dev, &info);
}

const void *DeviceMemory::map(VkFlags flags) const
{
    void *data;
    if (!EXPECT(vkMapMemory(device(), handle(), 0 ,0, flags, &data) == VK_SUCCESS))
        data = NULL;

    return data;
}

void *DeviceMemory::map(VkFlags flags)
{
    void *data;
    if (!EXPECT(vkMapMemory(device(), handle(), 0, 0, flags, &data) == VK_SUCCESS))
        data = NULL;

    return data;
}

void DeviceMemory::unmap() const
{
    EXPECT(vkUnmapMemory(device(), handle()) == VK_SUCCESS);
}

NON_DISPATCHABLE_HANDLE_DTOR(Fence, vkDestroyObject, VK_OBJECT_TYPE_FENCE)

void Fence::init(const Device &dev, const VkFenceCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateFence, dev, &info);
}

NON_DISPATCHABLE_HANDLE_DTOR(Semaphore, vkDestroyObject, VK_OBJECT_TYPE_SEMAPHORE)

void Semaphore::init(const Device &dev, const VkSemaphoreCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateSemaphore, dev, &info);
}

NON_DISPATCHABLE_HANDLE_DTOR(Event, vkDestroyObject, VK_OBJECT_TYPE_EVENT)

void Event::init(const Device &dev, const VkEventCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateEvent, dev, &info);
}

void Event::set()
{
    EXPECT(vkSetEvent(device(), handle()) == VK_SUCCESS);
}

void Event::reset()
{
    EXPECT(vkResetEvent(device(), handle()) == VK_SUCCESS);
}

NON_DISPATCHABLE_HANDLE_DTOR(QueryPool, vkDestroyObject, VK_OBJECT_TYPE_QUERY_POOL)

void QueryPool::init(const Device &dev, const VkQueryPoolCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateQueryPool, dev, &info);
}

VkResult QueryPool::results(uint32_t start, uint32_t count, size_t size, void *data)
{
    size_t tmp = size;
    VkResult err = vkGetQueryPoolResults(device(), handle(), start, count, &tmp, data, 0);
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
                         const DeviceMemory &mem, VkDeviceSize mem_offset)
{
    VkQueue queue = dev_->graphics_queues()[0]->handle();
    VkSparseMemoryBindInfo bindInfo;
    memset(&bindInfo, 0, sizeof(VkSparseMemoryBindInfo));
    bindInfo.offset    = offset;
    bindInfo.memOffset = mem_offset;
    bindInfo.mem       = mem.handle();
    EXPECT(vkQueueBindSparseBufferMemory(queue, obj(), 1, &bindInfo) == VK_SUCCESS);
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

void Image::bind_memory(const Device &dev, const VkSparseImageMemoryBindInfo &info,
                        const DeviceMemory &mem, VkDeviceSize mem_offset)
{
    VkQueue queue = dev.graphics_queues()[0]->handle();
    EXPECT(vkQueueBindSparseImageMemory(queue, obj(), 1, &info) == VK_SUCCESS);
}

VkSubresourceLayout Image::subresource_layout(const VkImageSubresource &subres) const
{
    VkSubresourceLayout data;
    size_t size = sizeof(data);
    if (!EXPECT(vkGetImageSubresourceLayout(dev_->handle(), obj(), &subres, &data) == VK_SUCCESS && size == sizeof(data)))
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

void AttachmentView::init(const Device &dev, const VkAttachmentViewCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateAttachmentView, dev, VK_OBJECT_TYPE_ATTACHMENT_VIEW, &info);
    alloc_memory();
}

void ShaderModule::init(const Device &dev, const VkShaderModuleCreateInfo &info)
{
    DERIVED_OBJECT_TYPE_INIT(vkCreateShaderModule, dev, VK_OBJECT_TYPE_SHADER_MODULE, &info);
}

VkResult ShaderModule::init_try(const Device &dev, const VkShaderModuleCreateInfo &info)
{
    /*
     * Note: Cannot use DERIVED_OBJECT_TYPE_INIT as we need the
     * return code.
     */
    VkShaderModule sh;
    dev_ = &dev;
    VkResult err = vkCreateShaderModule(dev.handle(), &info, &sh);
    if (err == VK_SUCCESS)
        Object::init(sh, VK_OBJECT_TYPE_SHADER_MODULE);

    return err;
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
    VkResult err = vkCreateShader(dev.handle(), &info, &sh);
    if (err == VK_SUCCESS)
        Object::init(sh, VK_OBJECT_TYPE_SHADER);

    return err;
}

void Pipeline::init(const Device &dev, const VkGraphicsPipelineCreateInfo &info)
{
    VkPipelineCache cache;
    VkPipelineCacheCreateInfo ci;
    memset((void *) &ci, 0, sizeof(VkPipelineCacheCreateInfo));
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VkResult err = vkCreatePipelineCache(dev.handle(), &ci, &cache);
    if (err == VK_SUCCESS) {
        DERIVED_OBJECT_TYPE_INIT(vkCreateGraphicsPipelines, dev, VK_OBJECT_TYPE_PIPELINE, cache, 1, &info);
        alloc_memory();
        vkDestroyPipelineCache(dev.handle(), cache);
    }
}

VkResult Pipeline::init_try(const Device &dev, const VkGraphicsPipelineCreateInfo &info)
{
    VkPipeline pipe;
    VkPipelineCache cache;
    VkPipelineCacheCreateInfo ci;
    dev_ = &dev;
    memset((void *) &ci, 0, sizeof(VkPipelineCacheCreateInfo));
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VkResult err = vkCreatePipelineCache(dev.handle(), &ci, &cache);
    EXPECT(err == VK_SUCCESS);
    if (err == VK_SUCCESS) {
        err = vkCreateGraphicsPipelines(dev.handle(), cache, 1, &info, &pipe);
        if (err == VK_SUCCESS) {
            Object::init(pipe, VK_OBJECT_TYPE_PIPELINE);
            alloc_memory();
            vkDestroyPipelineCache(dev.handle(), cache);
        }
    }

    return err;
}


void Pipeline::init(const Device &dev, const VkComputePipelineCreateInfo &info)
{
    VkPipelineCache cache;
    VkPipelineCacheCreateInfo ci;
    memset((void *) &ci, 0, sizeof(VkPipelineCacheCreateInfo));
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VkResult err = vkCreatePipelineCache(dev.handle(), &ci, &cache);
    if (err == VK_SUCCESS) {
        DERIVED_OBJECT_TYPE_INIT(vkCreateComputePipelines, dev, VK_OBJECT_TYPE_PIPELINE, cache, 1, &info);
        alloc_memory();
        vkDestroyPipelineCache(dev.handle(), cache);
    }
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
    EXPECT(vkResetDescriptorPool(dev_->handle(), obj()) == VK_SUCCESS);
}

std::vector<DescriptorSet *> DescriptorPool::alloc_sets(const Device &dev, VkDescriptorSetUsage usage, const std::vector<const DescriptorSetLayout *> &layouts)
{
    const std::vector<VkDescriptorSetLayout> layout_objs = make_objects<VkDescriptorSetLayout>(layouts);

    std::vector<VkDescriptorSet> set_objs;
    set_objs.resize(layout_objs.size());

    uint32_t set_count;
    VkResult err = vkAllocDescriptorSets(dev_->handle(), obj(), usage, layout_objs.size(), &layout_objs[0], &set_objs[0], &set_count);
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
