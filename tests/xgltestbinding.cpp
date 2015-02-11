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

#include <iostream>
#include <string.h> // memset(), memcmp()
#include "xgltestbinding.h"

namespace {

#define DERIVED_OBJECT_INIT(create_func, ...)                       \
    do {                                                            \
        obj_type obj;                                               \
        if (EXPECT(create_func(__VA_ARGS__, &obj) == XGL_SUCCESS))  \
            base_type::init(obj);                                   \
    } while (0)

#define STRINGIFY(x) #x
#define EXPECT(expr) ((expr) ? true : expect_failure(STRINGIFY(expr), __FILE__, __LINE__, __FUNCTION__))

xgl_testing::ErrorCallback error_callback;

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
std::vector<T> get_info(XGL_PHYSICAL_GPU gpu, XGL_PHYSICAL_GPU_INFO_TYPE type, size_t min_elems)
{
    std::vector<T> info;
    size_t size;
    if (EXPECT(xglGetGpuInfo(gpu, type, &size, NULL) == XGL_SUCCESS && size % sizeof(T) == 0)) {
        info.resize(size / sizeof(T));
        if (!EXPECT(xglGetGpuInfo(gpu, type, &size, &info[0]) == XGL_SUCCESS && size == info.size() * sizeof(T)))
            info.clear();
    }

    if (info.size() < min_elems)
        info.resize(min_elems);

    return info;
}

template<typename T>
std::vector<T> get_info(XGL_BASE_OBJECT obj, XGL_OBJECT_INFO_TYPE type, size_t min_elems)
{
    std::vector<T> info;
    size_t size;
    if (EXPECT(xglGetObjectInfo(obj, type, &size, NULL) == XGL_SUCCESS && size % sizeof(T) == 0)) {
        info.resize(size / sizeof(T));
        if (!EXPECT(xglGetObjectInfo(obj, type, &size, &info[0]) == XGL_SUCCESS && size == info.size() * sizeof(T)))
            info.clear();
    }

    if (info.size() < min_elems)
        info.resize(min_elems);

    return info;
}

} // namespace

namespace xgl_testing {

void set_error_callback(ErrorCallback callback)
{
    error_callback = callback;
}

XGL_PHYSICAL_GPU_PROPERTIES PhysicalGpu::properties() const
{
    return get_info<XGL_PHYSICAL_GPU_PROPERTIES>(gpu_, XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES, 1)[0];
}

XGL_PHYSICAL_GPU_PERFORMANCE PhysicalGpu::performance() const
{
    return get_info<XGL_PHYSICAL_GPU_PERFORMANCE>(gpu_, XGL_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE, 1)[0];
}

std::vector<XGL_PHYSICAL_GPU_QUEUE_PROPERTIES> PhysicalGpu::queue_properties() const
{
    return get_info<XGL_PHYSICAL_GPU_QUEUE_PROPERTIES>(gpu_, XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES, 0);
}

XGL_PHYSICAL_GPU_MEMORY_PROPERTIES PhysicalGpu::memory_properties() const
{
    return get_info<XGL_PHYSICAL_GPU_MEMORY_PROPERTIES>(gpu_, XGL_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES, 1)[0];
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
    if (!EXPECT(xglEnumerateLayers(gpu_, max_layer_count, max_string_size, &count, out, NULL) == XGL_SUCCESS))
        count = 0;
    layers.resize(count);

    return layers;
}

std::vector<const char *> PhysicalGpu::extensions() const
{
    static const char *known_exts[] = {
        "XGL_WSI_X11",
    };

    std::vector<const char *> exts;
    for (int i = 0; i < sizeof(known_exts) / sizeof(known_exts[0]); i++) {
        XGL_RESULT err = xglGetExtensionSupport(gpu_, known_exts[i]);
        if (err == XGL_SUCCESS)
            exts.push_back(known_exts[i]);
    }

    return exts;
}

XGL_GPU_COMPATIBILITY_INFO PhysicalGpu::compatibility(const PhysicalGpu &other) const
{
    XGL_GPU_COMPATIBILITY_INFO data;
    if (!EXPECT(xglGetMultiGpuCompatibility(gpu_, other.gpu_, &data) == XGL_SUCCESS))
        memset(&data, 0, sizeof(data));

    return data;
}

void BaseObject::init(XGL_BASE_OBJECT obj, bool own)
{
    EXPECT(!initialized());
    reinit(obj, own);
}

void BaseObject::reinit(XGL_BASE_OBJECT obj, bool own)
{
    obj_ = obj;
    own_obj_ = own;
}

uint32_t BaseObject::memory_allocation_count() const
{
    return get_info<uint32_t>(obj_, XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT, 1)[0];
}

std::vector<XGL_MEMORY_REQUIREMENTS> BaseObject::memory_requirements() const
{
    XGL_RESULT err;
    uint32_t num_allocations = 0;
    size_t num_alloc_size = sizeof(num_allocations);
    err = xglGetObjectInfo(obj_, XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                           &num_alloc_size, &num_allocations);
    EXPECT(err == XGL_SUCCESS && num_alloc_size == sizeof(num_allocations));
    std::vector<XGL_MEMORY_REQUIREMENTS> info =
        get_info<XGL_MEMORY_REQUIREMENTS>(obj_, XGL_INFO_TYPE_MEMORY_REQUIREMENTS, 0);
    EXPECT(info.size() == num_allocations);
    if (info.size() == 1 && !info[0].size)
        info.clear();

    return info;
}

void Object::init(XGL_OBJECT obj, bool own)
{
    BaseObject::init(obj, own);
    mem_alloc_count_ = memory_allocation_count();
}

void Object::reinit(XGL_OBJECT obj, bool own)
{
    cleanup();
    BaseObject::reinit(obj, own);
    mem_alloc_count_ = memory_allocation_count();
}

void Object::cleanup()
{
    if (!initialized())
        return;

    unbind_memory();

    if (internal_mems_) {
        delete[] internal_mems_;
        internal_mems_ = NULL;
        primary_mem_ = NULL;
    }

    mem_alloc_count_ = 0;

    if (own())
        EXPECT(xglDestroyObject(obj()) == XGL_SUCCESS);
}

void Object::bind_memory(uint32_t alloc_idx, const GpuMemory &mem, XGL_GPU_SIZE mem_offset)
{
    EXPECT(xglBindObjectMemory(obj(), alloc_idx, mem.obj(), mem_offset) == XGL_SUCCESS);
}

void Object::bind_memory(uint32_t alloc_idx, XGL_GPU_SIZE offset, XGL_GPU_SIZE size,
                         const GpuMemory &mem, XGL_GPU_SIZE mem_offset)
{
    EXPECT(!alloc_idx && xglBindObjectMemoryRange(obj(), 0, offset, size, mem.obj(), mem_offset) == XGL_SUCCESS);
}

void Object::unbind_memory(uint32_t alloc_idx)
{
    EXPECT(xglBindObjectMemory(obj(), alloc_idx, XGL_NULL_HANDLE, 0) == XGL_SUCCESS);
}

void Object::unbind_memory()
{
    for (uint32_t i = 0; i < mem_alloc_count_; i++)
        unbind_memory(i);
}

void Object::alloc_memory(const Device &dev, bool for_buf, bool for_img)
{
    if (!EXPECT(!internal_mems_) || !mem_alloc_count_)
        return;

    internal_mems_ = new GpuMemory[mem_alloc_count_];

    const std::vector<XGL_MEMORY_REQUIREMENTS> mem_reqs = memory_requirements();
    std::vector<XGL_IMAGE_MEMORY_REQUIREMENTS> img_reqs;
    std::vector<XGL_BUFFER_MEMORY_REQUIREMENTS> buf_reqs;
    XGL_MEMORY_ALLOC_IMAGE_INFO img_info;
    XGL_MEMORY_ALLOC_BUFFER_INFO buf_info;
    XGL_MEMORY_ALLOC_INFO info, *next_info = NULL;

    if (for_img) {
        img_reqs = get_info<XGL_IMAGE_MEMORY_REQUIREMENTS>(obj(),
                        XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS, 0);
        EXPECT(img_reqs.size() == 1);
        next_info = (XGL_MEMORY_ALLOC_INFO *) &img_info;
        img_info.pNext = NULL;
        img_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO;
        img_info.usage = img_reqs[0].usage;
        img_info.formatClass = img_reqs[0].formatClass;
        img_info.samples = img_reqs[0].samples;
    }


    if (for_buf) {
        buf_reqs = get_info<XGL_BUFFER_MEMORY_REQUIREMENTS>(obj(),
                        XGL_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS, 0);
        if (for_img)
            img_info.pNext = &buf_info;
        else
            next_info = (XGL_MEMORY_ALLOC_INFO *) &buf_info;
        buf_info.pNext = NULL;
        buf_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO;
        buf_info.usage = buf_reqs[0].usage;
    }


    for (int i = 0; i < mem_reqs.size(); i++) {
        info = GpuMemory::alloc_info(mem_reqs[i], next_info);

        switch (info.memType) {
        case XGL_MEMORY_TYPE_BUFFER:
            EXPECT(for_buf);
            info.memProps |= XGL_MEMORY_PROPERTY_CPU_VISIBLE_BIT;
            primary_mem_ = &internal_mems_[i];
            break;
        case XGL_MEMORY_TYPE_IMAGE:
            EXPECT(for_img);
            primary_mem_ = &internal_mems_[i];
            break;
        default:
            break;
        }

        internal_mems_[i].init(dev, info);
        bind_memory(i, internal_mems_[i], 0);
    }
}

void Object::alloc_memory(const std::vector<XGL_GPU_MEMORY> &mems)
{
    if (!EXPECT(!internal_mems_) || !mem_alloc_count_)
        return;

    internal_mems_ = new GpuMemory[mem_alloc_count_];

    const std::vector<XGL_MEMORY_REQUIREMENTS> mem_reqs = memory_requirements();
    if (!EXPECT(mem_reqs.size() == mems.size()))
        return;

    for (int i = 0; i < mem_reqs.size(); i++) {
        primary_mem_ = &internal_mems_[i];

        internal_mems_[i].init(mems[i]);
        bind_memory(i, internal_mems_[i], 0);
    }
}

std::vector<XGL_GPU_MEMORY> Object::memories() const
{
    std::vector<XGL_GPU_MEMORY> mems;
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

    EXPECT(xglDestroyDevice(obj()) == XGL_SUCCESS);
}

void Device::init(bool enable_layers)
{
    // request all queues
    const std::vector<XGL_PHYSICAL_GPU_QUEUE_PROPERTIES> queue_props = gpu_.queue_properties();
    std::vector<XGL_DEVICE_QUEUE_CREATE_INFO> queue_info;
    queue_info.reserve(queue_props.size());
    for (int i = 0; i < queue_props.size(); i++) {
        XGL_DEVICE_QUEUE_CREATE_INFO qi = {};
        qi.queueNodeIndex = i;
        qi.queueCount = queue_props[i].queueCount;
        queue_info.push_back(qi);
    }

    XGL_LAYER_CREATE_INFO layer_info = {};
    layer_info.sType = XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO;

    std::vector<const char *> layers;
    std::vector<char> layer_buf;
    // request all layers
    if (enable_layers) {
        layers = gpu_.layers(layer_buf);
        layer_info.layerCount = layers.size();
        layer_info.ppActiveLayerNames = &layers[0];
    }

    const std::vector<const char *> exts = gpu_.extensions();

    XGL_DEVICE_CREATE_INFO dev_info = {};
    dev_info.sType = XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_info.pNext = (enable_layers) ? static_cast<void *>(&layer_info) : NULL;
    dev_info.queueRecordCount = queue_info.size();
    dev_info.pRequestedQueues = &queue_info[0];
    dev_info.extensionCount = exts.size();
    dev_info.ppEnabledExtensionNames = &exts[0];
    dev_info.maxValidationLevel = XGL_VALIDATION_LEVEL_END_RANGE;
    dev_info.flags = XGL_DEVICE_CREATE_VALIDATION_BIT;

    init(dev_info);
}

void Device::init(const XGL_DEVICE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateDevice, gpu_.obj(), &info);

    init_queues();
    init_formats();
}

void Device::init_queues()
{
    const struct {
        QueueIndex index;
        XGL_QUEUE_TYPE type;
    } queue_mapping[] = {
        { GRAPHICS, XGL_QUEUE_TYPE_GRAPHICS },
        { COMPUTE, XGL_QUEUE_TYPE_COMPUTE },
        { DMA, XGL_QUEUE_TYPE_DMA },
    };

    for (int i = 0; i < QUEUE_COUNT; i++) {
        uint32_t idx = 0;

        while (true) {
            XGL_QUEUE queue;
            XGL_RESULT err = xglGetDeviceQueue(obj(), queue_mapping[i].type, idx++, &queue);
            if (err != XGL_SUCCESS)
                break;
            queues_[queue_mapping[i].index].push_back(new Queue(queue));
        }
    }

    EXPECT(!queues_[GRAPHICS].empty() || !queues_[COMPUTE].empty());
}

void Device::init_formats()
{
    for (int f = XGL_FMT_BEGIN_RANGE; f <= XGL_FMT_END_RANGE; f++) {
        const XGL_FORMAT fmt = static_cast<XGL_FORMAT>(f);
        const XGL_FORMAT_PROPERTIES props = format_properties(fmt);

        if (props.linearTilingFeatures) {
            const Format tmp = { fmt, XGL_LINEAR_TILING, props.linearTilingFeatures };
            formats_.push_back(tmp);
        }

        if (props.optimalTilingFeatures) {
            const Format tmp = { fmt, XGL_OPTIMAL_TILING, props.optimalTilingFeatures };
            formats_.push_back(tmp);
        }
    }

    EXPECT(!formats_.empty());
}

XGL_FORMAT_PROPERTIES Device::format_properties(XGL_FORMAT format)
{
    const XGL_FORMAT_INFO_TYPE type = XGL_INFO_TYPE_FORMAT_PROPERTIES;
    XGL_FORMAT_PROPERTIES data;
    size_t size = sizeof(data);
    if (!EXPECT(xglGetFormatInfo(obj(), format, type, &size, &data) == XGL_SUCCESS && size == sizeof(data)))
        memset(&data, 0, sizeof(data));

    return data;
}

void Device::wait()
{
    EXPECT(xglDeviceWaitIdle(obj()) == XGL_SUCCESS);
}

XGL_RESULT Device::wait(const std::vector<const Fence *> &fences, bool wait_all, uint64_t timeout)
{
    const std::vector<XGL_FENCE> fence_objs = make_objects<XGL_FENCE>(fences);
    XGL_RESULT err = xglWaitForFences(obj(), fence_objs.size(), &fence_objs[0], wait_all, timeout);
    EXPECT(err == XGL_SUCCESS || err == XGL_TIMEOUT);

    return err;
}

void Device::begin_descriptor_region_update(XGL_DESCRIPTOR_UPDATE_MODE mode)
{
    EXPECT(xglBeginDescriptorRegionUpdate(obj(), mode) == XGL_SUCCESS);
}

void Device::end_descriptor_region_update(CmdBuffer &cmd)
{
    EXPECT(xglEndDescriptorRegionUpdate(obj(), cmd.obj()) == XGL_SUCCESS);
}

void Queue::submit(const std::vector<const CmdBuffer *> &cmds, const std::vector<XGL_MEMORY_REF> &mem_refs, Fence &fence)
{
    const std::vector<XGL_CMD_BUFFER> cmd_objs = make_objects<XGL_CMD_BUFFER>(cmds);
    EXPECT(xglQueueSubmit(obj(), cmd_objs.size(), &cmd_objs[0], mem_refs.size(), &mem_refs[0], fence.obj()) == XGL_SUCCESS);
}

void Queue::submit(const CmdBuffer &cmd, const std::vector<XGL_MEMORY_REF> &mem_refs, Fence &fence)
{
    submit(std::vector<const CmdBuffer*>(1, &cmd), mem_refs, fence);
}

void Queue::submit(const CmdBuffer &cmd, const std::vector<XGL_MEMORY_REF> &mem_refs)
{
    Fence fence;
    submit(cmd, mem_refs, fence);
}

void Queue::set_global_mem_references(const std::vector<XGL_MEMORY_REF> &mem_refs)
{
    EXPECT(xglQueueSetGlobalMemReferences(obj(), mem_refs.size(), &mem_refs[0]) == XGL_SUCCESS);
}

void Queue::wait()
{
    EXPECT(xglQueueWaitIdle(obj()) == XGL_SUCCESS);
}

void Queue::signal_semaphore(QueueSemaphore &sem)
{
    EXPECT(xglSignalQueueSemaphore(obj(), sem.obj()) == XGL_SUCCESS);
}

void Queue::wait_semaphore(QueueSemaphore &sem)
{
    EXPECT(xglWaitQueueSemaphore(obj(), sem.obj()) == XGL_SUCCESS);
}

GpuMemory::~GpuMemory()
{
    if (initialized() && own())
        EXPECT(xglFreeMemory(obj()) == XGL_SUCCESS);
}

void GpuMemory::init(const Device &dev, const XGL_MEMORY_ALLOC_INFO &info)
{
    DERIVED_OBJECT_INIT(xglAllocMemory, dev.obj(), &info);
}

void GpuMemory::init(const Device &dev, size_t size, const void *data)
{
    DERIVED_OBJECT_INIT(xglPinSystemMemory, dev.obj(), data, size);
}

void GpuMemory::init(const Device &dev, const XGL_MEMORY_OPEN_INFO &info)
{
    DERIVED_OBJECT_INIT(xglOpenSharedMemory, dev.obj(), &info);
}

void GpuMemory::init(const Device &dev, const XGL_PEER_MEMORY_OPEN_INFO &info)
{
    DERIVED_OBJECT_INIT(xglOpenPeerMemory, dev.obj(), &info);
}

void GpuMemory::set_priority(XGL_MEMORY_PRIORITY priority)
{
    EXPECT(xglSetMemoryPriority(obj(), priority) == XGL_SUCCESS);
}

const void *GpuMemory::map(XGL_FLAGS flags) const
{
    void *data;
    if (!EXPECT(xglMapMemory(obj(), flags, &data) == XGL_SUCCESS))
        data = NULL;

    return data;
}

void *GpuMemory::map(XGL_FLAGS flags)
{
    void *data;
    if (!EXPECT(xglMapMemory(obj(), flags, &data) == XGL_SUCCESS))
        data = NULL;

    return data;
}

void GpuMemory::unmap() const
{
    EXPECT(xglUnmapMemory(obj()) == XGL_SUCCESS);
}

void Fence::init(const Device &dev, const XGL_FENCE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateFence, dev.obj(), &info);
    alloc_memory(dev);
}

void QueueSemaphore::init(const Device &dev, const XGL_QUEUE_SEMAPHORE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateQueueSemaphore, dev.obj(), &info);
    alloc_memory(dev);
}

void QueueSemaphore::init(const Device &dev, const XGL_QUEUE_SEMAPHORE_OPEN_INFO &info)
{
    DERIVED_OBJECT_INIT(xglOpenSharedQueueSemaphore, dev.obj(), &info);
}

void Event::init(const Device &dev, const XGL_EVENT_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateEvent, dev.obj(), &info);
    alloc_memory(dev);
}

void Event::set()
{
    EXPECT(xglSetEvent(obj()) == XGL_SUCCESS);
}

void Event::reset()
{
    EXPECT(xglResetEvent(obj()) == XGL_SUCCESS);
}

void QueryPool::init(const Device &dev, const XGL_QUERY_POOL_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateQueryPool, dev.obj(), &info);
    alloc_memory(dev);
}

XGL_RESULT QueryPool::results(uint32_t start, uint32_t count, size_t size, void *data)
{
    size_t tmp = size;
    XGL_RESULT err = xglGetQueryPoolResults(obj(), start, count, &tmp, data);
    if (err == XGL_SUCCESS) {
        if (!EXPECT(tmp == size))
            memset(data, 0, size);
    } else {
        EXPECT(err == XGL_NOT_READY);
    }

    return err;
}

void Buffer::init(const Device &dev, const XGL_BUFFER_CREATE_INFO &info)
{
    init_no_mem(dev, info);
    alloc_memory(dev, true, false);
}

void Buffer::init_no_mem(const Device &dev, const XGL_BUFFER_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateBuffer, dev.obj(), &info);
    create_info_ = info;
}

void BufferView::init(const Device &dev, const XGL_BUFFER_VIEW_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateBufferView, dev.obj(), &info);
    alloc_memory(dev);
}

void Image::init(const Device &dev, const XGL_IMAGE_CREATE_INFO &info)
{
    init_no_mem(dev, info);
    alloc_memory(dev, info.tiling == XGL_LINEAR_TILING, true);
}

void Image::init_no_mem(const Device &dev, const XGL_IMAGE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateImage, dev.obj(), &info);
    init_info(dev, info);
}

void Image::init(const Device &dev, const XGL_PEER_IMAGE_OPEN_INFO &info, const XGL_IMAGE_CREATE_INFO &original_info)
{
    XGL_IMAGE img;
    XGL_GPU_MEMORY mem;
    EXPECT(xglOpenPeerImage(dev.obj(), &info, &img, &mem) == XGL_SUCCESS);
    Object::init(img);

    init_info(dev, original_info);
    alloc_memory(std::vector<XGL_GPU_MEMORY>(1, mem));
}

void Image::init_info(const Device &dev, const XGL_IMAGE_CREATE_INFO &info)
{
    create_info_ = info;

    for (std::vector<Device::Format>::const_iterator it = dev.formats().begin(); it != dev.formats().end(); it++) {
        if (memcmp(&it->format, &create_info_.format, sizeof(it->format)) == 0 && it->tiling == create_info_.tiling) {
            format_features_ = it->features;
            break;
        }
    }
}

void Image::bind_memory(uint32_t alloc_idx, const XGL_IMAGE_MEMORY_BIND_INFO &info,
                        const GpuMemory &mem, XGL_GPU_SIZE mem_offset)
{
    EXPECT(!alloc_idx && xglBindImageMemoryRange(obj(), 0, &info, mem.obj(), mem_offset) == XGL_SUCCESS);
}

XGL_SUBRESOURCE_LAYOUT Image::subresource_layout(const XGL_IMAGE_SUBRESOURCE &subres) const
{
    const XGL_SUBRESOURCE_INFO_TYPE type = XGL_INFO_TYPE_SUBRESOURCE_LAYOUT;
    XGL_SUBRESOURCE_LAYOUT data;
    size_t size = sizeof(data);
    if (!EXPECT(xglGetImageSubresourceInfo(obj(), &subres, type, &size, &data) == XGL_SUCCESS && size == sizeof(data)))
        memset(&data, 0, sizeof(data));

    return data;
}

bool Image::transparent() const
{
    return (create_info_.tiling == XGL_LINEAR_TILING &&
            create_info_.samples == 1 &&
            !(create_info_.usage & (XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                    XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT)));
}

void ImageView::init(const Device &dev, const XGL_IMAGE_VIEW_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateImageView, dev.obj(), &info);
    alloc_memory(dev);
}

void ColorAttachmentView::init(const Device &dev, const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateColorAttachmentView, dev.obj(), &info);
    alloc_memory(dev);
}

void DepthStencilView::init(const Device &dev, const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateDepthStencilView, dev.obj(), &info);
    alloc_memory(dev);
}

void Shader::init(const Device &dev, const XGL_SHADER_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateShader, dev.obj(), &info);
}

XGL_RESULT Shader::init_try(const Device &dev, const XGL_SHADER_CREATE_INFO &info)
{
    XGL_SHADER sh;
    XGL_RESULT err = xglCreateShader(dev.obj(), &info, &sh);
    if (err == XGL_SUCCESS)
        Object::init(sh);

    return err;
}

void Pipeline::init(const Device &dev, const XGL_GRAPHICS_PIPELINE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateGraphicsPipeline, dev.obj(), &info);
    alloc_memory(dev);
}

void Pipeline::init(const Device &dev, const XGL_COMPUTE_PIPELINE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateComputePipeline, dev.obj(), &info);
    alloc_memory(dev);
}

void Pipeline::init(const Device&dev, size_t size, const void *data)
{
    DERIVED_OBJECT_INIT(xglLoadPipeline, dev.obj(), size, data);
    alloc_memory(dev);
}

size_t Pipeline::store(size_t size, void *data)
{
    if (!EXPECT(xglStorePipeline(obj(), &size, data) == XGL_SUCCESS))
        size = 0;

    return size;
}

void PipelineDelta::init(const Device &dev, const Pipeline &p1, const Pipeline &p2)
{
    DERIVED_OBJECT_INIT(xglCreatePipelineDelta, dev.obj(), p1.obj(), p2.obj());
    alloc_memory(dev);
}

void Sampler::init(const Device &dev, const XGL_SAMPLER_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateSampler, dev.obj(), &info);
    alloc_memory(dev);
}

void DescriptorSetLayout::init(const Device &dev, XGL_FLAGS stage_mask,
                               const std::vector<uint32_t> &bind_points,
                               const DescriptorSetLayout &prior_layout,
                               const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateDescriptorSetLayout, dev.obj(), stage_mask,
            &bind_points[0], prior_layout.obj(), &info);
    alloc_memory(dev);
}

void DescriptorSetLayout::init(const Device &dev, uint32_t bind_point,
                               const DescriptorSetLayout &prior_layout,
                               const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO &info)
{
    init(dev, XGL_SHADER_STAGE_FLAGS_ALL, std::vector<uint32_t>(1, bind_point), prior_layout, info);
}

void DescriptorRegion::init(const Device &dev, XGL_DESCRIPTOR_REGION_USAGE usage,
                            uint32_t max_sets, const XGL_DESCRIPTOR_REGION_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateDescriptorRegion, dev.obj(), usage, max_sets, &info);
    alloc_memory(dev);
}

void DescriptorRegion::clear()
{
    EXPECT(xglClearDescriptorRegion(obj()) == XGL_SUCCESS);
}

std::vector<DescriptorSet *> DescriptorRegion::alloc_sets(XGL_DESCRIPTOR_SET_USAGE usage, const std::vector<const DescriptorSetLayout *> &layouts)
{
    const std::vector<XGL_DESCRIPTOR_SET_LAYOUT> layout_objs = make_objects<XGL_DESCRIPTOR_SET_LAYOUT>(layouts);

    std::vector<XGL_DESCRIPTOR_SET> set_objs;
    set_objs.resize(layout_objs.size());

    uint32_t set_count;
    XGL_RESULT err = xglAllocDescriptorSets(obj(), usage, layout_objs.size(), &layout_objs[0], &set_objs[0], &set_count);
    if (err == XGL_SUCCESS)
        EXPECT(set_count == set_objs.size());
    set_objs.resize(set_count);

    std::vector<DescriptorSet *> sets;
    sets.reserve(set_count);
    for (std::vector<XGL_DESCRIPTOR_SET>::const_iterator it = set_objs.begin(); it != set_objs.end(); it++) {
        // do descriptor sets need memories bound?
        sets.push_back(new DescriptorSet(*it));
    }

    return sets;
}

std::vector<DescriptorSet *> DescriptorRegion::alloc_sets(XGL_DESCRIPTOR_SET_USAGE usage, const DescriptorSetLayout &layout, uint32_t count)
{
    return alloc_sets(usage, std::vector<const DescriptorSetLayout *>(count, &layout));
}

DescriptorSet *DescriptorRegion::alloc_sets(XGL_DESCRIPTOR_SET_USAGE usage, const DescriptorSetLayout &layout)
{
    std::vector<DescriptorSet *> set = alloc_sets(usage, layout, 1);
    return (set.empty()) ? NULL : set[0];
}

void DescriptorRegion::clear_sets(const std::vector<DescriptorSet *> &sets)
{
    const std::vector<XGL_DESCRIPTOR_SET> set_objs = make_objects<XGL_DESCRIPTOR_SET>(sets);
    xglClearDescriptorSets(obj(), set_objs.size(), &set_objs[0]);
}

void DescriptorSet::update(const void *update_chain)
{
    xglUpdateDescriptors(obj(), update_chain);
}

void DynamicVpStateObject::init(const Device &dev, const XGL_DYNAMIC_VP_STATE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateDynamicViewportState, dev.obj(), &info);
    alloc_memory(dev);
}

void DynamicRsStateObject::init(const Device &dev, const XGL_DYNAMIC_RS_STATE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateDynamicRasterState, dev.obj(), &info);
    alloc_memory(dev);
}

void DynamicCbStateObject::init(const Device &dev, const XGL_DYNAMIC_CB_STATE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateDynamicColorBlendState, dev.obj(), &info);
    alloc_memory(dev);
}

void DynamicDsStateObject::init(const Device &dev, const XGL_DYNAMIC_DS_STATE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateDynamicDepthStencilState, dev.obj(), &info);
    alloc_memory(dev);
}

void CmdBuffer::init(const Device &dev, const XGL_CMD_BUFFER_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateCommandBuffer, dev.obj(), &info);
}

void CmdBuffer::begin(const XGL_CMD_BUFFER_BEGIN_INFO *info)
{
    EXPECT(xglBeginCommandBuffer(obj(), info) == XGL_SUCCESS);
}

void CmdBuffer::begin(XGL_RENDER_PASS renderpass_obj)
{
    XGL_CMD_BUFFER_BEGIN_INFO info = {};
    XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO graphics_cmd_buf_info = {
        .sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = renderpass_obj,
    };
    info.flags = XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT |
          XGL_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT;
    info.sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;
    info.pNext = &graphics_cmd_buf_info;

    begin(&info);
}

void CmdBuffer::begin()
{
    XGL_CMD_BUFFER_BEGIN_INFO info = {};
    info.flags = XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT |
          XGL_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT;
    info.sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;

    begin(&info);
}

void CmdBuffer::end()
{
    EXPECT(xglEndCommandBuffer(obj()) == XGL_SUCCESS);
}

void CmdBuffer::reset()
{
    EXPECT(xglResetCommandBuffer(obj()) == XGL_SUCCESS);
}

}; // namespace xgl_testing
