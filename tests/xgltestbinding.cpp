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
    if (!EXPECT(xglEnumerateLayers(gpu_, max_layer_count, max_string_size, out, &count, NULL) == XGL_SUCCESS))
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
    return memory_requirements().size();
}

std::vector<XGL_MEMORY_REQUIREMENTS> BaseObject::memory_requirements() const
{
    std::vector<XGL_MEMORY_REQUIREMENTS> info =
        get_info<XGL_MEMORY_REQUIREMENTS>(obj_, XGL_INFO_TYPE_MEMORY_REQUIREMENTS, 0);
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
    EXPECT(!alloc_idx && xglBindObjectMemory(obj(), mem.obj(), mem_offset) == XGL_SUCCESS);
}

void Object::unbind_memory(uint32_t alloc_idx)
{
    EXPECT(!alloc_idx && xglBindObjectMemory(obj(), XGL_NULL_HANDLE, 0) == XGL_SUCCESS);
}

void Object::unbind_memory()
{
    for (uint32_t i = 0; i < mem_alloc_count_; i++)
        unbind_memory(i);
}

void Object::alloc_memory(const Device &dev, bool for_linear_img)
{
    if (!EXPECT(!internal_mems_) || !mem_alloc_count_)
        return;

    internal_mems_ = new GpuMemory[mem_alloc_count_];

    const std::vector<XGL_MEMORY_REQUIREMENTS> mem_reqs = memory_requirements();
    for (int i = 0; i < mem_reqs.size(); i++) {
        XGL_MEMORY_ALLOC_INFO info = GpuMemory::alloc_info(mem_reqs[i]);

        primary_mem_ = &internal_mems_[i];

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
    dev_info.pNext = static_cast<void *>(&layer_info);
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
    init_heap_props();
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

void Device::init_heap_props()
{
    uint32_t count;
    if (!EXPECT(xglGetMemoryHeapCount(obj(), &count) == XGL_SUCCESS && count))
        return;
    if (count > XGL_MAX_MEMORY_HEAPS)
        count = XGL_MAX_MEMORY_HEAPS;

    heap_props_.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        const XGL_MEMORY_HEAP_INFO_TYPE type = XGL_INFO_TYPE_MEMORY_HEAP_PROPERTIES;
        XGL_MEMORY_HEAP_PROPERTIES props;
        XGL_SIZE size = sizeof(props);
        if (EXPECT(xglGetMemoryHeapInfo(obj(), i, type, &size, &props) == XGL_SUCCESS && size == sizeof(props)))
            heap_props_.push_back(props);
    }
}

void Device::init_formats()
{
    for (int ch = XGL_CH_FMT_UNDEFINED; ch <= XGL_MAX_CH_FMT; ch++) {
        for (int num = XGL_NUM_FMT_UNDEFINED; num <= XGL_MAX_NUM_FMT; num++) {
            const XGL_FORMAT fmt = { static_cast<XGL_CHANNEL_FORMAT>(ch),
                                     static_cast<XGL_NUM_FORMAT>(num) };
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

void GpuMemory::init(const Device &dev, XGL_GPU_SIZE size)
{
    XGL_MEMORY_ALLOC_INFO info = {};
    info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    info.allocationSize = size;
    info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

    // find CPU visible heaps
    for (XGL_UINT id = 0; id < dev.heap_properties().size(); id++) {
        const XGL_MEMORY_HEAP_PROPERTIES &heap = dev.heap_properties()[id];
        if (heap.flags & XGL_MEMORY_HEAP_CPU_VISIBLE_BIT)
            info.heaps[info.heapCount++] = id;
    }

    EXPECT(info.heapCount);
    init(dev, info);
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

void Image::init(const Device &dev, const XGL_IMAGE_CREATE_INFO &info)
{
    init_no_mem(dev, info);
    alloc_memory(dev, info.tiling == XGL_LINEAR_TILING);
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

void DescriptorSet::init(const Device &dev, const XGL_DESCRIPTOR_SET_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateDescriptorSet, dev.obj(), &info);
    info_ = info;
}

void DescriptorSet::attach(uint32_t start_slot, const std::vector<const Sampler *> &samplers)
{
    const std::vector<XGL_SAMPLER> sampler_objs = make_objects<XGL_SAMPLER>(samplers);
    xglAttachSamplerDescriptors(obj(), start_slot, sampler_objs.size(), &sampler_objs[0]);
}

void DescriptorSet::attach(uint32_t start_slot, const std::vector<XGL_IMAGE_VIEW_ATTACH_INFO> &img_views)
{
    xglAttachImageViewDescriptors(obj(), start_slot, img_views.size(), &img_views[0]);
}

void DescriptorSet::attach(uint32_t start_slot, const std::vector<XGL_MEMORY_VIEW_ATTACH_INFO> &mem_views)
{
    xglAttachMemoryViewDescriptors(obj(), start_slot, mem_views.size(), &mem_views[0]);
}

void DescriptorSet::attach(uint32_t start_slot, const std::vector<XGL_DESCRIPTOR_SET_ATTACH_INFO> &sets)
{
    xglAttachNestedDescriptors(obj(), start_slot, sets.size(), &sets[0]);
}

void DynamicVpStateObject::init(const Device &dev, const XGL_VIEWPORT_STATE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateViewportState, dev.obj(), &info);
    alloc_memory(dev);
}

void DynamicRsStateObject::init(const Device &dev, const XGL_RASTER_STATE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateRasterState, dev.obj(), &info);
    alloc_memory(dev);
}

void DynamicMsaaStateObject::init(const Device &dev, const XGL_MSAA_STATE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateMsaaState, dev.obj(), &info);
    alloc_memory(dev);
}

void DynamicCbStateObject::init(const Device &dev, const XGL_COLOR_BLEND_STATE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateColorBlendState, dev.obj(), &info);
    alloc_memory(dev);
}

void DynamicDsStateObject::init(const Device &dev, const XGL_DEPTH_STENCIL_STATE_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateDepthStencilState, dev.obj(), &info);
    alloc_memory(dev);
}

void CmdBuffer::init(const Device &dev, const XGL_CMD_BUFFER_CREATE_INFO &info)
{
    DERIVED_OBJECT_INIT(xglCreateCommandBuffer, dev.obj(), &info);
}

void CmdBuffer::begin(XGL_FLAGS flags)
{
    EXPECT(xglBeginCommandBuffer(obj(), flags) == XGL_SUCCESS);
}

void CmdBuffer::begin()
{
    begin(XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT |
          XGL_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT);
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
