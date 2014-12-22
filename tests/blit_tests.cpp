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

// Blit (copy, clear, and resolve) tests

#include <set>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "xgl.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

namespace xgl_testing {

class Buffer;
class CmdBuffer;
class Device;
class Image;

XGL_SIZE get_format_size(XGL_FORMAT format);
XGL_EXTENT3D get_mip_level_extent(const XGL_EXTENT3D &extent, XGL_UINT mip_level);

class Environment : public ::testing::Environment {
public:
    Environment();

    bool parse_args(int argc, char **argv);

    virtual void SetUp();
    virtual void TearDown();

    const std::vector<Device *> &devices() { return devs_; }
    Device &default_device() { return *(devs_[default_dev_]); }

private:
    XGL_APPLICATION_INFO app_;
    int default_dev_;

    std::vector<Device *> devs_;
};

class Gpu {
public:
    explicit Gpu(XGL_PHYSICAL_GPU gpu) : gpu_(gpu) {}
    bool init();

    XGL_PHYSICAL_GPU obj() const { return gpu_; }
    const XGL_PHYSICAL_GPU_PROPERTIES &properties() const { return props_; };
    const XGL_PHYSICAL_GPU_PERFORMANCE &performance() const { return perf_; };
    const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES &memory_properties() const { return mem_props_; };
    const std::vector<XGL_PHYSICAL_GPU_QUEUE_PROPERTIES> &queue_properties() const { return queue_props_; }
    const std::vector<const XGL_CHAR *> &extensions() const { return exts_; }

private:
    void init_exts();

    XGL_PHYSICAL_GPU gpu_;
    XGL_PHYSICAL_GPU_PROPERTIES props_;
    XGL_PHYSICAL_GPU_PERFORMANCE perf_;
    XGL_PHYSICAL_GPU_MEMORY_PROPERTIES mem_props_;
    std::vector<XGL_PHYSICAL_GPU_QUEUE_PROPERTIES> queue_props_;
    std::vector<const XGL_CHAR *> exts_;
};

class Device {
public:
    explicit Device(XGL_PHYSICAL_GPU gpu) : gpu_(gpu), dev_(XGL_NULL_HANDLE) {}
    ~Device();
    bool init();

    XGL_DEVICE obj() const { return dev_; }
    XGL_PHYSICAL_GPU gpu() const { return gpu_.obj(); }

    XGL_QUEUE queue(XGL_QUEUE_TYPE type, XGL_UINT idx) const;

    const std::vector<XGL_MEMORY_HEAP_PROPERTIES> &heap_properties() const { return heap_props_; }

    struct Format {
        XGL_FORMAT format;
        XGL_IMAGE_TILING tiling;
        XGL_FLAGS features;
    };
    const std::vector<Format> &formats() const { return formats_; }

    bool submit(XGL_QUEUE queue, const CmdBuffer &cmd, XGL_FENCE fence);
    bool wait(XGL_QUEUE queue) { return (xglQueueWaitIdle(queue) == XGL_SUCCESS); }
    bool wait() { return (xglDeviceWaitIdle(dev_) == XGL_SUCCESS); }

private:
    Device(const Device &);
    Device &operator=(const Device &);

    void init_queues();
    void init_heap_props();
    void init_formats();

    Gpu gpu_;
    XGL_DEVICE dev_;
    std::vector<XGL_QUEUE> graphics_queues_;
    std::vector<XGL_QUEUE> compute_queues_;
    std::vector<XGL_QUEUE> dma_queues_;
    std::vector<XGL_MEMORY_HEAP_PROPERTIES> heap_props_;
    std::vector<Format> formats_;
};

class Object {
public:
    const XGL_MEMORY_REQUIREMENTS &memory_requirements() const { return mem_reqs_; }
    bool bind_memory(XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset);
    XGL_GPU_MEMORY alloc_memory(const Device &dev);

    XGL_GPU_MEMORY bound_memory() const { return bound_mem_; }

protected:
    Object() : obj_(XGL_NULL_HANDLE), bound_mem_(XGL_NULL_HANDLE) {}
    ~Object();
    bool init(XGL_OBJECT obj);

private:
    Object(const Object &);
    Object &operator=(const Object &);

    XGL_OBJECT obj_;
    XGL_MEMORY_REQUIREMENTS mem_reqs_;

protected:
    // not private because of Buffer, which is not an XGL_OBJECT yet
    XGL_GPU_MEMORY bound_mem_;

};

class CmdBuffer : public Object {
public:
    CmdBuffer() : cmd_(XGL_NULL_HANDLE) {}
    bool init(const Device &dev, const XGL_CMD_BUFFER_CREATE_INFO &info);
    bool init(const Device &dev);

    XGL_CMD_BUFFER obj() const { return cmd_; }

    void add_memory_ref(const Object &obj, XGL_FLAGS flags);
    void clear_memory_refs() { mem_refs_.clear(); }

    std::vector<XGL_MEMORY_REF> memory_refs() const
    {
        return std::vector<XGL_MEMORY_REF>(mem_refs_.begin(), mem_refs_.end());
    }

    bool begin(XGL_FLAGS flags);
    bool begin();
    bool end();

private:
    class mem_ref_compare {
    public:
        bool operator()(const XGL_MEMORY_REF &a, const XGL_MEMORY_REF &b) const
        {
            return (a.mem < b.mem);
        }
    };

    XGL_CMD_BUFFER_CREATE_INFO info_;
    XGL_CMD_BUFFER cmd_;
    std::set<XGL_MEMORY_REF, mem_ref_compare> mem_refs_;
};

class Buffer : public Object {
public:
    Buffer() : mem_(XGL_NULL_HANDLE) {}
    ~Buffer();
    bool init(const Device &dev, const XGL_MEMORY_ALLOC_INFO &info);
    bool init(const Device &dev, XGL_GPU_SIZE size);

    XGL_GPU_MEMORY obj() const { return mem_; }
    XGL_GPU_SIZE size() const { return info_.allocationSize; }

    XGL_MEMORY_STATE_TRANSITION prepare(XGL_MEMORY_STATE old_state, XGL_MEMORY_STATE new_state,
                                        XGL_GPU_SIZE offset, XGL_GPU_SIZE size)
    {
        XGL_MEMORY_STATE_TRANSITION transition = {};
        transition.sType = XGL_STRUCTURE_TYPE_MEMORY_STATE_TRANSITION;
        transition.mem = mem_;
        transition.oldState = old_state;
        transition.newState = new_state;
        transition.offset = offset;
        transition.regionSize = size;
        return transition;
    }

    void *map();
    void unmap();

private:
    XGL_MEMORY_ALLOC_INFO info_;
    XGL_GPU_MEMORY mem_;
};

class Image : public Object {
public:
    Image() : features_(0), img_(XGL_NULL_HANDLE), mem_(XGL_NULL_HANDLE) {}
    ~Image();
    bool init(const Device &dev, const XGL_IMAGE_CREATE_INFO &info);

    static XGL_IMAGE_CREATE_INFO create_info()
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

    static XGL_IMAGE_SUBRESOURCE subresource(XGL_IMAGE_ASPECT aspect, XGL_UINT mip_level, XGL_UINT array_slice)
    {
        XGL_IMAGE_SUBRESOURCE subres = {};
        subres.aspect = aspect;
        subres.mipLevel = mip_level;
        subres.arraySlice = array_slice;
        return subres;
    }

    static XGL_IMAGE_SUBRESOURCE subresource(const XGL_IMAGE_SUBRESOURCE_RANGE &range,
                                             XGL_UINT mip_level, XGL_UINT array_slice)
    {
        return subresource(range.aspect, range.baseMipLevel + mip_level, range.baseArraySlice + array_slice);
    }

    static XGL_IMAGE_SUBRESOURCE_RANGE subresource_range(XGL_IMAGE_ASPECT aspect,
                                                         XGL_UINT base_mip_level, XGL_UINT mip_levels,
                                                         XGL_UINT base_array_slice, XGL_UINT array_size)
    {
        XGL_IMAGE_SUBRESOURCE_RANGE range = {};
        range.aspect = aspect;
        range.baseMipLevel = base_mip_level;
        range.mipLevels = mip_levels;
        range.baseArraySlice = base_array_slice;
        range.arraySize = array_size;
        return range;
    }

    static XGL_IMAGE_SUBRESOURCE_RANGE subresource_range(XGL_IMAGE_ASPECT aspect,
                                                         const XGL_IMAGE_CREATE_INFO &info)
    {
        return subresource_range(aspect, 0, info.mipLevels, 0, info.arraySize);
    }

    static XGL_IMAGE_SUBRESOURCE_RANGE subresource_range(const XGL_IMAGE_SUBRESOURCE &subres)
    {
        return subresource_range(subres.aspect, subres.mipLevel, 1, subres.arraySlice, 1);
    }

    XGL_IMAGE obj() const { return img_; }

    bool copyable() const { return (features_ & XGL_FORMAT_IMAGE_COPY_BIT); }
    bool transparent() const;
    XGL_EXTENT3D extent(XGL_UINT mip_level) const { return get_mip_level_extent(info_.extent, mip_level); }
    XGL_EXTENT3D extent() const { return info_.extent; }

    XGL_IMAGE_SUBRESOURCE_RANGE subresource_range(XGL_IMAGE_ASPECT aspect) const
    {
        return subresource_range(aspect, info_);
    }

    XGL_SUBRESOURCE_LAYOUT subresource_layout(const XGL_IMAGE_SUBRESOURCE &subres) const;

    XGL_IMAGE_STATE_TRANSITION prepare(XGL_IMAGE_STATE old_state, XGL_IMAGE_STATE new_state,
                                       const XGL_IMAGE_SUBRESOURCE_RANGE &range) const
    {
        XGL_IMAGE_STATE_TRANSITION transition = {};
        transition.image = img_;
        transition.oldState = old_state;
        transition.newState = new_state;
        transition.subresourceRange = range;
        return transition;
    }

    void *map() const;
    void unmap() const;

private:
    XGL_IMAGE_CREATE_INFO info_;
    XGL_FLAGS features_;
    XGL_IMAGE img_;
    XGL_GPU_MEMORY mem_;
};

class ImageChecker {
public:
    explicit ImageChecker(const XGL_IMAGE_CREATE_INFO &info, const std::vector<XGL_MEMORY_IMAGE_COPY> &regions)
        : info_(info), regions_(regions), pattern_(HASH) {}
    explicit ImageChecker(const XGL_IMAGE_CREATE_INFO &info, const std::vector<XGL_IMAGE_SUBRESOURCE_RANGE> &ranges);
    explicit ImageChecker(const XGL_IMAGE_CREATE_INFO &info);

    void set_solid_pattern(const std::vector<uint8_t> &solid);

    XGL_GPU_SIZE buffer_size() const;
    bool fill(Buffer &buf) const { return walk(FILL, buf); }
    bool fill(Image &img) const { return walk(FILL, img); }
    bool check(Buffer &buf) const { return walk(CHECK, buf); }
    bool check(Image &img) const { return walk(CHECK, img); }

    const std::vector<XGL_MEMORY_IMAGE_COPY> &regions() const { return regions_; }

    static void hash_salt_generate() { hash_salt_++; }

private:
    enum Action {
        FILL,
        CHECK,
    };

    enum Pattern {
        HASH,
        SOLID,
    };

    XGL_SIZE buffer_cpp() const;
    XGL_SUBRESOURCE_LAYOUT buffer_layout(const XGL_MEMORY_IMAGE_COPY &region) const;

    bool walk(Action action, Buffer &buf) const;
    bool walk(Action action, Image &img) const;
    bool walk_region(Action action, const XGL_MEMORY_IMAGE_COPY &region, const XGL_SUBRESOURCE_LAYOUT &layout, void *data) const;

    std::vector<uint8_t> pattern_hash(const XGL_IMAGE_SUBRESOURCE &subres, const XGL_OFFSET3D &offset) const;

    static uint32_t hash_salt_;

    XGL_IMAGE_CREATE_INFO info_;
    std::vector<XGL_MEMORY_IMAGE_COPY> regions_;

    Pattern pattern_;
    std::vector<uint8_t> pattern_solid_;
};

Environment::Environment() :
    default_dev_(0)
{
    app_.sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO;
    app_.pAppName = (const XGL_CHAR *) "xgl_testing";
    app_.appVersion = 1;
    app_.pEngineName = (const XGL_CHAR *) "xgl_testing";
    app_.engineVersion = 1;
    app_.apiVersion = XGL_MAKE_VERSION(0, 22, 0);
}

bool Environment::parse_args(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
#define ARG(name) (strcmp(argv[i], name) == 0)
#define ARG_P(name) (i < argc - 1 && ARG(name))
        if (ARG_P("--gpu")) {
            default_dev_ = atoi(argv[++i]);
        } else {
            break;
        }
#undef ARG
#undef ARG_P
    }

    if (i < argc) {
        std::cout <<
            "invalid argument: " << argv[i] << "\n\n" <<
            "Usage: " << argv[0] << " <options>\n\n" <<
            "Options:\n"
            "  --gpu <n>  Use GPU<n> as the default GPU\n";

        return false;
    }

    return true;
}

void Environment::SetUp()
{
    XGL_PHYSICAL_GPU gpus[XGL_MAX_PHYSICAL_GPUS];
    XGL_UINT count;
    XGL_RESULT err;

    err = xglInitAndEnumerateGpus(&app_, NULL, ARRAY_SIZE(gpus), &count, gpus);
    ASSERT_EQ(XGL_SUCCESS, err);
    ASSERT_GT(count, default_dev_);

    devs_.reserve(count);
    for (XGL_UINT i = 0; i < count; i++) {
        devs_.push_back(new Device(gpus[i]));
        if (i == default_dev_) {
            const bool created = devs_[i]->init();
            ASSERT_EQ(true, created);
        }
    }
}

void Environment::TearDown()
{
    // destroy devices first
    for (std::vector<Device *>::iterator it = devs_.begin(); it != devs_.end(); it++)
        delete *it;
    devs_.clear();

    XGL_UINT dummy_count;
    xglInitAndEnumerateGpus(&app_, NULL, 0, &dummy_count, NULL);
}

bool Gpu::init()
{
    XGL_SIZE size;
    XGL_RESULT err;

    size = sizeof(props_);
    err = xglGetGpuInfo(gpu_, XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES, &size, &props_);
    if (err != XGL_SUCCESS || size != sizeof(props_))
        return false;

    size = sizeof(perf_);
    err = xglGetGpuInfo(gpu_, XGL_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE, &size, &perf_);
    if (err != XGL_SUCCESS || size != sizeof(perf_))
        return false;

    size = sizeof(mem_props_);
    err = xglGetGpuInfo(gpu_, XGL_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES, &size, &mem_props_);
    if (err != XGL_SUCCESS || size != sizeof(mem_props_))
        return false;

    err = xglGetGpuInfo(gpu_, XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES, &size, NULL);
    if (err != XGL_SUCCESS || size % sizeof(queue_props_[0]))
        return false;

    queue_props_.resize(size / sizeof(queue_props_[0]));
    err = xglGetGpuInfo(gpu_, XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES, &size, &queue_props_[0]);
    if (err != XGL_SUCCESS || size != queue_props_.size() * sizeof(queue_props_[0]))
        return false;

    init_exts();

    return true;
}

void Gpu::init_exts()
{
    static const XGL_CHAR *known_exts[] = {
        (const XGL_CHAR *) "XGL_WSI_X11",
    };
    XGL_RESULT err;

    for (int i; i < ARRAY_SIZE(known_exts); i++) {
        err = xglGetExtensionSupport(gpu_, known_exts[i]);
        if (err == XGL_SUCCESS)
            exts_.push_back(known_exts[i]);
    }
}

Device::~Device()
{
    if (dev_ != XGL_NULL_HANDLE)
        xglDestroyDevice(dev_);
}

bool Device::init()
{
    XGL_RESULT err;

    if (dev_ != XGL_NULL_HANDLE)
        return true;

    if (!gpu_.init())
        return false;

    // request all queues
    std::vector<XGL_DEVICE_QUEUE_CREATE_INFO> queue_info;
    queue_info.reserve(gpu_.queue_properties().size());
    for (XGL_UINT i = 0; i < gpu_.queue_properties().size(); i++) {
        XGL_DEVICE_QUEUE_CREATE_INFO qi = {};
        qi.queueNodeIndex = i;
        qi.queueCount = gpu_.queue_properties()[i].queueCount;
        queue_info.push_back(qi);
    }

    XGL_DEVICE_CREATE_INFO dev_info = {};
    dev_info.sType = XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_info.queueRecordCount = queue_info.size();
    dev_info.pRequestedQueues = &queue_info[0];
    dev_info.extensionCount = gpu_.extensions().size();
    dev_info.ppEnabledExtensionNames = &gpu_.extensions()[0];
    dev_info.maxValidationLevel = XGL_VALIDATION_LEVEL_END_RANGE;
    dev_info.flags = XGL_DEVICE_CREATE_VALIDATION_BIT;

    err = xglCreateDevice(gpu_.obj(), &dev_info, &dev_);
    if (err != XGL_SUCCESS)
        return false;

    init_queues();
    init_heap_props();
    init_formats();

    if (graphics_queues_.empty() || heap_props_.empty() || formats_.empty()) {
        xglDestroyDevice(dev_);
        dev_ = XGL_NULL_HANDLE;
        return false;
    }

    return true;
}

void Device::init_queues()
{
    const struct {
        XGL_QUEUE_TYPE type;
        std::vector<XGL_QUEUE> &queues;
    } known_queues[] = {
        { XGL_QUEUE_TYPE_GRAPHICS, graphics_queues_ },
        { XGL_QUEUE_TYPE_COMPUTE, compute_queues_ },
        { XGL_QUEUE_TYPE_DMA, dma_queues_ },
    };
    XGL_RESULT err;

    for (int i = 0; i < ARRAY_SIZE(known_queues); i++) {
        XGL_UINT idx = 0;

        while (true) {
            XGL_QUEUE queue;
            err = xglGetDeviceQueue(dev_, known_queues[i].type, idx++, &queue);
            if (err != XGL_SUCCESS)
                break;
            known_queues[i].queues.push_back(queue);
        }
    }
}

void Device::init_heap_props()
{
    XGL_UINT count;
    XGL_RESULT err;

    err = xglGetMemoryHeapCount(dev_, &count);
    if (err != XGL_SUCCESS || !count)
        return;

    heap_props_.reserve(count);
    for (XGL_UINT i = 0; i < count; i++) {
        XGL_MEMORY_HEAP_PROPERTIES props;
        XGL_SIZE size = sizeof(props);

        err = xglGetMemoryHeapInfo(dev_, i, XGL_INFO_TYPE_MEMORY_HEAP_PROPERTIES, &size, &props);
        if (err == XGL_SUCCESS && size == sizeof(props))
            heap_props_.push_back(props);
    }
}

void Device::init_formats()
{
    XGL_RESULT err;

    for (int ch = XGL_CH_FMT_UNDEFINED; ch <= XGL_MAX_CH_FMT; ch++) {
        for (int num = XGL_NUM_FMT_UNDEFINED; num <= XGL_MAX_NUM_FMT; num++) {
            const XGL_FORMAT fmt = { static_cast<XGL_CHANNEL_FORMAT>(ch),
                                     static_cast<XGL_NUM_FORMAT>(num) };

            XGL_FORMAT_PROPERTIES props;
            XGL_SIZE size = sizeof(props);
            err = xglGetFormatInfo(dev_, fmt, XGL_INFO_TYPE_FORMAT_PROPERTIES, &size, &props);
            if (err != XGL_SUCCESS || size != sizeof(props))
                continue;

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
}

XGL_QUEUE Device::queue(XGL_QUEUE_TYPE type, XGL_UINT idx) const
{
    switch (type) {
    case XGL_QUEUE_TYPE_GRAPHICS:   return graphics_queues_[idx];
    case XGL_QUEUE_TYPE_COMPUTE:    return compute_queues_[idx];
    case XGL_QUEUE_TYPE_DMA:        return dma_queues_[idx];
    default:                        return XGL_NULL_HANDLE;
    }
}

bool Device::submit(XGL_QUEUE queue, const CmdBuffer &cmd, XGL_FENCE fence)
{
    XGL_CMD_BUFFER obj = cmd.obj();
    const std::vector<XGL_MEMORY_REF> refs = cmd.memory_refs();

    return (xglQueueSubmit(queue, 1, &obj, refs.size(), &refs[0], fence) == XGL_SUCCESS);
}

Object::~Object()
{
    if (obj_ != XGL_NULL_HANDLE) {
        xglBindObjectMemory(obj_, XGL_NULL_HANDLE, 0);
        xglDestroyObject(obj_);
    }
}

bool Object::init(XGL_OBJECT obj)
{
    XGL_RESULT err;

    XGL_SIZE size = sizeof(mem_reqs_);
    err = xglGetObjectInfo(obj, XGL_INFO_TYPE_MEMORY_REQUIREMENTS, &size, &mem_reqs_);
    if (err != XGL_SUCCESS || size != sizeof(mem_reqs_))
        return false;

    obj_ = obj;

    return true;
}

bool Object::bind_memory(XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset)
{
    if (xglBindObjectMemory(obj_, mem, offset) == XGL_SUCCESS) {
        bound_mem_ = mem;
        return true;
    } else {
        return false;
    }
}

XGL_GPU_MEMORY Object::alloc_memory(const Device &dev)
{
    if (!mem_reqs_.size)
        return XGL_NULL_HANDLE;

    XGL_MEMORY_ALLOC_INFO mem_alloc = {};
    mem_alloc.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_alloc.allocationSize = mem_reqs_.size;
    mem_alloc.alignment = mem_reqs_.alignment;
    mem_alloc.heapCount = mem_reqs_.heapCount;
    memcpy(mem_alloc.heaps, mem_reqs_.heaps,
            sizeof(mem_reqs_.heaps[0]) * mem_reqs_.heapCount);
    mem_alloc.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

    XGL_GPU_MEMORY mem;
    XGL_RESULT err = xglAllocMemory(dev.obj(), &mem_alloc, &mem);
    if (err != XGL_SUCCESS)
        mem = XGL_NULL_HANDLE;

    return mem;
}

bool CmdBuffer::init(const Device &dev, const XGL_CMD_BUFFER_CREATE_INFO &info)
{
    info_ = info;

    XGL_RESULT err = xglCreateCommandBuffer(dev.obj(), &info_, &cmd_);
    if (err != XGL_SUCCESS)
        return false;

    return Object::init(cmd_);
}

bool CmdBuffer::init(const Device &dev)
{
    XGL_CMD_BUFFER_CREATE_INFO info = {};

    info.sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
    info.queueType = XGL_QUEUE_TYPE_GRAPHICS;

    return init(dev, info);
}

void CmdBuffer::add_memory_ref(const Object &obj, XGL_FLAGS flags)
{
    XGL_MEMORY_REF ref = {};
    ref.mem = obj.bound_memory();
    ref.flags = flags;

    std::pair<std::set<XGL_MEMORY_REF>::iterator, bool> inserted = mem_refs_.insert(ref);
    if (!inserted.second) {
        const XGL_FLAGS prev_flags = (*inserted.first).flags;

        if ((prev_flags & flags) != prev_flags) {
            mem_refs_.erase(inserted.first);
            mem_refs_.insert(ref);
        }
    }
}

bool CmdBuffer::begin(XGL_FLAGS flags)
{
    return (xglBeginCommandBuffer(cmd_, flags) == XGL_SUCCESS);
}

bool CmdBuffer::begin()
{
    return begin(XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT |
                 XGL_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT);
}

bool CmdBuffer::end()
{
    return (xglEndCommandBuffer(cmd_) == XGL_SUCCESS);
}

Buffer::~Buffer()
{
    if (mem_ != XGL_NULL_HANDLE)
        xglFreeMemory(mem_);
}

bool Buffer::init(const Device &dev, const XGL_MEMORY_ALLOC_INFO &info)
{
    info_ = info;

    // cannot call Object::init()
    if (xglAllocMemory(dev.obj(), &info_, &mem_) == XGL_SUCCESS) {
        bound_mem_ = mem_;
        return true;
    } else {
        return false;
    }
}

bool Buffer::init(const Device &dev, XGL_GPU_SIZE size)
{
    XGL_MEMORY_ALLOC_INFO info = {};
    info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    info.allocationSize = size;
    info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

    // find a CPU visible heap
    for (XGL_UINT id = 0; id < dev.heap_properties().size(); id++) {
        const XGL_MEMORY_HEAP_PROPERTIES &heap = dev.heap_properties()[id];

        if (heap.flags & XGL_MEMORY_HEAP_CPU_VISIBLE_BIT) {
            info.heapCount = 1;
            info.heaps[0] = id;
        }
    }

    return (info.heapCount) ? init(dev, info) : false;
}

void *Buffer::map()
{
    void *data;
    return (xglMapMemory(mem_, 0, &data) == XGL_SUCCESS) ? data : NULL;
}

void Buffer::unmap()
{
    xglUnmapMemory(mem_);
}

Image::~Image()
{
    if (mem_ != XGL_NULL_HANDLE) {
        xglBindObjectMemory(img_, XGL_NULL_HANDLE, 0);
        xglFreeMemory(mem_);
    }
}

bool Image::init(const Device &dev, const XGL_IMAGE_CREATE_INFO &info)
{
    info_ = info;

    for (std::vector<xgl_testing::Device::Format>::const_iterator it = dev.formats().begin();
         it != dev.formats().end(); it++) {
        if (!memcmp(&it->format, &info_.format, sizeof(info_.format)) && it->tiling == info_.tiling) {
            features_ = it->features;
            break;
        }
    }

    XGL_RESULT err = xglCreateImage(dev.obj(), &info_, &img_);
    if (err != XGL_SUCCESS)
        return false;

    if (!Object::init(img_))
        return false;

    mem_ = alloc_memory(dev);
    if (mem_ == XGL_NULL_HANDLE || !bind_memory(mem_, 0))
        return false;

    return true;
}

bool Image::transparent() const
{
    return (info_.tiling == XGL_LINEAR_TILING &&
            info_.samples == 1 &&
            !(info_.usage & (XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                             XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT)));
}

XGL_SUBRESOURCE_LAYOUT Image::subresource_layout(const XGL_IMAGE_SUBRESOURCE &subres) const
{
    XGL_SUBRESOURCE_LAYOUT layout;
    XGL_SIZE size = sizeof(layout);

    XGL_RESULT err = xglGetImageSubresourceInfo(img_, &subres,
            XGL_INFO_TYPE_SUBRESOURCE_LAYOUT, &size, &layout);
    if (err != XGL_SUCCESS || size != sizeof(layout))
        memset(&layout, 0, sizeof(layout));

    return layout;
}

void *Image::map() const
{
    void *data;
    return (transparent() && xglMapMemory(mem_, 0, &data) == XGL_SUCCESS) ? data : NULL;
}

void Image::unmap() const
{
    xglUnmapMemory(mem_);
}

uint32_t ImageChecker::hash_salt_;

ImageChecker::ImageChecker(const XGL_IMAGE_CREATE_INFO &info)
    : info_(info), regions_(), pattern_(HASH)
{
    // create a region for every mip level in array slice 0
    XGL_GPU_SIZE offset = 0;
    for (XGL_UINT lv = 0; lv < info_.mipLevels; lv++) {
        XGL_MEMORY_IMAGE_COPY region = {};

        region.memOffset = offset;
        region.imageSubresource.mipLevel = lv;
        region.imageSubresource.arraySlice = 0;
        region.imageExtent = get_mip_level_extent(info_.extent, lv);

        if (info_.usage & XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT) {
            if (info_.format.channelFormat != XGL_CH_FMT_R8) {
                region.imageSubresource.aspect = XGL_IMAGE_ASPECT_DEPTH;
                regions_.push_back(region);
            }

            if (info_.format.channelFormat == XGL_CH_FMT_R16G8 ||
                info_.format.channelFormat == XGL_CH_FMT_R32G8 ||
                info_.format.channelFormat == XGL_CH_FMT_R8) {
                region.imageSubresource.aspect = XGL_IMAGE_ASPECT_STENCIL;
                regions_.push_back(region);
            }
        } else {
            region.imageSubresource.aspect = XGL_IMAGE_ASPECT_COLOR;
            regions_.push_back(region);
        }

        offset += buffer_layout(region).size;
    }

    // arraySize should be limited in our tests.  If this proves to be an
    // issue, we can store only the regions for array slice 0 and be smart.
    if (info_.arraySize > 1) {
        const XGL_GPU_SIZE slice_pitch = offset;
        const XGL_UINT slice_region_count = regions_.size();

        regions_.reserve(slice_region_count * info_.arraySize);

        for (XGL_UINT slice = 1; slice < info_.arraySize; slice++) {
            for (XGL_UINT i = 0; i < slice_region_count; i++) {
                XGL_MEMORY_IMAGE_COPY region = regions_[i];

                region.memOffset += slice_pitch * slice;
                region.imageSubresource.arraySlice = slice;
                regions_.push_back(region);
            }
        }
    }
}

ImageChecker::ImageChecker(const XGL_IMAGE_CREATE_INFO &info, const std::vector<XGL_IMAGE_SUBRESOURCE_RANGE> &ranges)
    : info_(info), regions_(), pattern_(HASH)
{
    XGL_GPU_SIZE offset = 0;
    for (std::vector<XGL_IMAGE_SUBRESOURCE_RANGE>::const_iterator it = ranges.begin();
         it != ranges.end(); it++) {
        for (XGL_UINT lv = 0; lv < it->mipLevels; lv++) {
            for (XGL_UINT slice = 0; slice < it->arraySize; slice++) {
                XGL_MEMORY_IMAGE_COPY region = {};
                region.memOffset = offset;
                region.imageSubresource = Image::subresource(*it, lv, slice);
                region.imageExtent = get_mip_level_extent(info_.extent, lv);

                regions_.push_back(region);

                offset += buffer_layout(region).size;
            }
        }
    }
}

void ImageChecker::set_solid_pattern(const std::vector<uint8_t> &solid)
{
    pattern_ = SOLID;
    pattern_solid_.clear();
    pattern_solid_.reserve(buffer_cpp());
    for (int i = 0; i < buffer_cpp(); i++)
        pattern_solid_.push_back(solid[i % solid.size()]);
}

XGL_SIZE ImageChecker::buffer_cpp() const
{
    return get_format_size(info_.format);
}

XGL_SUBRESOURCE_LAYOUT ImageChecker::buffer_layout(const XGL_MEMORY_IMAGE_COPY &region) const
{
    XGL_SUBRESOURCE_LAYOUT layout = {};
    layout.offset = region.memOffset;
    layout.rowPitch = buffer_cpp() * region.imageExtent.width;
    layout.depthPitch = layout.rowPitch * region.imageExtent.height;
    layout.size = layout.depthPitch * region.imageExtent.depth;

    return layout;
}

XGL_GPU_SIZE ImageChecker::buffer_size() const
{
    XGL_GPU_SIZE size = 0;

    for (std::vector<XGL_MEMORY_IMAGE_COPY>::const_iterator it = regions_.begin();
         it != regions_.end(); it++) {
        const XGL_SUBRESOURCE_LAYOUT layout = buffer_layout(*it);
        if (size < layout.offset + layout.size)
            size = layout.offset + layout.size;
    }

    return size;
}

bool ImageChecker::walk_region(Action action, const XGL_MEMORY_IMAGE_COPY &region,
                               const XGL_SUBRESOURCE_LAYOUT &layout, void *data) const
{
    for (XGL_INT z = 0; z < region.imageExtent.depth; z++) {
        for (XGL_INT y = 0; y < region.imageExtent.height; y++) {
            for (XGL_INT x = 0; x < region.imageExtent.width; x++) {
                uint8_t *dst = static_cast<uint8_t *>(data);
                dst += layout.offset + layout.depthPitch * z +
                    layout.rowPitch * y + buffer_cpp() * x;

                XGL_OFFSET3D offset = region.imageOffset;
                offset.x += x;
                offset.y += y;
                offset.z += z;

                const std::vector<uint8_t> &val = (pattern_ == HASH) ?
                    pattern_hash(region.imageSubresource, offset) :
                    pattern_solid_;
                assert(val.size() == buffer_cpp());

                if (action == FILL) {
                    memcpy(dst, &val[0], val.size());
                } else {
                    for (int i = 0; i < val.size(); i++) {
                        EXPECT_EQ(val[i], dst[i]) <<
                            "Offset is: (" << x << ", " << y << ", " << z << ")";
                        if (val[i] != dst[i])
                            return false;
                    }
                }
            }
        }
    }

    return true;
}

bool ImageChecker::walk(Action action, Buffer &buf) const
{
    void *data = buf.map();
    if (!data)
        return false;

    std::vector<XGL_MEMORY_IMAGE_COPY>::const_iterator it;
    for (it = regions_.begin(); it != regions_.end(); it++) {
        if (!walk_region(action, *it, buffer_layout(*it), data))
            break;
    }

    buf.unmap();

    return (it == regions_.end());
}

bool ImageChecker::walk(Action action, Image &img) const
{
    void *data = img.map();
    if (!data)
        return false;

    std::vector<XGL_MEMORY_IMAGE_COPY>::const_iterator it;
    for (it = regions_.begin(); it != regions_.end(); it++) {
        if (!walk_region(action, *it, img.subresource_layout(it->imageSubresource), data))
            break;
    }

    img.unmap();

    return (it == regions_.end());
}

std::vector<uint8_t> ImageChecker::pattern_hash(const XGL_IMAGE_SUBRESOURCE &subres, const XGL_OFFSET3D &offset) const
{
#define HASH_BYTE(val, b) static_cast<uint8_t>((static_cast<uint32_t>(val) >> (b * 8)) & 0xff)
#define HASH_BYTES(val) HASH_BYTE(val, 0), HASH_BYTE(val, 1), HASH_BYTE(val, 2), HASH_BYTE(val, 3)
    const unsigned char input[] = {
        HASH_BYTES(hash_salt_),
        HASH_BYTES(subres.mipLevel),
        HASH_BYTES(subres.arraySlice),
        HASH_BYTES(offset.x),
        HASH_BYTES(offset.y),
        HASH_BYTES(offset.z),
    };
    unsigned long hash = 5381;

    for (XGL_INT i = 0; i < ARRAY_SIZE(input); i++)
        hash = ((hash << 5) + hash) + input[i];

    const uint8_t output[4] = { HASH_BYTES(hash) };
#undef HASH_BYTES
#undef HASH_BYTE

    std::vector<uint8_t> val;
    val.reserve(buffer_cpp());
    for (int i = 0; i < buffer_cpp(); i++)
        val.push_back(output[i % 4]);

    return val;
}

XGL_SIZE get_format_size(XGL_FORMAT format)
{
    static const struct format_info {
        XGL_SIZE size;
        XGL_UINT channel_count;
    } format_table[XGL_MAX_CH_FMT + 1] = {
        [XGL_CH_FMT_UNDEFINED]      = { 0,  0 },
        [XGL_CH_FMT_R4G4]           = { 1,  2 },
        [XGL_CH_FMT_R4G4B4A4]       = { 2,  4 },
        [XGL_CH_FMT_R5G6B5]         = { 2,  3 },
        [XGL_CH_FMT_B5G6R5]         = { 2,  3 },
        [XGL_CH_FMT_R5G5B5A1]       = { 2,  4 },
        [XGL_CH_FMT_R8]             = { 1,  1 },
        [XGL_CH_FMT_R8G8]           = { 2,  2 },
        [XGL_CH_FMT_R8G8B8A8]       = { 4,  4 },
        [XGL_CH_FMT_B8G8R8A8]       = { 4,  4 },
        [XGL_CH_FMT_R10G11B11]      = { 4,  3 },
        [XGL_CH_FMT_R11G11B10]      = { 4,  3 },
        [XGL_CH_FMT_R10G10B10A2]    = { 4,  4 },
        [XGL_CH_FMT_R16]            = { 2,  1 },
        [XGL_CH_FMT_R16G16]         = { 4,  2 },
        [XGL_CH_FMT_R16G16B16A16]   = { 8,  4 },
        [XGL_CH_FMT_R32]            = { 4,  1 },
        [XGL_CH_FMT_R32G32]         = { 8,  2 },
        [XGL_CH_FMT_R32G32B32]      = { 12, 3 },
        [XGL_CH_FMT_R32G32B32A32]   = { 16, 4 },
        [XGL_CH_FMT_R16G8]          = { 3,  2 },
        [XGL_CH_FMT_R32G8]          = { 5,  2 },
        [XGL_CH_FMT_R9G9B9E5]       = { 4,  3 },
        [XGL_CH_FMT_BC1]            = { 8,  4 },
        [XGL_CH_FMT_BC2]            = { 16, 4 },
        [XGL_CH_FMT_BC3]            = { 16, 4 },
        [XGL_CH_FMT_BC4]            = { 8,  4 },
        [XGL_CH_FMT_BC5]            = { 16, 4 },
        [XGL_CH_FMT_BC6U]           = { 16, 4 },
        [XGL_CH_FMT_BC6S]           = { 16, 4 },
        [XGL_CH_FMT_BC7]            = { 16, 4 },
    };

    return format_table[format.channelFormat].size;
}

XGL_EXTENT3D get_mip_level_extent(const XGL_EXTENT3D &extent, XGL_UINT mip_level)
{
    const XGL_EXTENT3D ext = {
        (extent.width  >> mip_level) ? extent.width  >> mip_level : 1,
        (extent.height >> mip_level) ? extent.height >> mip_level : 1,
        (extent.depth  >> mip_level) ? extent.depth  >> mip_level : 1,
    };

    return ext;
}

}; // namespace xgl_testing

namespace {

#define DO(action) ASSERT_EQ(true, action);

xgl_testing::Environment *environment;

class XglCmdBlitTest : public ::testing::Test {
protected:
    XglCmdBlitTest() :
        dev_(environment->default_device()),
        queue_(dev_.queue(XGL_QUEUE_TYPE_GRAPHICS, 0)),
        cmd_()
    {
        // make sure every test uses a different pattern
        xgl_testing::ImageChecker::hash_salt_generate();
    }

    virtual void SetUp()
    {
        DO(cmd_.init(dev_));
    }

    virtual void TearDown()
    {
    }

    bool submit_and_done()
    {
        const bool ret = (dev_.submit(queue_, cmd_, XGL_NULL_HANDLE) && dev_.wait(queue_));
        cmd_.clear_memory_refs();
        return ret;
    }

    xgl_testing::Device &dev_;
    XGL_QUEUE queue_;
    xgl_testing::CmdBuffer cmd_;
};

typedef XglCmdBlitTest XglCmdFillMemoryTest;

TEST_F(XglCmdFillMemoryTest, Basic)
{
    xgl_testing::Buffer buf;

    buf.init(dev_, 20);
    cmd_.add_memory_ref(buf, 0);

    cmd_.begin();
    xglCmdFillMemory(cmd_.obj(), buf.obj(), 0, 4, 0x11111111);
    xglCmdFillMemory(cmd_.obj(), buf.obj(), 4, 16, 0x22222222);
    cmd_.end();

    submit_and_done();

    const uint32_t *data = static_cast<const uint32_t *>(buf.map());
    EXPECT_EQ(0x11111111, data[0]);
    EXPECT_EQ(0x22222222, data[1]);
    EXPECT_EQ(0x22222222, data[2]);
    EXPECT_EQ(0x22222222, data[3]);
    EXPECT_EQ(0x22222222, data[4]);
    buf.unmap();
}

TEST_F(XglCmdFillMemoryTest, Large)
{
    const XGL_GPU_SIZE size = 32 * 1024 * 1024;
    xgl_testing::Buffer buf;

    buf.init(dev_, size);
    cmd_.add_memory_ref(buf, 0);

    cmd_.begin();
    xglCmdFillMemory(cmd_.obj(), buf.obj(), 0, size / 2, 0x11111111);
    xglCmdFillMemory(cmd_.obj(), buf.obj(), size / 2, size / 2, 0x22222222);
    cmd_.end();

    submit_and_done();

    const uint32_t *data = static_cast<const uint32_t *>(buf.map());
    XGL_GPU_SIZE offset;
    for (offset = 0; offset < size / 2; offset += 4)
        EXPECT_EQ(0x11111111, data[offset / 4]) << "Offset is: " << offset;
    for (; offset < size; offset += 4)
        EXPECT_EQ(0x22222222, data[offset / 4]) << "Offset is: " << offset;
    buf.unmap();
}

TEST_F(XglCmdFillMemoryTest, Overlap)
{
    xgl_testing::Buffer buf;

    buf.init(dev_, 64);
    cmd_.add_memory_ref(buf, 0);

    cmd_.begin();
    xglCmdFillMemory(cmd_.obj(), buf.obj(), 0, 48, 0x11111111);
    xglCmdFillMemory(cmd_.obj(), buf.obj(), 32, 32, 0x22222222);
    cmd_.end();

    submit_and_done();

    const uint32_t *data = static_cast<const uint32_t *>(buf.map());
    XGL_GPU_SIZE offset;
    for (offset = 0; offset < 32; offset += 4)
        EXPECT_EQ(0x11111111, data[offset / 4]) << "Offset is: " << offset;
    for (; offset < 64; offset += 4)
        EXPECT_EQ(0x22222222, data[offset / 4]) << "Offset is: " << offset;
    buf.unmap();
}

TEST_F(XglCmdFillMemoryTest, MultiAlignments)
{
    xgl_testing::Buffer bufs[9];
    XGL_GPU_SIZE size = 4;

    cmd_.begin();
    for (int i = 0; i < ARRAY_SIZE(bufs); i++) {
        bufs[i].init(dev_, size);
        cmd_.add_memory_ref(bufs[i], 0);
        xglCmdFillMemory(cmd_.obj(), bufs[i].obj(), 0, size, 0x11111111);
        size <<= 1;
    }
    cmd_.end();

    submit_and_done();

    size = 4;
    for (int i = 0; i < ARRAY_SIZE(bufs); i++) {
        const uint32_t *data = static_cast<const uint32_t *>(bufs[i].map());
        XGL_GPU_SIZE offset;
        for (offset = 0; offset < size; offset += 4)
            EXPECT_EQ(0x11111111, data[offset / 4]) << "Buffser is: " << i << "\n" <<
                                                       "Offset is: " << offset;
        bufs[i].unmap();

        size <<= 1;
    }
}

typedef XglCmdBlitTest XglCmdCopyMemoryTest;

TEST_F(XglCmdCopyMemoryTest, Basic)
{
    xgl_testing::Buffer src, dst;

    src.init(dev_, 4);
    uint32_t *data = static_cast<uint32_t *>(src.map());
    data[0] = 0x11111111;
    src.unmap();
    cmd_.add_memory_ref(src, XGL_MEMORY_REF_READ_ONLY_BIT);

    dst.init(dev_, 4);
    cmd_.add_memory_ref(dst, 0);

    cmd_.begin();
    XGL_MEMORY_COPY region = {};
    region.copySize = 4;
    xglCmdCopyMemory(cmd_.obj(), src.obj(), dst.obj(), 1, &region);
    cmd_.end();

    submit_and_done();

    data = static_cast<uint32_t *>(dst.map());
    EXPECT_EQ(0x11111111, data[0]);
    dst.unmap();
}

TEST_F(XglCmdCopyMemoryTest, Large)
{
    const XGL_GPU_SIZE size = 32 * 1024 * 1024;
    xgl_testing::Buffer src, dst;

    src.init(dev_, size);
    uint32_t *data = static_cast<uint32_t *>(src.map());
    XGL_GPU_SIZE offset;
    for (offset = 0; offset < size; offset += 4)
        data[offset / 4] = offset;
    src.unmap();
    cmd_.add_memory_ref(src, XGL_MEMORY_REF_READ_ONLY_BIT);

    dst.init(dev_, size);
    cmd_.add_memory_ref(dst, 0);

    cmd_.begin();
    XGL_MEMORY_COPY region = {};
    region.copySize = size;
    xglCmdCopyMemory(cmd_.obj(), src.obj(), dst.obj(), 1, &region);
    cmd_.end();

    submit_and_done();

    data = static_cast<uint32_t *>(dst.map());
    for (offset = 0; offset < size; offset += 4)
        EXPECT_EQ(offset, data[offset / 4]);
    dst.unmap();
}

TEST_F(XglCmdCopyMemoryTest, MultiAlignments)
{
    const XGL_MEMORY_COPY regions[] = {
        /* well aligned */
        {  0,   0,  256 },
        {  0, 256,  128 },
        {  0, 384,   64 },
        {  0, 448,   32 },
        {  0, 480,   16 },
        {  0, 496,    8 },

        /* ill aligned */
        {  7, 510,   16 },
        { 16, 530,   13 },
        { 32, 551,   16 },
        { 45, 570,   15 },
        { 50, 590,    1 },
    };
    xgl_testing::Buffer src, dst;

    src.init(dev_, 256);
    uint8_t *data = static_cast<uint8_t *>(src.map());
    for (int i = 0; i < 256; i++)
        data[i] = i;
    src.unmap();
    cmd_.add_memory_ref(src, XGL_MEMORY_REF_READ_ONLY_BIT);

    dst.init(dev_, 1024);
    cmd_.add_memory_ref(dst, 0);

    cmd_.begin();
    xglCmdCopyMemory(cmd_.obj(), src.obj(), dst.obj(), ARRAY_SIZE(regions), regions);
    cmd_.end();

    submit_and_done();

    data = static_cast<uint8_t *>(dst.map());
    for (int i = 0; i < ARRAY_SIZE(regions); i++) {
        const XGL_MEMORY_COPY &r = regions[i];

        for (int j = 0; j < r.copySize; j++) {
            EXPECT_EQ(r.srcOffset + j, data[r.destOffset + j]) <<
                "Region is: " << i << "\n" <<
                "Offset is: " << r.destOffset + j;
        }
    }
    dst.unmap();
}

TEST_F(XglCmdCopyMemoryTest, RAWHazard)
{
    xgl_testing::Buffer bufs[3];

    for (int i = 0; i < ARRAY_SIZE(bufs); i++) {
        bufs[i].init(dev_, 4);
        cmd_.add_memory_ref(bufs[i], 0);

        uint32_t *data = static_cast<uint32_t *>(bufs[i].map());
        data[0] = 0x22222222 * (i + 1);
        bufs[i].unmap();
    }

    cmd_.begin();

    xglCmdFillMemory(cmd_.obj(), bufs[0].obj(), 0, 4, 0x11111111);
    // is this necessary?
    XGL_MEMORY_STATE_TRANSITION transition = bufs[0].prepare(
            XGL_MEMORY_STATE_DATA_TRANSFER, XGL_MEMORY_STATE_DATA_TRANSFER, 0, 4);
    xglCmdPrepareMemoryRegions(cmd_.obj(), 1, &transition);

    XGL_MEMORY_COPY region = {};
    region.copySize = 4;
    xglCmdCopyMemory(cmd_.obj(), bufs[0].obj(), bufs[1].obj(), 1, &region);
    // is this necessary?
    transition = bufs[1].prepare(
            XGL_MEMORY_STATE_DATA_TRANSFER, XGL_MEMORY_STATE_DATA_TRANSFER, 0, 4);
    xglCmdPrepareMemoryRegions(cmd_.obj(), 1, &transition);

    xglCmdCopyMemory(cmd_.obj(), bufs[1].obj(), bufs[2].obj(), 1, &region);
    cmd_.end();

    submit_and_done();

    const uint32_t *data = static_cast<const uint32_t *>(bufs[2].map());
    EXPECT_EQ(0x11111111, data[0]);
    bufs[2].unmap();
}

class XglCmdBlitImageTest : public XglCmdBlitTest {
protected:
    void init_test_formats(XGL_FLAGS features)
    {
        first_linear_format_.channelFormat = XGL_CH_FMT_UNDEFINED;
        first_linear_format_.numericFormat = XGL_NUM_FMT_UNDEFINED;
        first_optimal_format_.channelFormat = XGL_CH_FMT_UNDEFINED;
        first_optimal_format_.numericFormat = XGL_NUM_FMT_UNDEFINED;

        for (std::vector<xgl_testing::Device::Format>::const_iterator it = dev_.formats().begin();
             it != dev_.formats().end(); it++) {
            if (it->features & features) {
                test_formats_.push_back(*it);

                if (it->tiling == XGL_LINEAR_TILING &&
                    first_linear_format_.channelFormat == XGL_CH_FMT_UNDEFINED)
                    first_linear_format_ = it->format;
                if (it->tiling == XGL_OPTIMAL_TILING &&
                    first_optimal_format_.channelFormat == XGL_CH_FMT_UNDEFINED)
                    first_optimal_format_ = it->format;
            }
        }
    }

    void init_test_formats()
    {
        init_test_formats(static_cast<XGL_FLAGS>(-1));
    }

    void fill_src(xgl_testing::Image &img, const xgl_testing::ImageChecker &checker)
    {
        if (img.transparent()) {
            checker.fill(img);
            return;
        }

        ASSERT_EQ(true, img.copyable());

        xgl_testing::Buffer in_buf;
        in_buf.init(dev_, checker.buffer_size());
        checker.fill(in_buf);

        cmd_.add_memory_ref(in_buf, XGL_MEMORY_REF_READ_ONLY_BIT);
        cmd_.add_memory_ref(img, 0);

        // copy in and tile
        cmd_.begin();
        xglCmdCopyMemoryToImage(cmd_.obj(), in_buf.obj(), img.obj(),
                checker.regions().size(), &checker.regions()[0]);
        cmd_.end();

        submit_and_done();
    }

    void check_dst(xgl_testing::Image &img, const xgl_testing::ImageChecker &checker)
    {
        if (img.transparent()) {
            DO(checker.check(img));
            return;
        }

        ASSERT_EQ(true, img.copyable());

        xgl_testing::Buffer out_buf;
        out_buf.init(dev_, checker.buffer_size());

        cmd_.add_memory_ref(img, XGL_MEMORY_REF_READ_ONLY_BIT);
        cmd_.add_memory_ref(out_buf, 0);

        // copy out and linearize
        cmd_.begin();
        xglCmdCopyImageToMemory(cmd_.obj(), img.obj(), out_buf.obj(),
                checker.regions().size(), &checker.regions()[0]);
        cmd_.end();

        submit_and_done();

        DO(checker.check(out_buf));
    }

    std::vector<xgl_testing::Device::Format> test_formats_;
    XGL_FORMAT first_linear_format_;
    XGL_FORMAT first_optimal_format_;
};

class XglCmdCopyMemoryToImageTest : public XglCmdBlitImageTest {
protected:
    virtual void SetUp()
    {
        XglCmdBlitTest::SetUp();
        init_test_formats(XGL_FORMAT_IMAGE_COPY_BIT);
        ASSERT_NE(true, test_formats_.empty());
    }

    void test_copy_memory_to_image(const XGL_IMAGE_CREATE_INFO &img_info, const xgl_testing::ImageChecker &checker)
    {
        xgl_testing::Buffer buf;
        xgl_testing::Image img;

        buf.init(dev_, checker.buffer_size());
        checker.fill(buf);
        cmd_.add_memory_ref(buf, XGL_MEMORY_REF_READ_ONLY_BIT);

        img.init(dev_, img_info);
        cmd_.add_memory_ref(img, 0);

        cmd_.begin();
        xglCmdCopyMemoryToImage(cmd_.obj(), buf.obj(), img.obj(),
                checker.regions().size(), &checker.regions()[0]);
        cmd_.end();

        submit_and_done();

        check_dst(img, checker);
    }

    void test_copy_memory_to_image(const XGL_IMAGE_CREATE_INFO &img_info, const std::vector<XGL_MEMORY_IMAGE_COPY> &regions)
    {
        xgl_testing::ImageChecker checker(img_info, regions);
        test_copy_memory_to_image(img_info, checker);
    }

    void test_copy_memory_to_image(const XGL_IMAGE_CREATE_INFO &img_info)
    {
        xgl_testing::ImageChecker checker(img_info);
        test_copy_memory_to_image(img_info, checker);
    }
};

TEST_F(XglCmdCopyMemoryToImageTest, Basic)
{
    for (std::vector<xgl_testing::Device::Format>::const_iterator it = test_formats_.begin();
         it != test_formats_.end(); it++) {
        XGL_IMAGE_CREATE_INFO img_info = xgl_testing::Image::create_info();
        img_info.imageType = XGL_IMAGE_2D;
        img_info.format = it->format;
        img_info.extent.width = 64;
        img_info.extent.height = 64;
        img_info.tiling = it->tiling;

        test_copy_memory_to_image(img_info);
    }
}

class XglCmdCopyImageToMemoryTest : public XglCmdBlitImageTest {
protected:
    virtual void SetUp()
    {
        XglCmdBlitTest::SetUp();
        init_test_formats(XGL_FORMAT_IMAGE_COPY_BIT);
        ASSERT_NE(true, test_formats_.empty());
    }

    void test_copy_image_to_memory(const XGL_IMAGE_CREATE_INFO &img_info, const xgl_testing::ImageChecker &checker)
    {
        xgl_testing::Image img;
        xgl_testing::Buffer buf;

        img.init(dev_, img_info);
        fill_src(img, checker);
        cmd_.add_memory_ref(img, XGL_MEMORY_REF_READ_ONLY_BIT);

        buf.init(dev_, checker.buffer_size());
        cmd_.add_memory_ref(buf, 0);

        cmd_.begin();
        xglCmdCopyImageToMemory(cmd_.obj(), img.obj(), buf.obj(),
                checker.regions().size(), &checker.regions()[0]);
        cmd_.end();

        submit_and_done();

        checker.check(buf);
    }

    void test_copy_image_to_memory(const XGL_IMAGE_CREATE_INFO &img_info, const std::vector<XGL_MEMORY_IMAGE_COPY> &regions)
    {
        xgl_testing::ImageChecker checker(img_info, regions);
        test_copy_image_to_memory(img_info, checker);
    }

    void test_copy_image_to_memory(const XGL_IMAGE_CREATE_INFO &img_info)
    {
        xgl_testing::ImageChecker checker(img_info);
        test_copy_image_to_memory(img_info, checker);
    }
};

TEST_F(XglCmdCopyImageToMemoryTest, Basic)
{
    for (std::vector<xgl_testing::Device::Format>::const_iterator it = test_formats_.begin();
         it != test_formats_.end(); it++) {
        XGL_IMAGE_CREATE_INFO img_info = xgl_testing::Image::create_info();
        img_info.imageType = XGL_IMAGE_2D;
        img_info.format = it->format;
        img_info.extent.width = 64;
        img_info.extent.height = 64;
        img_info.tiling = it->tiling;

        test_copy_image_to_memory(img_info);
    }
}

class XglCmdCopyImageTest : public XglCmdBlitImageTest {
protected:
    virtual void SetUp()
    {
        XglCmdBlitTest::SetUp();
        init_test_formats(XGL_FORMAT_IMAGE_COPY_BIT);
        ASSERT_NE(true, test_formats_.empty());
    }

    void test_copy_image(const XGL_IMAGE_CREATE_INFO &src_info, const XGL_IMAGE_CREATE_INFO &dst_info,
                         const std::vector<XGL_IMAGE_COPY> &copies)
    {
        // convert XGL_IMAGE_COPY to two sets of XGL_MEMORY_IMAGE_COPY
        std::vector<XGL_MEMORY_IMAGE_COPY> src_regions, dst_regions;
        XGL_GPU_SIZE src_offset = 0, dst_offset = 0;
        for (std::vector<XGL_IMAGE_COPY>::const_iterator it = copies.begin(); it != copies.end(); it++) {
            XGL_MEMORY_IMAGE_COPY src_region = {}, dst_region = {};

            src_region.memOffset = src_offset;
            src_region.imageSubresource = it->srcSubresource;
            src_region.imageOffset = it->srcOffset;
            src_region.imageExtent = it->extent;
            src_regions.push_back(src_region);

            dst_region.memOffset = src_offset;
            dst_region.imageSubresource = it->destSubresource;
            dst_region.imageOffset = it->destOffset;
            dst_region.imageExtent = it->extent;
            dst_regions.push_back(dst_region);

            const XGL_GPU_SIZE size = it->extent.width * it->extent.height * it->extent.depth;
            src_offset += xgl_testing::get_format_size(src_info.format) * size;
            dst_offset += xgl_testing::get_format_size(dst_info.format) * size;
        }

        xgl_testing::ImageChecker src_checker(src_info, src_regions);
        xgl_testing::ImageChecker dst_checker(dst_info, dst_regions);

        xgl_testing::Image src;
        src.init(dev_, src_info);
        fill_src(src, src_checker);
        cmd_.add_memory_ref(src, XGL_MEMORY_REF_READ_ONLY_BIT);

        xgl_testing::Image dst;
        dst.init(dev_, dst_info);
        cmd_.add_memory_ref(dst, 0);

        cmd_.begin();
        xglCmdCopyImage(cmd_.obj(), src.obj(), dst.obj(), copies.size(), &copies[0]);
        cmd_.end();

        submit_and_done();

        check_dst(dst, dst_checker);
    }
};

TEST_F(XglCmdCopyImageTest, Basic)
{
    for (std::vector<xgl_testing::Device::Format>::const_iterator it = test_formats_.begin();
         it != test_formats_.end(); it++) {
        XGL_IMAGE_CREATE_INFO img_info = xgl_testing::Image::create_info();
        img_info.imageType = XGL_IMAGE_2D;
        img_info.format = it->format;
        img_info.extent.width = 64;
        img_info.extent.height = 64;
        img_info.tiling = it->tiling;

        XGL_IMAGE_COPY copy = {};
        copy.srcSubresource = xgl_testing::Image::subresource(XGL_IMAGE_ASPECT_COLOR, 0, 0);
        copy.destSubresource = copy.srcSubresource;
        copy.extent = img_info.extent;

        test_copy_image(img_info, img_info, std::vector<XGL_IMAGE_COPY>(&copy, &copy + 1));
    }
}

class XglCmdCloneImageDataTest : public XglCmdBlitImageTest {
protected:
    virtual void SetUp()
    {
        XglCmdBlitTest::SetUp();
        init_test_formats();
        ASSERT_NE(true, test_formats_.empty());
    }

    void test_clone_image_data(const XGL_IMAGE_CREATE_INFO &img_info)
    {
        xgl_testing::ImageChecker checker(img_info);
        xgl_testing::Image src, dst;

        src.init(dev_, img_info);
        if (src.transparent() || src.copyable())
            fill_src(src, checker);
        cmd_.add_memory_ref(src, XGL_MEMORY_REF_READ_ONLY_BIT);

        dst.init(dev_, img_info);
        cmd_.add_memory_ref(dst, 0);

        const XGL_IMAGE_STATE state =
            (img_info.usage & XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) ?
            XGL_IMAGE_STATE_UNINITIALIZED_TARGET : XGL_IMAGE_STATE_DATA_TRANSFER;

        cmd_.begin();
        xglCmdCloneImageData(cmd_.obj(), src.obj(), state, dst.obj(), state);
        cmd_.end();

        submit_and_done();

        // cannot verify
        if (!dst.transparent() && !dst.copyable())
            return;

        check_dst(dst, checker);
    }
};

TEST_F(XglCmdCloneImageDataTest, Basic)
{
    for (std::vector<xgl_testing::Device::Format>::const_iterator it = test_formats_.begin();
         it != test_formats_.end(); it++) {
        // not sure what to do here
        if (it->format.channelFormat == XGL_CH_FMT_UNDEFINED ||
            (it->format.channelFormat >= XGL_CH_FMT_BC1 &&
             it->format.channelFormat <= XGL_CH_FMT_BC7) ||
            it->format.numericFormat == XGL_NUM_FMT_DS)
            continue;

        XGL_IMAGE_CREATE_INFO img_info = xgl_testing::Image::create_info();
        img_info.imageType = XGL_IMAGE_2D;
        img_info.format = it->format;
        img_info.extent.width = 64;
        img_info.extent.height = 64;
        img_info.tiling = it->tiling;
        img_info.flags = XGL_IMAGE_CREATE_CLONEABLE_BIT;

        const XGL_IMAGE_SUBRESOURCE_RANGE range =
            xgl_testing::Image::subresource_range(XGL_IMAGE_ASPECT_COLOR, img_info);
        std::vector<XGL_IMAGE_SUBRESOURCE_RANGE> ranges(&range, &range + 1);

        test_clone_image_data(img_info);
    }
}

class XglCmdClearColorImageTest : public XglCmdBlitImageTest {
protected:
    XglCmdClearColorImageTest() : test_raw_(false) {}
    XglCmdClearColorImageTest(bool test_raw) : test_raw_(test_raw) {}

    virtual void SetUp()
    {
        XglCmdBlitTest::SetUp();

        if (test_raw_)
            init_test_formats();
        else
            init_test_formats(XGL_FORMAT_CONVERSION_BIT);

        ASSERT_NE(true, test_formats_.empty());
    }

    union Color {
        XGL_FLOAT color[4];
        XGL_UINT32 raw[4];
    };

    bool test_raw_;

    std::vector<uint8_t> color_to_raw(XGL_FORMAT format, const XGL_FLOAT color[4])
    {
        std::vector<uint8_t> raw;

        // TODO support all formats
        if (format.numericFormat == XGL_NUM_FMT_UNORM) {
            switch (format.channelFormat) {
            case XGL_CH_FMT_R8G8B8A8:
                raw.push_back(color[0] * 255.0f);
                raw.push_back(color[1] * 255.0f);
                raw.push_back(color[2] * 255.0f);
                raw.push_back(color[3] * 255.0f);
                break;
            case XGL_CH_FMT_B8G8R8A8:
                raw.push_back(color[2] * 255.0f);
                raw.push_back(color[1] * 255.0f);
                raw.push_back(color[0] * 255.0f);
                raw.push_back(color[3] * 255.0f);
                break;
            default:
                break;
            }
        }

        return raw;
    }

    std::vector<uint8_t> color_to_raw(XGL_FORMAT format, const XGL_UINT32 color[4])
    {
        std::vector<uint8_t> raw;

        // TODO support all formats
        if (format.numericFormat == XGL_NUM_FMT_UNORM) {
            switch (format.channelFormat) {
            case XGL_CH_FMT_R8G8B8A8:
                raw.push_back(static_cast<uint8_t>(color[0]));
                raw.push_back(static_cast<uint8_t>(color[1]));
                raw.push_back(static_cast<uint8_t>(color[2]));
                raw.push_back(static_cast<uint8_t>(color[3]));
                break;
            case XGL_CH_FMT_B8G8R8A8:
                raw.push_back(static_cast<uint8_t>(color[2]));
                raw.push_back(static_cast<uint8_t>(color[1]));
                raw.push_back(static_cast<uint8_t>(color[0]));
                raw.push_back(static_cast<uint8_t>(color[3]));
                break;
            default:
                break;
            }
        }

        return raw;
    }

    std::vector<uint8_t> color_to_raw(XGL_FORMAT format, const Color &color)
    {
        if (test_raw_)
            return color_to_raw(format, color.raw);
        else
            return color_to_raw(format, color.color);
    }

    void test_clear_color_image(const XGL_IMAGE_CREATE_INFO &img_info,
                                const Color &color,
                                const std::vector<XGL_IMAGE_SUBRESOURCE_RANGE> &ranges)
    {
        xgl_testing::Image img;
        img.init(dev_, img_info);
        cmd_.add_memory_ref(img, 0);

        std::vector<XGL_IMAGE_STATE_TRANSITION> to_clear;
        std::vector<XGL_IMAGE_STATE_TRANSITION> to_xfer;

        const XGL_IMAGE_STATE initial_state =
            (img_info.usage & XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) ?
            XGL_IMAGE_STATE_UNINITIALIZED_TARGET : XGL_IMAGE_STATE_DATA_TRANSFER;
        for (std::vector<XGL_IMAGE_SUBRESOURCE_RANGE>::const_iterator it = ranges.begin();
             it != ranges.end(); it++) {
            to_clear.push_back(img.prepare(initial_state, XGL_IMAGE_STATE_CLEAR, *it));
            to_xfer.push_back(img.prepare(XGL_IMAGE_STATE_CLEAR, XGL_IMAGE_STATE_DATA_TRANSFER, *it));
        }

        cmd_.begin();

        xglCmdPrepareImages(cmd_.obj(), to_clear.size(), &to_clear[0]);
        if (test_raw_)
            xglCmdClearColorImageRaw(cmd_.obj(), img.obj(), color.raw, ranges.size(), &ranges[0]);
        else
            xglCmdClearColorImage(cmd_.obj(), img.obj(), color.color, ranges.size(), &ranges[0]);
        xglCmdPrepareImages(cmd_.obj(), to_xfer.size(), &to_xfer[0]);

        cmd_.end();

        submit_and_done();

        // cannot verify
        if (!img.transparent() && !img.copyable())
            return;

        xgl_testing::ImageChecker checker(img_info, ranges);

        const std::vector<uint8_t> solid_pattern = color_to_raw(img_info.format, color);
        if (solid_pattern.empty())
            return;

        checker.set_solid_pattern(solid_pattern);
        check_dst(img, checker);
    }

    void test_clear_color_image(const XGL_IMAGE_CREATE_INFO &img_info,
                                const XGL_FLOAT color[4],
                                const std::vector<XGL_IMAGE_SUBRESOURCE_RANGE> &ranges)
    {
        Color c;
        memcpy(c.color, color, sizeof(c.color));
        test_clear_color_image(img_info, c, ranges);
    }
};

TEST_F(XglCmdClearColorImageTest, Basic)
{
    for (std::vector<xgl_testing::Device::Format>::const_iterator it = test_formats_.begin();
         it != test_formats_.end(); it++) {
        const XGL_FLOAT color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };

        XGL_IMAGE_CREATE_INFO img_info = xgl_testing::Image::create_info();
        img_info.imageType = XGL_IMAGE_2D;
        img_info.format = it->format;
        img_info.extent.width = 64;
        img_info.extent.height = 64;
        img_info.tiling = it->tiling;

        const XGL_IMAGE_SUBRESOURCE_RANGE range =
            xgl_testing::Image::subresource_range(XGL_IMAGE_ASPECT_COLOR, img_info);
        std::vector<XGL_IMAGE_SUBRESOURCE_RANGE> ranges(&range, &range + 1);

        test_clear_color_image(img_info, color, ranges);
    }
}

class XglCmdClearColorImageRawTest : public XglCmdClearColorImageTest {
protected:
    XglCmdClearColorImageRawTest() : XglCmdClearColorImageTest(true) {}

    void test_clear_color_image_raw(const XGL_IMAGE_CREATE_INFO &img_info,
                                    const XGL_UINT32 color[4],
                                    const std::vector<XGL_IMAGE_SUBRESOURCE_RANGE> &ranges)
    {
        Color c;
        memcpy(c.raw, color, sizeof(c.raw));
        test_clear_color_image(img_info, c, ranges);
    }
};

TEST_F(XglCmdClearColorImageRawTest, Basic)
{
    for (std::vector<xgl_testing::Device::Format>::const_iterator it = test_formats_.begin();
         it != test_formats_.end(); it++) {
        const XGL_UINT32 color[4] = { 0x11111111, 0x22222222, 0x33333333, 0x44444444 };

        // not sure what to do here
        if (it->format.channelFormat == XGL_CH_FMT_UNDEFINED ||
            it->format.channelFormat == XGL_CH_FMT_R32G32B32 ||
            it->format.numericFormat == XGL_NUM_FMT_DS)
            continue;

        XGL_IMAGE_CREATE_INFO img_info = xgl_testing::Image::create_info();
        img_info.imageType = XGL_IMAGE_2D;
        img_info.format = it->format;
        img_info.extent.width = 64;
        img_info.extent.height = 64;
        img_info.tiling = it->tiling;

        const XGL_IMAGE_SUBRESOURCE_RANGE range =
            xgl_testing::Image::subresource_range(XGL_IMAGE_ASPECT_COLOR, img_info);
        std::vector<XGL_IMAGE_SUBRESOURCE_RANGE> ranges(&range, &range + 1);

        test_clear_color_image_raw(img_info, color, ranges);
    }
}

class XglCmdClearDepthStencilTest : public XglCmdBlitImageTest {
protected:
    virtual void SetUp()
    {
        XglCmdBlitTest::SetUp();
        init_test_formats(XGL_FORMAT_DEPTH_ATTACHMENT_BIT |
                          XGL_FORMAT_STENCIL_ATTACHMENT_BIT);
        ASSERT_NE(true, test_formats_.empty());
    }

    std::vector<uint8_t> ds_to_raw(XGL_FORMAT format, XGL_FLOAT depth, XGL_UINT32 stencil)
    {
        std::vector<uint8_t> raw;

        // depth
        switch (format.channelFormat) {
        case XGL_CH_FMT_R16:
        case XGL_CH_FMT_R16G8:
            {
                const uint16_t unorm = depth * 65535.0f;
                raw.push_back(unorm & 0xff);
                raw.push_back(unorm >> 8);
            }
            break;
        case XGL_CH_FMT_R32:
        case XGL_CH_FMT_R32G8:
            {
                const union {
                    XGL_FLOAT depth;
                    uint32_t u32;
                } u = { depth };

                raw.push_back((u.u32      ) & 0xff);
                raw.push_back((u.u32 >>  8) & 0xff);
                raw.push_back((u.u32 >> 16) & 0xff);
                raw.push_back((u.u32 >> 24) & 0xff);
            }
            break;
        default:
            break;
        }

        // stencil
        switch (format.channelFormat) {
        case XGL_CH_FMT_R8:
            raw.push_back(stencil);
            break;
        case XGL_CH_FMT_R16G8:
            raw.push_back(stencil);
            raw.push_back(0);
            break;
        case XGL_CH_FMT_R32G8:
            raw.push_back(stencil);
            raw.push_back(0);
            raw.push_back(0);
            raw.push_back(0);
            break;
        default:
            break;
        }

        return raw;
    }

    void test_clear_depth_stencil(const XGL_IMAGE_CREATE_INFO &img_info,
                                  XGL_FLOAT depth, XGL_UINT32 stencil,
                                  const std::vector<XGL_IMAGE_SUBRESOURCE_RANGE> &ranges)
    {
        xgl_testing::Image img;
        img.init(dev_, img_info);
        cmd_.add_memory_ref(img, 0);

        std::vector<XGL_IMAGE_STATE_TRANSITION> to_clear;
        std::vector<XGL_IMAGE_STATE_TRANSITION> to_xfer;

        for (std::vector<XGL_IMAGE_SUBRESOURCE_RANGE>::const_iterator it = ranges.begin();
             it != ranges.end(); it++) {
            to_clear.push_back(img.prepare(XGL_IMAGE_STATE_UNINITIALIZED_TARGET, XGL_IMAGE_STATE_CLEAR, *it));
            to_xfer.push_back(img.prepare(XGL_IMAGE_STATE_CLEAR, XGL_IMAGE_STATE_DATA_TRANSFER, *it));
        }

        cmd_.begin();
        xglCmdPrepareImages(cmd_.obj(), to_clear.size(), &to_clear[0]);
        xglCmdClearDepthStencil(cmd_.obj(), img.obj(), depth, stencil, ranges.size(), &ranges[0]);
        xglCmdPrepareImages(cmd_.obj(), to_xfer.size(), &to_xfer[0]);
        cmd_.end();

        submit_and_done();

        // cannot verify
        if (!img.transparent() && !img.copyable())
            return;

        xgl_testing::ImageChecker checker(img_info, ranges);

        checker.set_solid_pattern(ds_to_raw(img_info.format, depth, stencil));
        check_dst(img, checker);
    }
};

TEST_F(XglCmdClearDepthStencilTest, Basic)
{
    for (std::vector<xgl_testing::Device::Format>::const_iterator it = test_formats_.begin();
         it != test_formats_.end(); it++) {
        // known driver issues
        if (it->format.channelFormat == XGL_CH_FMT_R8)
            continue;

        XGL_IMAGE_CREATE_INFO img_info = xgl_testing::Image::create_info();
        img_info.imageType = XGL_IMAGE_2D;
        img_info.format = it->format;
        img_info.extent.width = 64;
        img_info.extent.height = 64;
        img_info.tiling = it->tiling;
        img_info.usage = XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT;

        const XGL_IMAGE_SUBRESOURCE_RANGE range =
            xgl_testing::Image::subresource_range(XGL_IMAGE_ASPECT_DEPTH, img_info);
        std::vector<XGL_IMAGE_SUBRESOURCE_RANGE> ranges(&range, &range + 1);

        test_clear_depth_stencil(img_info, 0.25f, 63, ranges);
    }
}

}; // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    environment = new xgl_testing::Environment();

    if (!environment->parse_args(argc, argv))
        return -1;

    ::testing::AddGlobalTestEnvironment(environment);

    return RUN_ALL_TESTS();
}
