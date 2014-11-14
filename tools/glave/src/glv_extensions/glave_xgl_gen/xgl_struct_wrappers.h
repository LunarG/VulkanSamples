//This is the copyright
//#includes, #defines, globals and such...
#include <xgl.h>
#include <xgl_string_helper.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//class declaration
class xgl_raster_state_create_info_struct_wrapper
{
public:
    xgl_raster_state_create_info_struct_wrapper();
    xgl_raster_state_create_info_struct_wrapper(XGL_RASTER_STATE_CREATE_INFO* pInStruct);
    xgl_raster_state_create_info_struct_wrapper(const XGL_RASTER_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_raster_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_FILL_MODE get_fillMode() { return m_struct.fillMode; }
    void set_fillMode(XGL_FILL_MODE inValue) { m_struct.fillMode = inValue; }
    XGL_CULL_MODE get_cullMode() { return m_struct.cullMode; }
    void set_cullMode(XGL_CULL_MODE inValue) { m_struct.cullMode = inValue; }
    XGL_FACE_ORIENTATION get_frontFace() { return m_struct.frontFace; }
    void set_frontFace(XGL_FACE_ORIENTATION inValue) { m_struct.frontFace = inValue; }
    XGL_INT get_depthBias() { return m_struct.depthBias; }
    void set_depthBias(XGL_INT inValue) { m_struct.depthBias = inValue; }
    XGL_FLOAT get_depthBiasClamp() { return m_struct.depthBiasClamp; }
    void set_depthBiasClamp(XGL_FLOAT inValue) { m_struct.depthBiasClamp = inValue; }
    XGL_FLOAT get_slopeScaledDepthBias() { return m_struct.slopeScaledDepthBias; }
    void set_slopeScaledDepthBias(XGL_FLOAT inValue) { m_struct.slopeScaledDepthBias = inValue; }


private:
    XGL_RASTER_STATE_CREATE_INFO m_struct;
    const XGL_RASTER_STATE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_gpu_compatibility_info_struct_wrapper
{
public:
    xgl_gpu_compatibility_info_struct_wrapper();
    xgl_gpu_compatibility_info_struct_wrapper(XGL_GPU_COMPATIBILITY_INFO* pInStruct);
    xgl_gpu_compatibility_info_struct_wrapper(const XGL_GPU_COMPATIBILITY_INFO* pInStruct);

    virtual ~xgl_gpu_compatibility_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_FLAGS get_compatibilityFlags() { return m_struct.compatibilityFlags; }
    void set_compatibilityFlags(XGL_FLAGS inValue) { m_struct.compatibilityFlags = inValue; }


private:
    XGL_GPU_COMPATIBILITY_INFO m_struct;
    const XGL_GPU_COMPATIBILITY_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_image_view_create_info_struct_wrapper
{
public:
    xgl_image_view_create_info_struct_wrapper();
    xgl_image_view_create_info_struct_wrapper(XGL_IMAGE_VIEW_CREATE_INFO* pInStruct);
    xgl_image_view_create_info_struct_wrapper(const XGL_IMAGE_VIEW_CREATE_INFO* pInStruct);

    virtual ~xgl_image_view_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_IMAGE get_image() { return m_struct.image; }
    void set_image(XGL_IMAGE inValue) { m_struct.image = inValue; }
    XGL_IMAGE_VIEW_TYPE get_viewType() { return m_struct.viewType; }
    void set_viewType(XGL_IMAGE_VIEW_TYPE inValue) { m_struct.viewType = inValue; }
    XGL_FORMAT get_format() { return m_struct.format; }
    void set_format(XGL_FORMAT inValue) { m_struct.format = inValue; }
    XGL_CHANNEL_MAPPING get_channels() { return m_struct.channels; }
    void set_channels(XGL_CHANNEL_MAPPING inValue) { m_struct.channels = inValue; }
    XGL_IMAGE_SUBRESOURCE_RANGE get_subresourceRange() { return m_struct.subresourceRange; }
    void set_subresourceRange(XGL_IMAGE_SUBRESOURCE_RANGE inValue) { m_struct.subresourceRange = inValue; }
    XGL_FLOAT get_minLod() { return m_struct.minLod; }
    void set_minLod(XGL_FLOAT inValue) { m_struct.minLod = inValue; }


private:
    XGL_IMAGE_VIEW_CREATE_INFO m_struct;
    const XGL_IMAGE_VIEW_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_memory_open_info_struct_wrapper
{
public:
    xgl_memory_open_info_struct_wrapper();
    xgl_memory_open_info_struct_wrapper(XGL_MEMORY_OPEN_INFO* pInStruct);
    xgl_memory_open_info_struct_wrapper(const XGL_MEMORY_OPEN_INFO* pInStruct);

    virtual ~xgl_memory_open_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    XGL_VOID* get_pNext() { return m_struct.pNext; }
    void set_pNext(XGL_VOID* inValue) { m_struct.pNext = inValue; }
    XGL_GPU_MEMORY get_sharedMem() { return m_struct.sharedMem; }
    void set_sharedMem(XGL_GPU_MEMORY inValue) { m_struct.sharedMem = inValue; }


private:
    XGL_MEMORY_OPEN_INFO m_struct;
    const XGL_MEMORY_OPEN_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_memory_heap_properties_struct_wrapper
{
public:
    xgl_memory_heap_properties_struct_wrapper();
    xgl_memory_heap_properties_struct_wrapper(XGL_MEMORY_HEAP_PROPERTIES* pInStruct);
    xgl_memory_heap_properties_struct_wrapper(const XGL_MEMORY_HEAP_PROPERTIES* pInStruct);

    virtual ~xgl_memory_heap_properties_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_SIZE get_structSize() { return m_struct.structSize; }
    void set_structSize(XGL_SIZE inValue) { m_struct.structSize = inValue; }
    XGL_HEAP_MEMORY_TYPE get_heapMemoryType() { return m_struct.heapMemoryType; }
    void set_heapMemoryType(XGL_HEAP_MEMORY_TYPE inValue) { m_struct.heapMemoryType = inValue; }
    XGL_GPU_SIZE get_heapSize() { return m_struct.heapSize; }
    void set_heapSize(XGL_GPU_SIZE inValue) { m_struct.heapSize = inValue; }
    XGL_GPU_SIZE get_pageSize() { return m_struct.pageSize; }
    void set_pageSize(XGL_GPU_SIZE inValue) { m_struct.pageSize = inValue; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }
    XGL_FLOAT get_gpuReadPerfRating() { return m_struct.gpuReadPerfRating; }
    void set_gpuReadPerfRating(XGL_FLOAT inValue) { m_struct.gpuReadPerfRating = inValue; }
    XGL_FLOAT get_gpuWritePerfRating() { return m_struct.gpuWritePerfRating; }
    void set_gpuWritePerfRating(XGL_FLOAT inValue) { m_struct.gpuWritePerfRating = inValue; }
    XGL_FLOAT get_cpuReadPerfRating() { return m_struct.cpuReadPerfRating; }
    void set_cpuReadPerfRating(XGL_FLOAT inValue) { m_struct.cpuReadPerfRating = inValue; }
    XGL_FLOAT get_cpuWritePerfRating() { return m_struct.cpuWritePerfRating; }
    void set_cpuWritePerfRating(XGL_FLOAT inValue) { m_struct.cpuWritePerfRating = inValue; }


private:
    XGL_MEMORY_HEAP_PROPERTIES m_struct;
    const XGL_MEMORY_HEAP_PROPERTIES* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_image_subresource_struct_wrapper
{
public:
    xgl_image_subresource_struct_wrapper();
    xgl_image_subresource_struct_wrapper(XGL_IMAGE_SUBRESOURCE* pInStruct);
    xgl_image_subresource_struct_wrapper(const XGL_IMAGE_SUBRESOURCE* pInStruct);

    virtual ~xgl_image_subresource_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_IMAGE_ASPECT get_aspect() { return m_struct.aspect; }
    void set_aspect(XGL_IMAGE_ASPECT inValue) { m_struct.aspect = inValue; }
    XGL_UINT get_mipLevel() { return m_struct.mipLevel; }
    void set_mipLevel(XGL_UINT inValue) { m_struct.mipLevel = inValue; }
    XGL_UINT get_arraySlice() { return m_struct.arraySlice; }
    void set_arraySlice(XGL_UINT inValue) { m_struct.arraySlice = inValue; }


private:
    XGL_IMAGE_SUBRESOURCE m_struct;
    const XGL_IMAGE_SUBRESOURCE* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_physical_gpu_performance_struct_wrapper
{
public:
    xgl_physical_gpu_performance_struct_wrapper();
    xgl_physical_gpu_performance_struct_wrapper(XGL_PHYSICAL_GPU_PERFORMANCE* pInStruct);
    xgl_physical_gpu_performance_struct_wrapper(const XGL_PHYSICAL_GPU_PERFORMANCE* pInStruct);

    virtual ~xgl_physical_gpu_performance_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_FLOAT get_maxGpuClock() { return m_struct.maxGpuClock; }
    void set_maxGpuClock(XGL_FLOAT inValue) { m_struct.maxGpuClock = inValue; }
    XGL_FLOAT get_aluPerClock() { return m_struct.aluPerClock; }
    void set_aluPerClock(XGL_FLOAT inValue) { m_struct.aluPerClock = inValue; }
    XGL_FLOAT get_texPerClock() { return m_struct.texPerClock; }
    void set_texPerClock(XGL_FLOAT inValue) { m_struct.texPerClock = inValue; }
    XGL_FLOAT get_primsPerClock() { return m_struct.primsPerClock; }
    void set_primsPerClock(XGL_FLOAT inValue) { m_struct.primsPerClock = inValue; }
    XGL_FLOAT get_pixelsPerClock() { return m_struct.pixelsPerClock; }
    void set_pixelsPerClock(XGL_FLOAT inValue) { m_struct.pixelsPerClock = inValue; }


private:
    XGL_PHYSICAL_GPU_PERFORMANCE m_struct;
    const XGL_PHYSICAL_GPU_PERFORMANCE* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_physical_gpu_memory_properties_struct_wrapper
{
public:
    xgl_physical_gpu_memory_properties_struct_wrapper();
    xgl_physical_gpu_memory_properties_struct_wrapper(XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* pInStruct);
    xgl_physical_gpu_memory_properties_struct_wrapper(const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* pInStruct);

    virtual ~xgl_physical_gpu_memory_properties_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_SIZE get_structSize() { return m_struct.structSize; }
    void set_structSize(XGL_SIZE inValue) { m_struct.structSize = inValue; }
    XGL_BOOL get_supportsMigration() { return m_struct.supportsMigration; }
    void set_supportsMigration(XGL_BOOL inValue) { m_struct.supportsMigration = inValue; }
    XGL_BOOL get_supportsVirtualMemoryRemapping() { return m_struct.supportsVirtualMemoryRemapping; }
    void set_supportsVirtualMemoryRemapping(XGL_BOOL inValue) { m_struct.supportsVirtualMemoryRemapping = inValue; }
    XGL_BOOL get_supportsPinning() { return m_struct.supportsPinning; }
    void set_supportsPinning(XGL_BOOL inValue) { m_struct.supportsPinning = inValue; }


private:
    XGL_PHYSICAL_GPU_MEMORY_PROPERTIES m_struct;
    const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_pipeline_shader_struct_wrapper
{
public:
    xgl_pipeline_shader_struct_wrapper();
    xgl_pipeline_shader_struct_wrapper(XGL_PIPELINE_SHADER* pInStruct);
    xgl_pipeline_shader_struct_wrapper(const XGL_PIPELINE_SHADER* pInStruct);

    virtual ~xgl_pipeline_shader_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_PIPELINE_SHADER_STAGE get_stage() { return m_struct.stage; }
    void set_stage(XGL_PIPELINE_SHADER_STAGE inValue) { m_struct.stage = inValue; }
    XGL_SHADER get_shader() { return m_struct.shader; }
    void set_shader(XGL_SHADER inValue) { m_struct.shader = inValue; }
    XGL_UINT get_linkConstBufferCount() { return m_struct.linkConstBufferCount; }
    void set_linkConstBufferCount(XGL_UINT inValue) { m_struct.linkConstBufferCount = inValue; }
    const XGL_LINK_CONST_BUFFER* get_pLinkConstBufferInfo() { return m_struct.pLinkConstBufferInfo; }
    XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO get_dynamicMemoryViewMapping() { return m_struct.dynamicMemoryViewMapping; }
    void set_dynamicMemoryViewMapping(XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO inValue) { m_struct.dynamicMemoryViewMapping = inValue; }


private:
    XGL_PIPELINE_SHADER m_struct;
    const XGL_PIPELINE_SHADER* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_fence_create_info_struct_wrapper
{
public:
    xgl_fence_create_info_struct_wrapper();
    xgl_fence_create_info_struct_wrapper(XGL_FENCE_CREATE_INFO* pInStruct);
    xgl_fence_create_info_struct_wrapper(const XGL_FENCE_CREATE_INFO* pInStruct);

    virtual ~xgl_fence_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }


private:
    XGL_FENCE_CREATE_INFO m_struct;
    const XGL_FENCE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_pipeline_cb_attachment_state_struct_wrapper
{
public:
    xgl_pipeline_cb_attachment_state_struct_wrapper();
    xgl_pipeline_cb_attachment_state_struct_wrapper(XGL_PIPELINE_CB_ATTACHMENT_STATE* pInStruct);
    xgl_pipeline_cb_attachment_state_struct_wrapper(const XGL_PIPELINE_CB_ATTACHMENT_STATE* pInStruct);

    virtual ~xgl_pipeline_cb_attachment_state_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_BOOL get_blendEnable() { return m_struct.blendEnable; }
    void set_blendEnable(XGL_BOOL inValue) { m_struct.blendEnable = inValue; }
    XGL_FORMAT get_format() { return m_struct.format; }
    void set_format(XGL_FORMAT inValue) { m_struct.format = inValue; }
    XGL_UINT8 get_channelWriteMask() { return m_struct.channelWriteMask; }
    void set_channelWriteMask(XGL_UINT8 inValue) { m_struct.channelWriteMask = inValue; }


private:
    XGL_PIPELINE_CB_ATTACHMENT_STATE m_struct;
    const XGL_PIPELINE_CB_ATTACHMENT_STATE* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_alloc_callbacks_struct_wrapper
{
public:
    xgl_alloc_callbacks_struct_wrapper();
    xgl_alloc_callbacks_struct_wrapper(XGL_ALLOC_CALLBACKS* pInStruct);
    xgl_alloc_callbacks_struct_wrapper(const XGL_ALLOC_CALLBACKS* pInStruct);

    virtual ~xgl_alloc_callbacks_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_VOID* get_pUserData() { return m_struct.pUserData; }
    void set_pUserData(XGL_VOID* inValue) { m_struct.pUserData = inValue; }
    XGL_ALLOC_FUNCTION get_pfnAlloc() { return m_struct.pfnAlloc; }
    void set_pfnAlloc(XGL_ALLOC_FUNCTION inValue) { m_struct.pfnAlloc = inValue; }
    XGL_FREE_FUNCTION get_pfnFree() { return m_struct.pfnFree; }
    void set_pfnFree(XGL_FREE_FUNCTION inValue) { m_struct.pfnFree = inValue; }


private:
    XGL_ALLOC_CALLBACKS m_struct;
    const XGL_ALLOC_CALLBACKS* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_color_attachment_view_create_info_struct_wrapper
{
public:
    xgl_color_attachment_view_create_info_struct_wrapper();
    xgl_color_attachment_view_create_info_struct_wrapper(XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pInStruct);
    xgl_color_attachment_view_create_info_struct_wrapper(const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pInStruct);

    virtual ~xgl_color_attachment_view_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    XGL_VOID* get_pNext() { return m_struct.pNext; }
    void set_pNext(XGL_VOID* inValue) { m_struct.pNext = inValue; }
    XGL_IMAGE get_image() { return m_struct.image; }
    void set_image(XGL_IMAGE inValue) { m_struct.image = inValue; }
    XGL_FORMAT get_format() { return m_struct.format; }
    void set_format(XGL_FORMAT inValue) { m_struct.format = inValue; }
    XGL_UINT get_mipLevel() { return m_struct.mipLevel; }
    void set_mipLevel(XGL_UINT inValue) { m_struct.mipLevel = inValue; }
    XGL_UINT get_baseArraySlice() { return m_struct.baseArraySlice; }
    void set_baseArraySlice(XGL_UINT inValue) { m_struct.baseArraySlice = inValue; }
    XGL_UINT get_arraySize() { return m_struct.arraySize; }
    void set_arraySize(XGL_UINT inValue) { m_struct.arraySize = inValue; }


private:
    XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO m_struct;
    const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_image_copy_struct_wrapper
{
public:
    xgl_image_copy_struct_wrapper();
    xgl_image_copy_struct_wrapper(XGL_IMAGE_COPY* pInStruct);
    xgl_image_copy_struct_wrapper(const XGL_IMAGE_COPY* pInStruct);

    virtual ~xgl_image_copy_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_IMAGE_SUBRESOURCE get_srcSubresource() { return m_struct.srcSubresource; }
    void set_srcSubresource(XGL_IMAGE_SUBRESOURCE inValue) { m_struct.srcSubresource = inValue; }
    XGL_OFFSET3D get_srcOffset() { return m_struct.srcOffset; }
    void set_srcOffset(XGL_OFFSET3D inValue) { m_struct.srcOffset = inValue; }
    XGL_IMAGE_SUBRESOURCE get_destSubresource() { return m_struct.destSubresource; }
    void set_destSubresource(XGL_IMAGE_SUBRESOURCE inValue) { m_struct.destSubresource = inValue; }
    XGL_OFFSET3D get_destOffset() { return m_struct.destOffset; }
    void set_destOffset(XGL_OFFSET3D inValue) { m_struct.destOffset = inValue; }
    XGL_EXTENT3D get_extent() { return m_struct.extent; }
    void set_extent(XGL_EXTENT3D inValue) { m_struct.extent = inValue; }


private:
    XGL_IMAGE_COPY m_struct;
    const XGL_IMAGE_COPY* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_msaa_state_create_info_struct_wrapper
{
public:
    xgl_msaa_state_create_info_struct_wrapper();
    xgl_msaa_state_create_info_struct_wrapper(XGL_MSAA_STATE_CREATE_INFO* pInStruct);
    xgl_msaa_state_create_info_struct_wrapper(const XGL_MSAA_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_msaa_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_UINT get_samples() { return m_struct.samples; }
    void set_samples(XGL_UINT inValue) { m_struct.samples = inValue; }
    XGL_SAMPLE_MASK get_sampleMask() { return m_struct.sampleMask; }
    void set_sampleMask(XGL_SAMPLE_MASK inValue) { m_struct.sampleMask = inValue; }


private:
    XGL_MSAA_STATE_CREATE_INFO m_struct;
    const XGL_MSAA_STATE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_descriptor_set_create_info_struct_wrapper
{
public:
    xgl_descriptor_set_create_info_struct_wrapper();
    xgl_descriptor_set_create_info_struct_wrapper(XGL_DESCRIPTOR_SET_CREATE_INFO* pInStruct);
    xgl_descriptor_set_create_info_struct_wrapper(const XGL_DESCRIPTOR_SET_CREATE_INFO* pInStruct);

    virtual ~xgl_descriptor_set_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_UINT get_slots() { return m_struct.slots; }
    void set_slots(XGL_UINT inValue) { m_struct.slots = inValue; }


private:
    XGL_DESCRIPTOR_SET_CREATE_INFO m_struct;
    const XGL_DESCRIPTOR_SET_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_color_attachment_bind_info_struct_wrapper
{
public:
    xgl_color_attachment_bind_info_struct_wrapper();
    xgl_color_attachment_bind_info_struct_wrapper(XGL_COLOR_ATTACHMENT_BIND_INFO* pInStruct);
    xgl_color_attachment_bind_info_struct_wrapper(const XGL_COLOR_ATTACHMENT_BIND_INFO* pInStruct);

    virtual ~xgl_color_attachment_bind_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_COLOR_ATTACHMENT_VIEW get_view() { return m_struct.view; }
    void set_view(XGL_COLOR_ATTACHMENT_VIEW inValue) { m_struct.view = inValue; }
    XGL_IMAGE_STATE get_colorAttachmentState() { return m_struct.colorAttachmentState; }
    void set_colorAttachmentState(XGL_IMAGE_STATE inValue) { m_struct.colorAttachmentState = inValue; }


private:
    XGL_COLOR_ATTACHMENT_BIND_INFO m_struct;
    const XGL_COLOR_ATTACHMENT_BIND_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_event_create_info_struct_wrapper
{
public:
    xgl_event_create_info_struct_wrapper();
    xgl_event_create_info_struct_wrapper(XGL_EVENT_CREATE_INFO* pInStruct);
    xgl_event_create_info_struct_wrapper(const XGL_EVENT_CREATE_INFO* pInStruct);

    virtual ~xgl_event_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }


private:
    XGL_EVENT_CREATE_INFO m_struct;
    const XGL_EVENT_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_memory_requirements_struct_wrapper
{
public:
    xgl_memory_requirements_struct_wrapper();
    xgl_memory_requirements_struct_wrapper(XGL_MEMORY_REQUIREMENTS* pInStruct);
    xgl_memory_requirements_struct_wrapper(const XGL_MEMORY_REQUIREMENTS* pInStruct);

    virtual ~xgl_memory_requirements_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_GPU_SIZE get_size() { return m_struct.size; }
    void set_size(XGL_GPU_SIZE inValue) { m_struct.size = inValue; }
    XGL_GPU_SIZE get_alignment() { return m_struct.alignment; }
    void set_alignment(XGL_GPU_SIZE inValue) { m_struct.alignment = inValue; }
    XGL_UINT get_heapCount() { return m_struct.heapCount; }
    void set_heapCount(XGL_UINT inValue) { m_struct.heapCount = inValue; }


private:
    XGL_MEMORY_REQUIREMENTS m_struct;
    const XGL_MEMORY_REQUIREMENTS* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_queue_semaphore_open_info_struct_wrapper
{
public:
    xgl_queue_semaphore_open_info_struct_wrapper();
    xgl_queue_semaphore_open_info_struct_wrapper(XGL_QUEUE_SEMAPHORE_OPEN_INFO* pInStruct);
    xgl_queue_semaphore_open_info_struct_wrapper(const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pInStruct);

    virtual ~xgl_queue_semaphore_open_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_QUEUE_SEMAPHORE get_sharedSemaphore() { return m_struct.sharedSemaphore; }
    void set_sharedSemaphore(XGL_QUEUE_SEMAPHORE inValue) { m_struct.sharedSemaphore = inValue; }


private:
    XGL_QUEUE_SEMAPHORE_OPEN_INFO m_struct;
    const XGL_QUEUE_SEMAPHORE_OPEN_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_image_resolve_struct_wrapper
{
public:
    xgl_image_resolve_struct_wrapper();
    xgl_image_resolve_struct_wrapper(XGL_IMAGE_RESOLVE* pInStruct);
    xgl_image_resolve_struct_wrapper(const XGL_IMAGE_RESOLVE* pInStruct);

    virtual ~xgl_image_resolve_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_IMAGE_SUBRESOURCE get_srcSubresource() { return m_struct.srcSubresource; }
    void set_srcSubresource(XGL_IMAGE_SUBRESOURCE inValue) { m_struct.srcSubresource = inValue; }
    XGL_OFFSET2D get_srcOffset() { return m_struct.srcOffset; }
    void set_srcOffset(XGL_OFFSET2D inValue) { m_struct.srcOffset = inValue; }
    XGL_IMAGE_SUBRESOURCE get_destSubresource() { return m_struct.destSubresource; }
    void set_destSubresource(XGL_IMAGE_SUBRESOURCE inValue) { m_struct.destSubresource = inValue; }
    XGL_OFFSET2D get_destOffset() { return m_struct.destOffset; }
    void set_destOffset(XGL_OFFSET2D inValue) { m_struct.destOffset = inValue; }
    XGL_EXTENT2D get_extent() { return m_struct.extent; }
    void set_extent(XGL_EXTENT2D inValue) { m_struct.extent = inValue; }


private:
    XGL_IMAGE_RESOLVE m_struct;
    const XGL_IMAGE_RESOLVE* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_draw_indexed_indirect_cmd_struct_wrapper
{
public:
    xgl_draw_indexed_indirect_cmd_struct_wrapper();
    xgl_draw_indexed_indirect_cmd_struct_wrapper(XGL_DRAW_INDEXED_INDIRECT_CMD* pInStruct);
    xgl_draw_indexed_indirect_cmd_struct_wrapper(const XGL_DRAW_INDEXED_INDIRECT_CMD* pInStruct);

    virtual ~xgl_draw_indexed_indirect_cmd_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_UINT32 get_indexCount() { return m_struct.indexCount; }
    void set_indexCount(XGL_UINT32 inValue) { m_struct.indexCount = inValue; }
    XGL_UINT32 get_instanceCount() { return m_struct.instanceCount; }
    void set_instanceCount(XGL_UINT32 inValue) { m_struct.instanceCount = inValue; }
    XGL_UINT32 get_firstIndex() { return m_struct.firstIndex; }
    void set_firstIndex(XGL_UINT32 inValue) { m_struct.firstIndex = inValue; }
    XGL_INT32 get_vertexOffset() { return m_struct.vertexOffset; }
    void set_vertexOffset(XGL_INT32 inValue) { m_struct.vertexOffset = inValue; }
    XGL_UINT32 get_firstInstance() { return m_struct.firstInstance; }
    void set_firstInstance(XGL_UINT32 inValue) { m_struct.firstInstance = inValue; }


private:
    XGL_DRAW_INDEXED_INDIRECT_CMD m_struct;
    const XGL_DRAW_INDEXED_INDIRECT_CMD* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_compute_pipeline_create_info_struct_wrapper
{
public:
    xgl_compute_pipeline_create_info_struct_wrapper();
    xgl_compute_pipeline_create_info_struct_wrapper(XGL_COMPUTE_PIPELINE_CREATE_INFO* pInStruct);
    xgl_compute_pipeline_create_info_struct_wrapper(const XGL_COMPUTE_PIPELINE_CREATE_INFO* pInStruct);

    virtual ~xgl_compute_pipeline_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_PIPELINE_SHADER get_cs() { return m_struct.cs; }
    void set_cs(XGL_PIPELINE_SHADER inValue) { m_struct.cs = inValue; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }


private:
    XGL_COMPUTE_PIPELINE_CREATE_INFO m_struct;
    const XGL_COMPUTE_PIPELINE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_peer_image_open_info_struct_wrapper
{
public:
    xgl_peer_image_open_info_struct_wrapper();
    xgl_peer_image_open_info_struct_wrapper(XGL_PEER_IMAGE_OPEN_INFO* pInStruct);
    xgl_peer_image_open_info_struct_wrapper(const XGL_PEER_IMAGE_OPEN_INFO* pInStruct);

    virtual ~xgl_peer_image_open_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_IMAGE get_originalImage() { return m_struct.originalImage; }
    void set_originalImage(XGL_IMAGE inValue) { m_struct.originalImage = inValue; }


private:
    XGL_PEER_IMAGE_OPEN_INFO m_struct;
    const XGL_PEER_IMAGE_OPEN_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_physical_gpu_queue_properties_struct_wrapper
{
public:
    xgl_physical_gpu_queue_properties_struct_wrapper();
    xgl_physical_gpu_queue_properties_struct_wrapper(XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* pInStruct);
    xgl_physical_gpu_queue_properties_struct_wrapper(const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* pInStruct);

    virtual ~xgl_physical_gpu_queue_properties_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_SIZE get_structSize() { return m_struct.structSize; }
    void set_structSize(XGL_SIZE inValue) { m_struct.structSize = inValue; }
    XGL_FLAGS get_queueFlags() { return m_struct.queueFlags; }
    void set_queueFlags(XGL_FLAGS inValue) { m_struct.queueFlags = inValue; }
    XGL_UINT get_queueCount() { return m_struct.queueCount; }
    void set_queueCount(XGL_UINT inValue) { m_struct.queueCount = inValue; }
    XGL_UINT get_maxAtomicCounters() { return m_struct.maxAtomicCounters; }
    void set_maxAtomicCounters(XGL_UINT inValue) { m_struct.maxAtomicCounters = inValue; }
    XGL_BOOL get_supportsTimestamps() { return m_struct.supportsTimestamps; }
    void set_supportsTimestamps(XGL_BOOL inValue) { m_struct.supportsTimestamps = inValue; }


private:
    XGL_PHYSICAL_GPU_QUEUE_PROPERTIES m_struct;
    const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_pipeline_statistics_data_struct_wrapper
{
public:
    xgl_pipeline_statistics_data_struct_wrapper();
    xgl_pipeline_statistics_data_struct_wrapper(XGL_PIPELINE_STATISTICS_DATA* pInStruct);
    xgl_pipeline_statistics_data_struct_wrapper(const XGL_PIPELINE_STATISTICS_DATA* pInStruct);

    virtual ~xgl_pipeline_statistics_data_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_UINT64 get_fsInvocations() { return m_struct.fsInvocations; }
    void set_fsInvocations(XGL_UINT64 inValue) { m_struct.fsInvocations = inValue; }
    XGL_UINT64 get_cPrimitives() { return m_struct.cPrimitives; }
    void set_cPrimitives(XGL_UINT64 inValue) { m_struct.cPrimitives = inValue; }
    XGL_UINT64 get_cInvocations() { return m_struct.cInvocations; }
    void set_cInvocations(XGL_UINT64 inValue) { m_struct.cInvocations = inValue; }
    XGL_UINT64 get_vsInvocations() { return m_struct.vsInvocations; }
    void set_vsInvocations(XGL_UINT64 inValue) { m_struct.vsInvocations = inValue; }
    XGL_UINT64 get_gsInvocations() { return m_struct.gsInvocations; }
    void set_gsInvocations(XGL_UINT64 inValue) { m_struct.gsInvocations = inValue; }
    XGL_UINT64 get_gsPrimitives() { return m_struct.gsPrimitives; }
    void set_gsPrimitives(XGL_UINT64 inValue) { m_struct.gsPrimitives = inValue; }
    XGL_UINT64 get_iaPrimitives() { return m_struct.iaPrimitives; }
    void set_iaPrimitives(XGL_UINT64 inValue) { m_struct.iaPrimitives = inValue; }
    XGL_UINT64 get_iaVertices() { return m_struct.iaVertices; }
    void set_iaVertices(XGL_UINT64 inValue) { m_struct.iaVertices = inValue; }
    XGL_UINT64 get_tcsInvocations() { return m_struct.tcsInvocations; }
    void set_tcsInvocations(XGL_UINT64 inValue) { m_struct.tcsInvocations = inValue; }
    XGL_UINT64 get_tesInvocations() { return m_struct.tesInvocations; }
    void set_tesInvocations(XGL_UINT64 inValue) { m_struct.tesInvocations = inValue; }
    XGL_UINT64 get_csInvocations() { return m_struct.csInvocations; }
    void set_csInvocations(XGL_UINT64 inValue) { m_struct.csInvocations = inValue; }


private:
    XGL_PIPELINE_STATISTICS_DATA m_struct;
    const XGL_PIPELINE_STATISTICS_DATA* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_device_queue_create_info_struct_wrapper
{
public:
    xgl_device_queue_create_info_struct_wrapper();
    xgl_device_queue_create_info_struct_wrapper(XGL_DEVICE_QUEUE_CREATE_INFO* pInStruct);
    xgl_device_queue_create_info_struct_wrapper(const XGL_DEVICE_QUEUE_CREATE_INFO* pInStruct);

    virtual ~xgl_device_queue_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_UINT get_queueNodeIndex() { return m_struct.queueNodeIndex; }
    void set_queueNodeIndex(XGL_UINT inValue) { m_struct.queueNodeIndex = inValue; }
    XGL_UINT get_queueCount() { return m_struct.queueCount; }
    void set_queueCount(XGL_UINT inValue) { m_struct.queueCount = inValue; }


private:
    XGL_DEVICE_QUEUE_CREATE_INFO m_struct;
    const XGL_DEVICE_QUEUE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_sampler_create_info_struct_wrapper
{
public:
    xgl_sampler_create_info_struct_wrapper();
    xgl_sampler_create_info_struct_wrapper(XGL_SAMPLER_CREATE_INFO* pInStruct);
    xgl_sampler_create_info_struct_wrapper(const XGL_SAMPLER_CREATE_INFO* pInStruct);

    virtual ~xgl_sampler_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_TEX_FILTER get_magFilter() { return m_struct.magFilter; }
    void set_magFilter(XGL_TEX_FILTER inValue) { m_struct.magFilter = inValue; }
    XGL_TEX_FILTER get_minFilter() { return m_struct.minFilter; }
    void set_minFilter(XGL_TEX_FILTER inValue) { m_struct.minFilter = inValue; }
    XGL_TEX_MIPMAP_MODE get_mipMode() { return m_struct.mipMode; }
    void set_mipMode(XGL_TEX_MIPMAP_MODE inValue) { m_struct.mipMode = inValue; }
    XGL_TEX_ADDRESS get_addressU() { return m_struct.addressU; }
    void set_addressU(XGL_TEX_ADDRESS inValue) { m_struct.addressU = inValue; }
    XGL_TEX_ADDRESS get_addressV() { return m_struct.addressV; }
    void set_addressV(XGL_TEX_ADDRESS inValue) { m_struct.addressV = inValue; }
    XGL_TEX_ADDRESS get_addressW() { return m_struct.addressW; }
    void set_addressW(XGL_TEX_ADDRESS inValue) { m_struct.addressW = inValue; }
    XGL_FLOAT get_mipLodBias() { return m_struct.mipLodBias; }
    void set_mipLodBias(XGL_FLOAT inValue) { m_struct.mipLodBias = inValue; }
    XGL_UINT get_maxAnisotropy() { return m_struct.maxAnisotropy; }
    void set_maxAnisotropy(XGL_UINT inValue) { m_struct.maxAnisotropy = inValue; }
    XGL_COMPARE_FUNC get_compareFunc() { return m_struct.compareFunc; }
    void set_compareFunc(XGL_COMPARE_FUNC inValue) { m_struct.compareFunc = inValue; }
    XGL_FLOAT get_minLod() { return m_struct.minLod; }
    void set_minLod(XGL_FLOAT inValue) { m_struct.minLod = inValue; }
    XGL_FLOAT get_maxLod() { return m_struct.maxLod; }
    void set_maxLod(XGL_FLOAT inValue) { m_struct.maxLod = inValue; }
    XGL_BORDER_COLOR_TYPE get_borderColorType() { return m_struct.borderColorType; }
    void set_borderColorType(XGL_BORDER_COLOR_TYPE inValue) { m_struct.borderColorType = inValue; }


private:
    XGL_SAMPLER_CREATE_INFO m_struct;
    const XGL_SAMPLER_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_queue_semaphore_create_info_struct_wrapper
{
public:
    xgl_queue_semaphore_create_info_struct_wrapper();
    xgl_queue_semaphore_create_info_struct_wrapper(XGL_QUEUE_SEMAPHORE_CREATE_INFO* pInStruct);
    xgl_queue_semaphore_create_info_struct_wrapper(const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pInStruct);

    virtual ~xgl_queue_semaphore_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_UINT get_initialCount() { return m_struct.initialCount; }
    void set_initialCount(XGL_UINT inValue) { m_struct.initialCount = inValue; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }


private:
    XGL_QUEUE_SEMAPHORE_CREATE_INFO m_struct;
    const XGL_QUEUE_SEMAPHORE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_format_struct_wrapper
{
public:
    xgl_format_struct_wrapper();
    xgl_format_struct_wrapper(XGL_FORMAT* pInStruct);
    xgl_format_struct_wrapper(const XGL_FORMAT* pInStruct);

    virtual ~xgl_format_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_CHANNEL_FORMAT get_channelFormat() { return m_struct.channelFormat; }
    void set_channelFormat(XGL_CHANNEL_FORMAT inValue) { m_struct.channelFormat = inValue; }
    XGL_NUM_FORMAT get_numericFormat() { return m_struct.numericFormat; }
    void set_numericFormat(XGL_NUM_FORMAT inValue) { m_struct.numericFormat = inValue; }


private:
    XGL_FORMAT m_struct;
    const XGL_FORMAT* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_memory_state_transition_struct_wrapper
{
public:
    xgl_memory_state_transition_struct_wrapper();
    xgl_memory_state_transition_struct_wrapper(XGL_MEMORY_STATE_TRANSITION* pInStruct);
    xgl_memory_state_transition_struct_wrapper(const XGL_MEMORY_STATE_TRANSITION* pInStruct);

    virtual ~xgl_memory_state_transition_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    XGL_VOID* get_pNext() { return m_struct.pNext; }
    void set_pNext(XGL_VOID* inValue) { m_struct.pNext = inValue; }
    XGL_GPU_MEMORY get_mem() { return m_struct.mem; }
    void set_mem(XGL_GPU_MEMORY inValue) { m_struct.mem = inValue; }
    XGL_MEMORY_STATE get_oldState() { return m_struct.oldState; }
    void set_oldState(XGL_MEMORY_STATE inValue) { m_struct.oldState = inValue; }
    XGL_MEMORY_STATE get_newState() { return m_struct.newState; }
    void set_newState(XGL_MEMORY_STATE inValue) { m_struct.newState = inValue; }
    XGL_GPU_SIZE get_offset() { return m_struct.offset; }
    void set_offset(XGL_GPU_SIZE inValue) { m_struct.offset = inValue; }
    XGL_GPU_SIZE get_regionSize() { return m_struct.regionSize; }
    void set_regionSize(XGL_GPU_SIZE inValue) { m_struct.regionSize = inValue; }


private:
    XGL_MEMORY_STATE_TRANSITION m_struct;
    const XGL_MEMORY_STATE_TRANSITION* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_extent3d_struct_wrapper
{
public:
    xgl_extent3d_struct_wrapper();
    xgl_extent3d_struct_wrapper(XGL_EXTENT3D* pInStruct);
    xgl_extent3d_struct_wrapper(const XGL_EXTENT3D* pInStruct);

    virtual ~xgl_extent3d_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_INT get_width() { return m_struct.width; }
    void set_width(XGL_INT inValue) { m_struct.width = inValue; }
    XGL_INT get_height() { return m_struct.height; }
    void set_height(XGL_INT inValue) { m_struct.height = inValue; }
    XGL_INT get_depth() { return m_struct.depth; }
    void set_depth(XGL_INT inValue) { m_struct.depth = inValue; }


private:
    XGL_EXTENT3D m_struct;
    const XGL_EXTENT3D* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_dynamic_memory_view_slot_info_struct_wrapper
{
public:
    xgl_dynamic_memory_view_slot_info_struct_wrapper();
    xgl_dynamic_memory_view_slot_info_struct_wrapper(XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO* pInStruct);
    xgl_dynamic_memory_view_slot_info_struct_wrapper(const XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO* pInStruct);

    virtual ~xgl_dynamic_memory_view_slot_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_DESCRIPTOR_SET_SLOT_TYPE get_slotObjectType() { return m_struct.slotObjectType; }
    void set_slotObjectType(XGL_DESCRIPTOR_SET_SLOT_TYPE inValue) { m_struct.slotObjectType = inValue; }
    XGL_UINT get_shaderEntityIndex() { return m_struct.shaderEntityIndex; }
    void set_shaderEntityIndex(XGL_UINT inValue) { m_struct.shaderEntityIndex = inValue; }


private:
    XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO m_struct;
    const XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_image_view_attach_info_struct_wrapper
{
public:
    xgl_image_view_attach_info_struct_wrapper();
    xgl_image_view_attach_info_struct_wrapper(XGL_IMAGE_VIEW_ATTACH_INFO* pInStruct);
    xgl_image_view_attach_info_struct_wrapper(const XGL_IMAGE_VIEW_ATTACH_INFO* pInStruct);

    virtual ~xgl_image_view_attach_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    XGL_VOID* get_pNext() { return m_struct.pNext; }
    void set_pNext(XGL_VOID* inValue) { m_struct.pNext = inValue; }
    XGL_IMAGE_VIEW get_view() { return m_struct.view; }
    void set_view(XGL_IMAGE_VIEW inValue) { m_struct.view = inValue; }
    XGL_IMAGE_STATE get_state() { return m_struct.state; }
    void set_state(XGL_IMAGE_STATE inValue) { m_struct.state = inValue; }


private:
    XGL_IMAGE_VIEW_ATTACH_INFO m_struct;
    const XGL_IMAGE_VIEW_ATTACH_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_image_subresource_range_struct_wrapper
{
public:
    xgl_image_subresource_range_struct_wrapper();
    xgl_image_subresource_range_struct_wrapper(XGL_IMAGE_SUBRESOURCE_RANGE* pInStruct);
    xgl_image_subresource_range_struct_wrapper(const XGL_IMAGE_SUBRESOURCE_RANGE* pInStruct);

    virtual ~xgl_image_subresource_range_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_IMAGE_ASPECT get_aspect() { return m_struct.aspect; }
    void set_aspect(XGL_IMAGE_ASPECT inValue) { m_struct.aspect = inValue; }
    XGL_UINT get_baseMipLevel() { return m_struct.baseMipLevel; }
    void set_baseMipLevel(XGL_UINT inValue) { m_struct.baseMipLevel = inValue; }
    XGL_UINT get_mipLevels() { return m_struct.mipLevels; }
    void set_mipLevels(XGL_UINT inValue) { m_struct.mipLevels = inValue; }
    XGL_UINT get_baseArraySlice() { return m_struct.baseArraySlice; }
    void set_baseArraySlice(XGL_UINT inValue) { m_struct.baseArraySlice = inValue; }
    XGL_UINT get_arraySize() { return m_struct.arraySize; }
    void set_arraySize(XGL_UINT inValue) { m_struct.arraySize = inValue; }


private:
    XGL_IMAGE_SUBRESOURCE_RANGE m_struct;
    const XGL_IMAGE_SUBRESOURCE_RANGE* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_pipeline_db_state_create_info_struct_wrapper
{
public:
    xgl_pipeline_db_state_create_info_struct_wrapper();
    xgl_pipeline_db_state_create_info_struct_wrapper(XGL_PIPELINE_DB_STATE_CREATE_INFO* pInStruct);
    xgl_pipeline_db_state_create_info_struct_wrapper(const XGL_PIPELINE_DB_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_pipeline_db_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_FORMAT get_format() { return m_struct.format; }
    void set_format(XGL_FORMAT inValue) { m_struct.format = inValue; }


private:
    XGL_PIPELINE_DB_STATE_CREATE_INFO m_struct;
    const XGL_PIPELINE_DB_STATE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_application_info_struct_wrapper
{
public:
    xgl_application_info_struct_wrapper();
    xgl_application_info_struct_wrapper(XGL_APPLICATION_INFO* pInStruct);
    xgl_application_info_struct_wrapper(const XGL_APPLICATION_INFO* pInStruct);

    virtual ~xgl_application_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    XGL_VOID* get_pNext() { return m_struct.pNext; }
    void set_pNext(XGL_VOID* inValue) { m_struct.pNext = inValue; }
    const XGL_CHAR* get_pAppName() { return m_struct.pAppName; }
    XGL_UINT32 get_appVersion() { return m_struct.appVersion; }
    void set_appVersion(XGL_UINT32 inValue) { m_struct.appVersion = inValue; }
    const XGL_CHAR* get_pEngineName() { return m_struct.pEngineName; }
    XGL_UINT32 get_engineVersion() { return m_struct.engineVersion; }
    void set_engineVersion(XGL_UINT32 inValue) { m_struct.engineVersion = inValue; }
    XGL_UINT32 get_apiVersion() { return m_struct.apiVersion; }
    void set_apiVersion(XGL_UINT32 inValue) { m_struct.apiVersion = inValue; }


private:
    XGL_APPLICATION_INFO m_struct;
    const XGL_APPLICATION_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_offset2d_struct_wrapper
{
public:
    xgl_offset2d_struct_wrapper();
    xgl_offset2d_struct_wrapper(XGL_OFFSET2D* pInStruct);
    xgl_offset2d_struct_wrapper(const XGL_OFFSET2D* pInStruct);

    virtual ~xgl_offset2d_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_INT get_x() { return m_struct.x; }
    void set_x(XGL_INT inValue) { m_struct.x = inValue; }
    XGL_INT get_y() { return m_struct.y; }
    void set_y(XGL_INT inValue) { m_struct.y = inValue; }


private:
    XGL_OFFSET2D m_struct;
    const XGL_OFFSET2D* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_viewport_state_create_info_struct_wrapper
{
public:
    xgl_viewport_state_create_info_struct_wrapper();
    xgl_viewport_state_create_info_struct_wrapper(XGL_VIEWPORT_STATE_CREATE_INFO* pInStruct);
    xgl_viewport_state_create_info_struct_wrapper(const XGL_VIEWPORT_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_viewport_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_UINT get_viewportCount() { return m_struct.viewportCount; }
    void set_viewportCount(XGL_UINT inValue) { m_struct.viewportCount = inValue; }
    XGL_BOOL get_scissorEnable() { return m_struct.scissorEnable; }
    void set_scissorEnable(XGL_BOOL inValue) { m_struct.scissorEnable = inValue; }


private:
    XGL_VIEWPORT_STATE_CREATE_INFO m_struct;
    const XGL_VIEWPORT_STATE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_image_state_transition_struct_wrapper
{
public:
    xgl_image_state_transition_struct_wrapper();
    xgl_image_state_transition_struct_wrapper(XGL_IMAGE_STATE_TRANSITION* pInStruct);
    xgl_image_state_transition_struct_wrapper(const XGL_IMAGE_STATE_TRANSITION* pInStruct);

    virtual ~xgl_image_state_transition_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_IMAGE get_image() { return m_struct.image; }
    void set_image(XGL_IMAGE inValue) { m_struct.image = inValue; }
    XGL_IMAGE_STATE get_oldState() { return m_struct.oldState; }
    void set_oldState(XGL_IMAGE_STATE inValue) { m_struct.oldState = inValue; }
    XGL_IMAGE_STATE get_newState() { return m_struct.newState; }
    void set_newState(XGL_IMAGE_STATE inValue) { m_struct.newState = inValue; }
    XGL_IMAGE_SUBRESOURCE_RANGE get_subresourceRange() { return m_struct.subresourceRange; }
    void set_subresourceRange(XGL_IMAGE_SUBRESOURCE_RANGE inValue) { m_struct.subresourceRange = inValue; }


private:
    XGL_IMAGE_STATE_TRANSITION m_struct;
    const XGL_IMAGE_STATE_TRANSITION* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_device_create_info_struct_wrapper
{
public:
    xgl_device_create_info_struct_wrapper();
    xgl_device_create_info_struct_wrapper(XGL_DEVICE_CREATE_INFO* pInStruct);
    xgl_device_create_info_struct_wrapper(const XGL_DEVICE_CREATE_INFO* pInStruct);

    virtual ~xgl_device_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    XGL_VOID* get_pNext() { return m_struct.pNext; }
    void set_pNext(XGL_VOID* inValue) { m_struct.pNext = inValue; }
    XGL_UINT get_queueRecordCount() { return m_struct.queueRecordCount; }
    void set_queueRecordCount(XGL_UINT inValue) { m_struct.queueRecordCount = inValue; }
    const XGL_DEVICE_QUEUE_CREATE_INFO* get_pRequestedQueues() { return m_struct.pRequestedQueues; }
    XGL_UINT get_extensionCount() { return m_struct.extensionCount; }
    void set_extensionCount(XGL_UINT inValue) { m_struct.extensionCount = inValue; }
    const XGL_CHAR*const* get_ppEnabledExtensionNames() { return m_struct.ppEnabledExtensionNames; }
    XGL_VALIDATION_LEVEL get_maxValidationLevel() { return m_struct.maxValidationLevel; }
    void set_maxValidationLevel(XGL_VALIDATION_LEVEL inValue) { m_struct.maxValidationLevel = inValue; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }


private:
    XGL_DEVICE_CREATE_INFO m_struct;
    const XGL_DEVICE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_image_create_info_struct_wrapper
{
public:
    xgl_image_create_info_struct_wrapper();
    xgl_image_create_info_struct_wrapper(XGL_IMAGE_CREATE_INFO* pInStruct);
    xgl_image_create_info_struct_wrapper(const XGL_IMAGE_CREATE_INFO* pInStruct);

    virtual ~xgl_image_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_IMAGE_TYPE get_imageType() { return m_struct.imageType; }
    void set_imageType(XGL_IMAGE_TYPE inValue) { m_struct.imageType = inValue; }
    XGL_FORMAT get_format() { return m_struct.format; }
    void set_format(XGL_FORMAT inValue) { m_struct.format = inValue; }
    XGL_EXTENT3D get_extent() { return m_struct.extent; }
    void set_extent(XGL_EXTENT3D inValue) { m_struct.extent = inValue; }
    XGL_UINT get_mipLevels() { return m_struct.mipLevels; }
    void set_mipLevels(XGL_UINT inValue) { m_struct.mipLevels = inValue; }
    XGL_UINT get_arraySize() { return m_struct.arraySize; }
    void set_arraySize(XGL_UINT inValue) { m_struct.arraySize = inValue; }
    XGL_UINT get_samples() { return m_struct.samples; }
    void set_samples(XGL_UINT inValue) { m_struct.samples = inValue; }
    XGL_IMAGE_TILING get_tiling() { return m_struct.tiling; }
    void set_tiling(XGL_IMAGE_TILING inValue) { m_struct.tiling = inValue; }
    XGL_FLAGS get_usage() { return m_struct.usage; }
    void set_usage(XGL_FLAGS inValue) { m_struct.usage = inValue; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }


private:
    XGL_IMAGE_CREATE_INFO m_struct;
    const XGL_IMAGE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_rect_struct_wrapper
{
public:
    xgl_rect_struct_wrapper();
    xgl_rect_struct_wrapper(XGL_RECT* pInStruct);
    xgl_rect_struct_wrapper(const XGL_RECT* pInStruct);

    virtual ~xgl_rect_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_OFFSET2D get_offset() { return m_struct.offset; }
    void set_offset(XGL_OFFSET2D inValue) { m_struct.offset = inValue; }
    XGL_EXTENT2D get_extent() { return m_struct.extent; }
    void set_extent(XGL_EXTENT2D inValue) { m_struct.extent = inValue; }


private:
    XGL_RECT m_struct;
    const XGL_RECT* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_memory_copy_struct_wrapper
{
public:
    xgl_memory_copy_struct_wrapper();
    xgl_memory_copy_struct_wrapper(XGL_MEMORY_COPY* pInStruct);
    xgl_memory_copy_struct_wrapper(const XGL_MEMORY_COPY* pInStruct);

    virtual ~xgl_memory_copy_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_GPU_SIZE get_srcOffset() { return m_struct.srcOffset; }
    void set_srcOffset(XGL_GPU_SIZE inValue) { m_struct.srcOffset = inValue; }
    XGL_GPU_SIZE get_destOffset() { return m_struct.destOffset; }
    void set_destOffset(XGL_GPU_SIZE inValue) { m_struct.destOffset = inValue; }
    XGL_GPU_SIZE get_copySize() { return m_struct.copySize; }
    void set_copySize(XGL_GPU_SIZE inValue) { m_struct.copySize = inValue; }


private:
    XGL_MEMORY_COPY m_struct;
    const XGL_MEMORY_COPY* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_descriptor_slot_info_struct_wrapper
{
public:
    xgl_descriptor_slot_info_struct_wrapper();
    xgl_descriptor_slot_info_struct_wrapper(XGL_DESCRIPTOR_SLOT_INFO* pInStruct);
    xgl_descriptor_slot_info_struct_wrapper(const XGL_DESCRIPTOR_SLOT_INFO* pInStruct);

    virtual ~xgl_descriptor_slot_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_DESCRIPTOR_SET_SLOT_TYPE get_slotObjectType() { return m_struct.slotObjectType; }
    void set_slotObjectType(XGL_DESCRIPTOR_SET_SLOT_TYPE inValue) { m_struct.slotObjectType = inValue; }
    XGL_UINT get_shaderEntityIndex() { return m_struct.shaderEntityIndex; }
    void set_shaderEntityIndex(XGL_UINT inValue) { m_struct.shaderEntityIndex = inValue; }
    const struct _XGL_DESCRIPTOR_SET_MAPPING* get_pNextLevelSet() { return m_struct.pNextLevelSet; }


private:
    XGL_DESCRIPTOR_SLOT_INFO m_struct;
    const XGL_DESCRIPTOR_SLOT_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_link_const_buffer_struct_wrapper
{
public:
    xgl_link_const_buffer_struct_wrapper();
    xgl_link_const_buffer_struct_wrapper(XGL_LINK_CONST_BUFFER* pInStruct);
    xgl_link_const_buffer_struct_wrapper(const XGL_LINK_CONST_BUFFER* pInStruct);

    virtual ~xgl_link_const_buffer_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_UINT get_bufferId() { return m_struct.bufferId; }
    void set_bufferId(XGL_UINT inValue) { m_struct.bufferId = inValue; }
    XGL_SIZE get_bufferSize() { return m_struct.bufferSize; }
    void set_bufferSize(XGL_SIZE inValue) { m_struct.bufferSize = inValue; }
    const XGL_VOID* get_pBufferData() { return m_struct.pBufferData; }


private:
    XGL_LINK_CONST_BUFFER m_struct;
    const XGL_LINK_CONST_BUFFER* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_memory_image_copy_struct_wrapper
{
public:
    xgl_memory_image_copy_struct_wrapper();
    xgl_memory_image_copy_struct_wrapper(XGL_MEMORY_IMAGE_COPY* pInStruct);
    xgl_memory_image_copy_struct_wrapper(const XGL_MEMORY_IMAGE_COPY* pInStruct);

    virtual ~xgl_memory_image_copy_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_GPU_SIZE get_memOffset() { return m_struct.memOffset; }
    void set_memOffset(XGL_GPU_SIZE inValue) { m_struct.memOffset = inValue; }
    XGL_IMAGE_SUBRESOURCE get_imageSubresource() { return m_struct.imageSubresource; }
    void set_imageSubresource(XGL_IMAGE_SUBRESOURCE inValue) { m_struct.imageSubresource = inValue; }
    XGL_OFFSET3D get_imageOffset() { return m_struct.imageOffset; }
    void set_imageOffset(XGL_OFFSET3D inValue) { m_struct.imageOffset = inValue; }
    XGL_EXTENT3D get_imageExtent() { return m_struct.imageExtent; }
    void set_imageExtent(XGL_EXTENT3D inValue) { m_struct.imageExtent = inValue; }


private:
    XGL_MEMORY_IMAGE_COPY m_struct;
    const XGL_MEMORY_IMAGE_COPY* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_depth_stencil_state_create_info_struct_wrapper
{
public:
    xgl_depth_stencil_state_create_info_struct_wrapper();
    xgl_depth_stencil_state_create_info_struct_wrapper(XGL_DEPTH_STENCIL_STATE_CREATE_INFO* pInStruct);
    xgl_depth_stencil_state_create_info_struct_wrapper(const XGL_DEPTH_STENCIL_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_depth_stencil_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_BOOL get_depthTestEnable() { return m_struct.depthTestEnable; }
    void set_depthTestEnable(XGL_BOOL inValue) { m_struct.depthTestEnable = inValue; }
    XGL_BOOL get_depthWriteEnable() { return m_struct.depthWriteEnable; }
    void set_depthWriteEnable(XGL_BOOL inValue) { m_struct.depthWriteEnable = inValue; }
    XGL_COMPARE_FUNC get_depthFunc() { return m_struct.depthFunc; }
    void set_depthFunc(XGL_COMPARE_FUNC inValue) { m_struct.depthFunc = inValue; }
    XGL_BOOL get_depthBoundsEnable() { return m_struct.depthBoundsEnable; }
    void set_depthBoundsEnable(XGL_BOOL inValue) { m_struct.depthBoundsEnable = inValue; }
    XGL_FLOAT get_minDepth() { return m_struct.minDepth; }
    void set_minDepth(XGL_FLOAT inValue) { m_struct.minDepth = inValue; }
    XGL_FLOAT get_maxDepth() { return m_struct.maxDepth; }
    void set_maxDepth(XGL_FLOAT inValue) { m_struct.maxDepth = inValue; }
    XGL_BOOL get_stencilTestEnable() { return m_struct.stencilTestEnable; }
    void set_stencilTestEnable(XGL_BOOL inValue) { m_struct.stencilTestEnable = inValue; }
    XGL_UINT32 get_stencilReadMask() { return m_struct.stencilReadMask; }
    void set_stencilReadMask(XGL_UINT32 inValue) { m_struct.stencilReadMask = inValue; }
    XGL_UINT32 get_stencilWriteMask() { return m_struct.stencilWriteMask; }
    void set_stencilWriteMask(XGL_UINT32 inValue) { m_struct.stencilWriteMask = inValue; }
    XGL_STENCIL_OP_STATE get_front() { return m_struct.front; }
    void set_front(XGL_STENCIL_OP_STATE inValue) { m_struct.front = inValue; }
    XGL_STENCIL_OP_STATE get_back() { return m_struct.back; }
    void set_back(XGL_STENCIL_OP_STATE inValue) { m_struct.back = inValue; }


private:
    XGL_DEPTH_STENCIL_STATE_CREATE_INFO m_struct;
    const XGL_DEPTH_STENCIL_STATE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_viewport_struct_wrapper
{
public:
    xgl_viewport_struct_wrapper();
    xgl_viewport_struct_wrapper(XGL_VIEWPORT* pInStruct);
    xgl_viewport_struct_wrapper(const XGL_VIEWPORT* pInStruct);

    virtual ~xgl_viewport_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_FLOAT get_originX() { return m_struct.originX; }
    void set_originX(XGL_FLOAT inValue) { m_struct.originX = inValue; }
    XGL_FLOAT get_originY() { return m_struct.originY; }
    void set_originY(XGL_FLOAT inValue) { m_struct.originY = inValue; }
    XGL_FLOAT get_width() { return m_struct.width; }
    void set_width(XGL_FLOAT inValue) { m_struct.width = inValue; }
    XGL_FLOAT get_height() { return m_struct.height; }
    void set_height(XGL_FLOAT inValue) { m_struct.height = inValue; }
    XGL_FLOAT get_minDepth() { return m_struct.minDepth; }
    void set_minDepth(XGL_FLOAT inValue) { m_struct.minDepth = inValue; }
    XGL_FLOAT get_maxDepth() { return m_struct.maxDepth; }
    void set_maxDepth(XGL_FLOAT inValue) { m_struct.maxDepth = inValue; }


private:
    XGL_VIEWPORT m_struct;
    const XGL_VIEWPORT* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_descriptor_set_mapping_struct_wrapper
{
public:
    xgl_descriptor_set_mapping_struct_wrapper();
    xgl_descriptor_set_mapping_struct_wrapper(XGL_DESCRIPTOR_SET_MAPPING* pInStruct);
    xgl_descriptor_set_mapping_struct_wrapper(const XGL_DESCRIPTOR_SET_MAPPING* pInStruct);

    virtual ~xgl_descriptor_set_mapping_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_UINT get_descriptorCount() { return m_struct.descriptorCount; }
    void set_descriptorCount(XGL_UINT inValue) { m_struct.descriptorCount = inValue; }
    const XGL_DESCRIPTOR_SLOT_INFO* get_pDescriptorInfo() { return m_struct.pDescriptorInfo; }


private:
    XGL_DESCRIPTOR_SET_MAPPING m_struct;
    const XGL_DESCRIPTOR_SET_MAPPING* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_peer_memory_open_info_struct_wrapper
{
public:
    xgl_peer_memory_open_info_struct_wrapper();
    xgl_peer_memory_open_info_struct_wrapper(XGL_PEER_MEMORY_OPEN_INFO* pInStruct);
    xgl_peer_memory_open_info_struct_wrapper(const XGL_PEER_MEMORY_OPEN_INFO* pInStruct);

    virtual ~xgl_peer_memory_open_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    XGL_VOID* get_pNext() { return m_struct.pNext; }
    void set_pNext(XGL_VOID* inValue) { m_struct.pNext = inValue; }
    XGL_GPU_MEMORY get_originalMem() { return m_struct.originalMem; }
    void set_originalMem(XGL_GPU_MEMORY inValue) { m_struct.originalMem = inValue; }


private:
    XGL_PEER_MEMORY_OPEN_INFO m_struct;
    const XGL_PEER_MEMORY_OPEN_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_subresource_layout_struct_wrapper
{
public:
    xgl_subresource_layout_struct_wrapper();
    xgl_subresource_layout_struct_wrapper(XGL_SUBRESOURCE_LAYOUT* pInStruct);
    xgl_subresource_layout_struct_wrapper(const XGL_SUBRESOURCE_LAYOUT* pInStruct);

    virtual ~xgl_subresource_layout_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_GPU_SIZE get_offset() { return m_struct.offset; }
    void set_offset(XGL_GPU_SIZE inValue) { m_struct.offset = inValue; }
    XGL_GPU_SIZE get_size() { return m_struct.size; }
    void set_size(XGL_GPU_SIZE inValue) { m_struct.size = inValue; }
    XGL_GPU_SIZE get_rowPitch() { return m_struct.rowPitch; }
    void set_rowPitch(XGL_GPU_SIZE inValue) { m_struct.rowPitch = inValue; }
    XGL_GPU_SIZE get_depthPitch() { return m_struct.depthPitch; }
    void set_depthPitch(XGL_GPU_SIZE inValue) { m_struct.depthPitch = inValue; }


private:
    XGL_SUBRESOURCE_LAYOUT m_struct;
    const XGL_SUBRESOURCE_LAYOUT* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_descriptor_set_attach_info_struct_wrapper
{
public:
    xgl_descriptor_set_attach_info_struct_wrapper();
    xgl_descriptor_set_attach_info_struct_wrapper(XGL_DESCRIPTOR_SET_ATTACH_INFO* pInStruct);
    xgl_descriptor_set_attach_info_struct_wrapper(const XGL_DESCRIPTOR_SET_ATTACH_INFO* pInStruct);

    virtual ~xgl_descriptor_set_attach_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_DESCRIPTOR_SET get_descriptorSet() { return m_struct.descriptorSet; }
    void set_descriptorSet(XGL_DESCRIPTOR_SET inValue) { m_struct.descriptorSet = inValue; }
    XGL_UINT get_slotOffset() { return m_struct.slotOffset; }
    void set_slotOffset(XGL_UINT inValue) { m_struct.slotOffset = inValue; }


private:
    XGL_DESCRIPTOR_SET_ATTACH_INFO m_struct;
    const XGL_DESCRIPTOR_SET_ATTACH_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_pipeline_tess_state_create_info_struct_wrapper
{
public:
    xgl_pipeline_tess_state_create_info_struct_wrapper();
    xgl_pipeline_tess_state_create_info_struct_wrapper(XGL_PIPELINE_TESS_STATE_CREATE_INFO* pInStruct);
    xgl_pipeline_tess_state_create_info_struct_wrapper(const XGL_PIPELINE_TESS_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_pipeline_tess_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_UINT get_patchControlPoints() { return m_struct.patchControlPoints; }
    void set_patchControlPoints(XGL_UINT inValue) { m_struct.patchControlPoints = inValue; }
    XGL_FLOAT get_optimalTessFactor() { return m_struct.optimalTessFactor; }
    void set_optimalTessFactor(XGL_FLOAT inValue) { m_struct.optimalTessFactor = inValue; }
    XGL_FLOAT get_fixedTessFactor() { return m_struct.fixedTessFactor; }
    void set_fixedTessFactor(XGL_FLOAT inValue) { m_struct.fixedTessFactor = inValue; }


private:
    XGL_PIPELINE_TESS_STATE_CREATE_INFO m_struct;
    const XGL_PIPELINE_TESS_STATE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_pipeline_rs_state_create_info_struct_wrapper
{
public:
    xgl_pipeline_rs_state_create_info_struct_wrapper();
    xgl_pipeline_rs_state_create_info_struct_wrapper(XGL_PIPELINE_RS_STATE_CREATE_INFO* pInStruct);
    xgl_pipeline_rs_state_create_info_struct_wrapper(const XGL_PIPELINE_RS_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_pipeline_rs_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_BOOL get_depthClipEnable() { return m_struct.depthClipEnable; }
    void set_depthClipEnable(XGL_BOOL inValue) { m_struct.depthClipEnable = inValue; }
    XGL_BOOL get_rasterizerDiscardEnable() { return m_struct.rasterizerDiscardEnable; }
    void set_rasterizerDiscardEnable(XGL_BOOL inValue) { m_struct.rasterizerDiscardEnable = inValue; }
    XGL_FLOAT get_pointSize() { return m_struct.pointSize; }
    void set_pointSize(XGL_FLOAT inValue) { m_struct.pointSize = inValue; }


private:
    XGL_PIPELINE_RS_STATE_CREATE_INFO m_struct;
    const XGL_PIPELINE_RS_STATE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_stencil_op_state_struct_wrapper
{
public:
    xgl_stencil_op_state_struct_wrapper();
    xgl_stencil_op_state_struct_wrapper(XGL_STENCIL_OP_STATE* pInStruct);
    xgl_stencil_op_state_struct_wrapper(const XGL_STENCIL_OP_STATE* pInStruct);

    virtual ~xgl_stencil_op_state_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STENCIL_OP get_stencilFailOp() { return m_struct.stencilFailOp; }
    void set_stencilFailOp(XGL_STENCIL_OP inValue) { m_struct.stencilFailOp = inValue; }
    XGL_STENCIL_OP get_stencilPassOp() { return m_struct.stencilPassOp; }
    void set_stencilPassOp(XGL_STENCIL_OP inValue) { m_struct.stencilPassOp = inValue; }
    XGL_STENCIL_OP get_stencilDepthFailOp() { return m_struct.stencilDepthFailOp; }
    void set_stencilDepthFailOp(XGL_STENCIL_OP inValue) { m_struct.stencilDepthFailOp = inValue; }
    XGL_COMPARE_FUNC get_stencilFunc() { return m_struct.stencilFunc; }
    void set_stencilFunc(XGL_COMPARE_FUNC inValue) { m_struct.stencilFunc = inValue; }
    XGL_UINT32 get_stencilRef() { return m_struct.stencilRef; }
    void set_stencilRef(XGL_UINT32 inValue) { m_struct.stencilRef = inValue; }


private:
    XGL_STENCIL_OP_STATE m_struct;
    const XGL_STENCIL_OP_STATE* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_shader_create_info_struct_wrapper
{
public:
    xgl_shader_create_info_struct_wrapper();
    xgl_shader_create_info_struct_wrapper(XGL_SHADER_CREATE_INFO* pInStruct);
    xgl_shader_create_info_struct_wrapper(const XGL_SHADER_CREATE_INFO* pInStruct);

    virtual ~xgl_shader_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_SIZE get_codeSize() { return m_struct.codeSize; }
    void set_codeSize(XGL_SIZE inValue) { m_struct.codeSize = inValue; }
    const XGL_VOID* get_pCode() { return m_struct.pCode; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }


private:
    XGL_SHADER_CREATE_INFO m_struct;
    const XGL_SHADER_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_color_blend_state_create_info_struct_wrapper
{
public:
    xgl_color_blend_state_create_info_struct_wrapper();
    xgl_color_blend_state_create_info_struct_wrapper(XGL_COLOR_BLEND_STATE_CREATE_INFO* pInStruct);
    xgl_color_blend_state_create_info_struct_wrapper(const XGL_COLOR_BLEND_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_color_blend_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }


private:
    XGL_COLOR_BLEND_STATE_CREATE_INFO m_struct;
    const XGL_COLOR_BLEND_STATE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_pipeline_cb_state_create_info_struct_wrapper
{
public:
    xgl_pipeline_cb_state_create_info_struct_wrapper();
    xgl_pipeline_cb_state_create_info_struct_wrapper(XGL_PIPELINE_CB_STATE* pInStruct);
    xgl_pipeline_cb_state_create_info_struct_wrapper(const XGL_PIPELINE_CB_STATE* pInStruct);

    virtual ~xgl_pipeline_cb_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_BOOL get_alphaToCoverageEnable() { return m_struct.alphaToCoverageEnable; }
    void set_alphaToCoverageEnable(XGL_BOOL inValue) { m_struct.alphaToCoverageEnable = inValue; }
    XGL_BOOL get_dualSourceBlendEnable() { return m_struct.dualSourceBlendEnable; }
    void set_dualSourceBlendEnable(XGL_BOOL inValue) { m_struct.dualSourceBlendEnable = inValue; }
    XGL_LOGIC_OP get_logicOp() { return m_struct.logicOp; }
    void set_logicOp(XGL_LOGIC_OP inValue) { m_struct.logicOp = inValue; }


private:
    XGL_PIPELINE_CB_STATE m_struct;
    const XGL_PIPELINE_CB_STATE* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_channel_mapping_struct_wrapper
{
public:
    xgl_channel_mapping_struct_wrapper();
    xgl_channel_mapping_struct_wrapper(XGL_CHANNEL_MAPPING* pInStruct);
    xgl_channel_mapping_struct_wrapper(const XGL_CHANNEL_MAPPING* pInStruct);

    virtual ~xgl_channel_mapping_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_CHANNEL_SWIZZLE get_r() { return m_struct.r; }
    void set_r(XGL_CHANNEL_SWIZZLE inValue) { m_struct.r = inValue; }
    XGL_CHANNEL_SWIZZLE get_g() { return m_struct.g; }
    void set_g(XGL_CHANNEL_SWIZZLE inValue) { m_struct.g = inValue; }
    XGL_CHANNEL_SWIZZLE get_b() { return m_struct.b; }
    void set_b(XGL_CHANNEL_SWIZZLE inValue) { m_struct.b = inValue; }
    XGL_CHANNEL_SWIZZLE get_a() { return m_struct.a; }
    void set_a(XGL_CHANNEL_SWIZZLE inValue) { m_struct.a = inValue; }


private:
    XGL_CHANNEL_MAPPING m_struct;
    const XGL_CHANNEL_MAPPING* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_depth_stencil_view_create_info_struct_wrapper
{
public:
    xgl_depth_stencil_view_create_info_struct_wrapper();
    xgl_depth_stencil_view_create_info_struct_wrapper(XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pInStruct);
    xgl_depth_stencil_view_create_info_struct_wrapper(const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pInStruct);

    virtual ~xgl_depth_stencil_view_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_IMAGE get_image() { return m_struct.image; }
    void set_image(XGL_IMAGE inValue) { m_struct.image = inValue; }
    XGL_UINT get_mipLevel() { return m_struct.mipLevel; }
    void set_mipLevel(XGL_UINT inValue) { m_struct.mipLevel = inValue; }
    XGL_UINT get_baseArraySlice() { return m_struct.baseArraySlice; }
    void set_baseArraySlice(XGL_UINT inValue) { m_struct.baseArraySlice = inValue; }
    XGL_UINT get_arraySize() { return m_struct.arraySize; }
    void set_arraySize(XGL_UINT inValue) { m_struct.arraySize = inValue; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }


private:
    XGL_DEPTH_STENCIL_VIEW_CREATE_INFO m_struct;
    const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_virtual_memory_remap_range_struct_wrapper
{
public:
    xgl_virtual_memory_remap_range_struct_wrapper();
    xgl_virtual_memory_remap_range_struct_wrapper(XGL_VIRTUAL_MEMORY_REMAP_RANGE* pInStruct);
    xgl_virtual_memory_remap_range_struct_wrapper(const XGL_VIRTUAL_MEMORY_REMAP_RANGE* pInStruct);

    virtual ~xgl_virtual_memory_remap_range_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_GPU_MEMORY get_virtualMem() { return m_struct.virtualMem; }
    void set_virtualMem(XGL_GPU_MEMORY inValue) { m_struct.virtualMem = inValue; }
    XGL_GPU_SIZE get_virtualStartPage() { return m_struct.virtualStartPage; }
    void set_virtualStartPage(XGL_GPU_SIZE inValue) { m_struct.virtualStartPage = inValue; }
    XGL_GPU_MEMORY get_realMem() { return m_struct.realMem; }
    void set_realMem(XGL_GPU_MEMORY inValue) { m_struct.realMem = inValue; }
    XGL_GPU_SIZE get_realStartPage() { return m_struct.realStartPage; }
    void set_realStartPage(XGL_GPU_SIZE inValue) { m_struct.realStartPage = inValue; }
    XGL_GPU_SIZE get_pageCount() { return m_struct.pageCount; }
    void set_pageCount(XGL_GPU_SIZE inValue) { m_struct.pageCount = inValue; }


private:
    XGL_VIRTUAL_MEMORY_REMAP_RANGE m_struct;
    const XGL_VIRTUAL_MEMORY_REMAP_RANGE* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_cmd_buffer_create_info_struct_wrapper
{
public:
    xgl_cmd_buffer_create_info_struct_wrapper();
    xgl_cmd_buffer_create_info_struct_wrapper(XGL_CMD_BUFFER_CREATE_INFO* pInStruct);
    xgl_cmd_buffer_create_info_struct_wrapper(const XGL_CMD_BUFFER_CREATE_INFO* pInStruct);

    virtual ~xgl_cmd_buffer_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_QUEUE_TYPE get_queueType() { return m_struct.queueType; }
    void set_queueType(XGL_QUEUE_TYPE inValue) { m_struct.queueType = inValue; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }


private:
    XGL_CMD_BUFFER_CREATE_INFO m_struct;
    const XGL_CMD_BUFFER_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_format_properties_struct_wrapper
{
public:
    xgl_format_properties_struct_wrapper();
    xgl_format_properties_struct_wrapper(XGL_FORMAT_PROPERTIES* pInStruct);
    xgl_format_properties_struct_wrapper(const XGL_FORMAT_PROPERTIES* pInStruct);

    virtual ~xgl_format_properties_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_FLAGS get_linearTilingFeatures() { return m_struct.linearTilingFeatures; }
    void set_linearTilingFeatures(XGL_FLAGS inValue) { m_struct.linearTilingFeatures = inValue; }
    XGL_FLAGS get_optimalTilingFeatures() { return m_struct.optimalTilingFeatures; }
    void set_optimalTilingFeatures(XGL_FLAGS inValue) { m_struct.optimalTilingFeatures = inValue; }


private:
    XGL_FORMAT_PROPERTIES m_struct;
    const XGL_FORMAT_PROPERTIES* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_physical_gpu_properties_struct_wrapper
{
public:
    xgl_physical_gpu_properties_struct_wrapper();
    xgl_physical_gpu_properties_struct_wrapper(XGL_PHYSICAL_GPU_PROPERTIES* pInStruct);
    xgl_physical_gpu_properties_struct_wrapper(const XGL_PHYSICAL_GPU_PROPERTIES* pInStruct);

    virtual ~xgl_physical_gpu_properties_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_SIZE get_structSize() { return m_struct.structSize; }
    void set_structSize(XGL_SIZE inValue) { m_struct.structSize = inValue; }
    XGL_UINT32 get_apiVersion() { return m_struct.apiVersion; }
    void set_apiVersion(XGL_UINT32 inValue) { m_struct.apiVersion = inValue; }
    XGL_UINT32 get_driverVersion() { return m_struct.driverVersion; }
    void set_driverVersion(XGL_UINT32 inValue) { m_struct.driverVersion = inValue; }
    XGL_UINT32 get_vendorId() { return m_struct.vendorId; }
    void set_vendorId(XGL_UINT32 inValue) { m_struct.vendorId = inValue; }
    XGL_UINT32 get_deviceId() { return m_struct.deviceId; }
    void set_deviceId(XGL_UINT32 inValue) { m_struct.deviceId = inValue; }
    XGL_PHYSICAL_GPU_TYPE get_gpuType() { return m_struct.gpuType; }
    void set_gpuType(XGL_PHYSICAL_GPU_TYPE inValue) { m_struct.gpuType = inValue; }
    XGL_UINT get_maxMemRefsPerSubmission() { return m_struct.maxMemRefsPerSubmission; }
    void set_maxMemRefsPerSubmission(XGL_UINT inValue) { m_struct.maxMemRefsPerSubmission = inValue; }
    XGL_GPU_SIZE get_virtualMemPageSize() { return m_struct.virtualMemPageSize; }
    void set_virtualMemPageSize(XGL_GPU_SIZE inValue) { m_struct.virtualMemPageSize = inValue; }
    XGL_GPU_SIZE get_maxInlineMemoryUpdateSize() { return m_struct.maxInlineMemoryUpdateSize; }
    void set_maxInlineMemoryUpdateSize(XGL_GPU_SIZE inValue) { m_struct.maxInlineMemoryUpdateSize = inValue; }
    XGL_UINT get_maxBoundDescriptorSets() { return m_struct.maxBoundDescriptorSets; }
    void set_maxBoundDescriptorSets(XGL_UINT inValue) { m_struct.maxBoundDescriptorSets = inValue; }
    XGL_UINT get_maxThreadGroupSize() { return m_struct.maxThreadGroupSize; }
    void set_maxThreadGroupSize(XGL_UINT inValue) { m_struct.maxThreadGroupSize = inValue; }
    XGL_UINT64 get_timestampFrequency() { return m_struct.timestampFrequency; }
    void set_timestampFrequency(XGL_UINT64 inValue) { m_struct.timestampFrequency = inValue; }
    XGL_BOOL get_multiColorAttachmentClears() { return m_struct.multiColorAttachmentClears; }
    void set_multiColorAttachmentClears(XGL_BOOL inValue) { m_struct.multiColorAttachmentClears = inValue; }


private:
    XGL_PHYSICAL_GPU_PROPERTIES m_struct;
    const XGL_PHYSICAL_GPU_PROPERTIES* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_depth_stencil_bind_info_struct_wrapper
{
public:
    xgl_depth_stencil_bind_info_struct_wrapper();
    xgl_depth_stencil_bind_info_struct_wrapper(XGL_DEPTH_STENCIL_BIND_INFO* pInStruct);
    xgl_depth_stencil_bind_info_struct_wrapper(const XGL_DEPTH_STENCIL_BIND_INFO* pInStruct);

    virtual ~xgl_depth_stencil_bind_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_DEPTH_STENCIL_VIEW get_view() { return m_struct.view; }
    void set_view(XGL_DEPTH_STENCIL_VIEW inValue) { m_struct.view = inValue; }
    XGL_IMAGE_STATE get_depthState() { return m_struct.depthState; }
    void set_depthState(XGL_IMAGE_STATE inValue) { m_struct.depthState = inValue; }
    XGL_IMAGE_STATE get_stencilState() { return m_struct.stencilState; }
    void set_stencilState(XGL_IMAGE_STATE inValue) { m_struct.stencilState = inValue; }


private:
    XGL_DEPTH_STENCIL_BIND_INFO m_struct;
    const XGL_DEPTH_STENCIL_BIND_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_draw_indirect_cmd_struct_wrapper
{
public:
    xgl_draw_indirect_cmd_struct_wrapper();
    xgl_draw_indirect_cmd_struct_wrapper(XGL_DRAW_INDIRECT_CMD* pInStruct);
    xgl_draw_indirect_cmd_struct_wrapper(const XGL_DRAW_INDIRECT_CMD* pInStruct);

    virtual ~xgl_draw_indirect_cmd_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_UINT32 get_vertexCount() { return m_struct.vertexCount; }
    void set_vertexCount(XGL_UINT32 inValue) { m_struct.vertexCount = inValue; }
    XGL_UINT32 get_instanceCount() { return m_struct.instanceCount; }
    void set_instanceCount(XGL_UINT32 inValue) { m_struct.instanceCount = inValue; }
    XGL_UINT32 get_firstVertex() { return m_struct.firstVertex; }
    void set_firstVertex(XGL_UINT32 inValue) { m_struct.firstVertex = inValue; }
    XGL_UINT32 get_firstInstance() { return m_struct.firstInstance; }
    void set_firstInstance(XGL_UINT32 inValue) { m_struct.firstInstance = inValue; }


private:
    XGL_DRAW_INDIRECT_CMD m_struct;
    const XGL_DRAW_INDIRECT_CMD* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_graphics_pipeline_create_info_struct_wrapper
{
public:
    xgl_graphics_pipeline_create_info_struct_wrapper();
    xgl_graphics_pipeline_create_info_struct_wrapper(XGL_GRAPHICS_PIPELINE_CREATE_INFO* pInStruct);
    xgl_graphics_pipeline_create_info_struct_wrapper(const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pInStruct);

    virtual ~xgl_graphics_pipeline_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }


private:
    XGL_GRAPHICS_PIPELINE_CREATE_INFO m_struct;
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_pipeline_ia_state_create_info_struct_wrapper
{
public:
    xgl_pipeline_ia_state_create_info_struct_wrapper();
    xgl_pipeline_ia_state_create_info_struct_wrapper(XGL_PIPELINE_IA_STATE_CREATE_INFO* pInStruct);
    xgl_pipeline_ia_state_create_info_struct_wrapper(const XGL_PIPELINE_IA_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_pipeline_ia_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_PRIMITIVE_TOPOLOGY get_topology() { return m_struct.topology; }
    void set_topology(XGL_PRIMITIVE_TOPOLOGY inValue) { m_struct.topology = inValue; }
    XGL_BOOL get_disableVertexReuse() { return m_struct.disableVertexReuse; }
    void set_disableVertexReuse(XGL_BOOL inValue) { m_struct.disableVertexReuse = inValue; }
    XGL_PROVOKING_VERTEX_CONVENTION get_provokingVertex() { return m_struct.provokingVertex; }
    void set_provokingVertex(XGL_PROVOKING_VERTEX_CONVENTION inValue) { m_struct.provokingVertex = inValue; }
    XGL_BOOL get_primitiveRestartEnable() { return m_struct.primitiveRestartEnable; }
    void set_primitiveRestartEnable(XGL_BOOL inValue) { m_struct.primitiveRestartEnable = inValue; }
    XGL_UINT32 get_primitiveRestartIndex() { return m_struct.primitiveRestartIndex; }
    void set_primitiveRestartIndex(XGL_UINT32 inValue) { m_struct.primitiveRestartIndex = inValue; }


private:
    XGL_PIPELINE_IA_STATE_CREATE_INFO m_struct;
    const XGL_PIPELINE_IA_STATE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_color_attachment_blend_state_struct_wrapper
{
public:
    xgl_color_attachment_blend_state_struct_wrapper();
    xgl_color_attachment_blend_state_struct_wrapper(XGL_COLOR_ATTACHMENT_BLEND_STATE* pInStruct);
    xgl_color_attachment_blend_state_struct_wrapper(const XGL_COLOR_ATTACHMENT_BLEND_STATE* pInStruct);

    virtual ~xgl_color_attachment_blend_state_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_BOOL get_blendEnable() { return m_struct.blendEnable; }
    void set_blendEnable(XGL_BOOL inValue) { m_struct.blendEnable = inValue; }
    XGL_BLEND get_srcBlendColor() { return m_struct.srcBlendColor; }
    void set_srcBlendColor(XGL_BLEND inValue) { m_struct.srcBlendColor = inValue; }
    XGL_BLEND get_destBlendColor() { return m_struct.destBlendColor; }
    void set_destBlendColor(XGL_BLEND inValue) { m_struct.destBlendColor = inValue; }
    XGL_BLEND_FUNC get_blendFuncColor() { return m_struct.blendFuncColor; }
    void set_blendFuncColor(XGL_BLEND_FUNC inValue) { m_struct.blendFuncColor = inValue; }
    XGL_BLEND get_srcBlendAlpha() { return m_struct.srcBlendAlpha; }
    void set_srcBlendAlpha(XGL_BLEND inValue) { m_struct.srcBlendAlpha = inValue; }
    XGL_BLEND get_destBlendAlpha() { return m_struct.destBlendAlpha; }
    void set_destBlendAlpha(XGL_BLEND inValue) { m_struct.destBlendAlpha = inValue; }
    XGL_BLEND_FUNC get_blendFuncAlpha() { return m_struct.blendFuncAlpha; }
    void set_blendFuncAlpha(XGL_BLEND_FUNC inValue) { m_struct.blendFuncAlpha = inValue; }


private:
    XGL_COLOR_ATTACHMENT_BLEND_STATE m_struct;
    const XGL_COLOR_ATTACHMENT_BLEND_STATE* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_extent2d_struct_wrapper
{
public:
    xgl_extent2d_struct_wrapper();
    xgl_extent2d_struct_wrapper(XGL_EXTENT2D* pInStruct);
    xgl_extent2d_struct_wrapper(const XGL_EXTENT2D* pInStruct);

    virtual ~xgl_extent2d_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_INT get_width() { return m_struct.width; }
    void set_width(XGL_INT inValue) { m_struct.width = inValue; }
    XGL_INT get_height() { return m_struct.height; }
    void set_height(XGL_INT inValue) { m_struct.height = inValue; }


private:
    XGL_EXTENT2D m_struct;
    const XGL_EXTENT2D* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_memory_alloc_info_struct_wrapper
{
public:
    xgl_memory_alloc_info_struct_wrapper();
    xgl_memory_alloc_info_struct_wrapper(XGL_MEMORY_ALLOC_INFO* pInStruct);
    xgl_memory_alloc_info_struct_wrapper(const XGL_MEMORY_ALLOC_INFO* pInStruct);

    virtual ~xgl_memory_alloc_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    XGL_VOID* get_pNext() { return m_struct.pNext; }
    void set_pNext(XGL_VOID* inValue) { m_struct.pNext = inValue; }
    XGL_GPU_SIZE get_allocationSize() { return m_struct.allocationSize; }
    void set_allocationSize(XGL_GPU_SIZE inValue) { m_struct.allocationSize = inValue; }
    XGL_GPU_SIZE get_alignment() { return m_struct.alignment; }
    void set_alignment(XGL_GPU_SIZE inValue) { m_struct.alignment = inValue; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }
    XGL_UINT get_heapCount() { return m_struct.heapCount; }
    void set_heapCount(XGL_UINT inValue) { m_struct.heapCount = inValue; }
    XGL_MEMORY_PRIORITY get_memPriority() { return m_struct.memPriority; }
    void set_memPriority(XGL_MEMORY_PRIORITY inValue) { m_struct.memPriority = inValue; }


private:
    XGL_MEMORY_ALLOC_INFO m_struct;
    const XGL_MEMORY_ALLOC_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_memory_ref_struct_wrapper
{
public:
    xgl_memory_ref_struct_wrapper();
    xgl_memory_ref_struct_wrapper(XGL_MEMORY_REF* pInStruct);
    xgl_memory_ref_struct_wrapper(const XGL_MEMORY_REF* pInStruct);

    virtual ~xgl_memory_ref_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_GPU_MEMORY get_mem() { return m_struct.mem; }
    void set_mem(XGL_GPU_MEMORY inValue) { m_struct.mem = inValue; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }


private:
    XGL_MEMORY_REF m_struct;
    const XGL_MEMORY_REF* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_query_pool_create_info_struct_wrapper
{
public:
    xgl_query_pool_create_info_struct_wrapper();
    xgl_query_pool_create_info_struct_wrapper(XGL_QUERY_POOL_CREATE_INFO* pInStruct);
    xgl_query_pool_create_info_struct_wrapper(const XGL_QUERY_POOL_CREATE_INFO* pInStruct);

    virtual ~xgl_query_pool_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_QUERY_TYPE get_queryType() { return m_struct.queryType; }
    void set_queryType(XGL_QUERY_TYPE inValue) { m_struct.queryType = inValue; }
    XGL_UINT get_slots() { return m_struct.slots; }
    void set_slots(XGL_UINT inValue) { m_struct.slots = inValue; }


private:
    XGL_QUERY_POOL_CREATE_INFO m_struct;
    const XGL_QUERY_POOL_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_offset3d_struct_wrapper
{
public:
    xgl_offset3d_struct_wrapper();
    xgl_offset3d_struct_wrapper(XGL_OFFSET3D* pInStruct);
    xgl_offset3d_struct_wrapper(const XGL_OFFSET3D* pInStruct);

    virtual ~xgl_offset3d_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_INT get_x() { return m_struct.x; }
    void set_x(XGL_INT inValue) { m_struct.x = inValue; }
    XGL_INT get_y() { return m_struct.y; }
    void set_y(XGL_INT inValue) { m_struct.y = inValue; }
    XGL_INT get_z() { return m_struct.z; }
    void set_z(XGL_INT inValue) { m_struct.z = inValue; }


private:
    XGL_OFFSET3D m_struct;
    const XGL_OFFSET3D* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_pipeline_shader_stage_create_info_struct_wrapper
{
public:
    xgl_pipeline_shader_stage_create_info_struct_wrapper();
    xgl_pipeline_shader_stage_create_info_struct_wrapper(XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pInStruct);
    xgl_pipeline_shader_stage_create_info_struct_wrapper(const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pInStruct);

    virtual ~xgl_pipeline_shader_stage_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const XGL_VOID* get_pNext() { return m_struct.pNext; }
    XGL_PIPELINE_SHADER get_shader() { return m_struct.shader; }
    void set_shader(XGL_PIPELINE_SHADER inValue) { m_struct.shader = inValue; }


private:
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO m_struct;
    const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_memory_view_attach_info_struct_wrapper
{
public:
    xgl_memory_view_attach_info_struct_wrapper();
    xgl_memory_view_attach_info_struct_wrapper(XGL_MEMORY_VIEW_ATTACH_INFO* pInStruct);
    xgl_memory_view_attach_info_struct_wrapper(const XGL_MEMORY_VIEW_ATTACH_INFO* pInStruct);

    virtual ~xgl_memory_view_attach_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    XGL_VOID* get_pNext() { return m_struct.pNext; }
    void set_pNext(XGL_VOID* inValue) { m_struct.pNext = inValue; }
    XGL_GPU_MEMORY get_mem() { return m_struct.mem; }
    void set_mem(XGL_GPU_MEMORY inValue) { m_struct.mem = inValue; }
    XGL_GPU_SIZE get_offset() { return m_struct.offset; }
    void set_offset(XGL_GPU_SIZE inValue) { m_struct.offset = inValue; }
    XGL_GPU_SIZE get_range() { return m_struct.range; }
    void set_range(XGL_GPU_SIZE inValue) { m_struct.range = inValue; }
    XGL_GPU_SIZE get_stride() { return m_struct.stride; }
    void set_stride(XGL_GPU_SIZE inValue) { m_struct.stride = inValue; }
    XGL_FORMAT get_format() { return m_struct.format; }
    void set_format(XGL_FORMAT inValue) { m_struct.format = inValue; }
    XGL_MEMORY_STATE get_state() { return m_struct.state; }
    void set_state(XGL_MEMORY_STATE inValue) { m_struct.state = inValue; }


private:
    XGL_MEMORY_VIEW_ATTACH_INFO m_struct;
    const XGL_MEMORY_VIEW_ATTACH_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_dispatch_indirect_cmd_struct_wrapper
{
public:
    xgl_dispatch_indirect_cmd_struct_wrapper();
    xgl_dispatch_indirect_cmd_struct_wrapper(XGL_DISPATCH_INDIRECT_CMD* pInStruct);
    xgl_dispatch_indirect_cmd_struct_wrapper(const XGL_DISPATCH_INDIRECT_CMD* pInStruct);

    virtual ~xgl_dispatch_indirect_cmd_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_UINT32 get_x() { return m_struct.x; }
    void set_x(XGL_UINT32 inValue) { m_struct.x = inValue; }
    XGL_UINT32 get_y() { return m_struct.y; }
    void set_y(XGL_UINT32 inValue) { m_struct.y = inValue; }
    XGL_UINT32 get_z() { return m_struct.z; }
    void set_z(XGL_UINT32 inValue) { m_struct.z = inValue; }


private:
    XGL_DISPATCH_INDIRECT_CMD m_struct;
    const XGL_DISPATCH_INDIRECT_CMD* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};

//any footer info for class
