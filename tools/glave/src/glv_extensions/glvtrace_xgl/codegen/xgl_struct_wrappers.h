//This is the copyright
//#includes, #defines, globals and such...
#include <xgl.h>
#include <xgl_enum_string_helper.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//class declaration
class xgl_buffer_view_attach_info_struct_wrapper
{
public:
    xgl_buffer_view_attach_info_struct_wrapper();
    xgl_buffer_view_attach_info_struct_wrapper(XGL_BUFFER_VIEW_ATTACH_INFO* pInStruct);
    xgl_buffer_view_attach_info_struct_wrapper(const XGL_BUFFER_VIEW_ATTACH_INFO* pInStruct);

    virtual ~xgl_buffer_view_attach_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_BUFFER_VIEW get_view() { return m_struct.view; }
    void set_view(XGL_BUFFER_VIEW inValue) { m_struct.view = inValue; }


private:
    XGL_BUFFER_VIEW_ATTACH_INFO m_struct;
    const XGL_BUFFER_VIEW_ATTACH_INFO* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
    XGL_IMAGE_TYPE get_imageType() { return m_struct.imageType; }
    void set_imageType(XGL_IMAGE_TYPE inValue) { m_struct.imageType = inValue; }
    XGL_FORMAT get_format() { return m_struct.format; }
    void set_format(XGL_FORMAT inValue) { m_struct.format = inValue; }
    XGL_EXTENT3D get_extent() { return m_struct.extent; }
    void set_extent(XGL_EXTENT3D inValue) { m_struct.extent = inValue; }
    uint32_t get_mipLevels() { return m_struct.mipLevels; }
    void set_mipLevels(uint32_t inValue) { m_struct.mipLevels = inValue; }
    uint32_t get_arraySize() { return m_struct.arraySize; }
    void set_arraySize(uint32_t inValue) { m_struct.arraySize = inValue; }
    uint32_t get_samples() { return m_struct.samples; }
    void set_samples(uint32_t inValue) { m_struct.samples = inValue; }
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
    const void* get_pNext() { return m_struct.pNext; }
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
    float get_minLod() { return m_struct.minLod; }
    void set_minLod(float inValue) { m_struct.minLod = inValue; }


private:
    XGL_IMAGE_VIEW_CREATE_INFO m_struct;
    const XGL_IMAGE_VIEW_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_layer_create_info_struct_wrapper
{
public:
    xgl_layer_create_info_struct_wrapper();
    xgl_layer_create_info_struct_wrapper(XGL_LAYER_CREATE_INFO* pInStruct);
    xgl_layer_create_info_struct_wrapper(const XGL_LAYER_CREATE_INFO* pInStruct);

    virtual ~xgl_layer_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_layerCount() { return m_struct.layerCount; }
    void set_layerCount(uint32_t inValue) { m_struct.layerCount = inValue; }


private:
    XGL_LAYER_CREATE_INFO m_struct;
    const XGL_LAYER_CREATE_INFO* m_origStructAddr;
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
    uint32_t get_queueNodeIndex() { return m_struct.queueNodeIndex; }
    void set_queueNodeIndex(uint32_t inValue) { m_struct.queueNodeIndex = inValue; }
    uint32_t get_queueCount() { return m_struct.queueCount; }
    void set_queueCount(uint32_t inValue) { m_struct.queueCount = inValue; }


private:
    XGL_DEVICE_QUEUE_CREATE_INFO m_struct;
    const XGL_DEVICE_QUEUE_CREATE_INFO* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
    XGL_PIPELINE_SHADER get_cs() { return m_struct.cs; }
    void set_cs(XGL_PIPELINE_SHADER inValue) { m_struct.cs = inValue; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }
    XGL_DESCRIPTOR_SET_LAYOUT get_lastSetLayout() { return m_struct.lastSetLayout; }
    void set_lastSetLayout(XGL_DESCRIPTOR_SET_LAYOUT inValue) { m_struct.lastSetLayout = inValue; }
    uint32_t get_localSizeX() { return m_struct.localSizeX; }
    void set_localSizeX(uint32_t inValue) { m_struct.localSizeX = inValue; }
    uint32_t get_localSizeY() { return m_struct.localSizeY; }
    void set_localSizeY(uint32_t inValue) { m_struct.localSizeY = inValue; }
    uint32_t get_localSizeZ() { return m_struct.localSizeZ; }
    void set_localSizeZ(uint32_t inValue) { m_struct.localSizeZ = inValue; }


private:
    XGL_COMPUTE_PIPELINE_CREATE_INFO m_struct;
    const XGL_COMPUTE_PIPELINE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_clear_color_value_struct_wrapper
{
public:
    xgl_clear_color_value_struct_wrapper();
    xgl_clear_color_value_struct_wrapper(XGL_CLEAR_COLOR_VALUE* pInStruct);
    xgl_clear_color_value_struct_wrapper(const XGL_CLEAR_COLOR_VALUE* pInStruct);

    virtual ~xgl_clear_color_value_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }


private:
    XGL_CLEAR_COLOR_VALUE m_struct;
    const XGL_CLEAR_COLOR_VALUE* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
    bool32_t get_depthClipEnable() { return m_struct.depthClipEnable; }
    void set_depthClipEnable(bool32_t inValue) { m_struct.depthClipEnable = inValue; }
    bool32_t get_rasterizerDiscardEnable() { return m_struct.rasterizerDiscardEnable; }
    void set_rasterizerDiscardEnable(bool32_t inValue) { m_struct.rasterizerDiscardEnable = inValue; }
    bool32_t get_programPointSize() { return m_struct.programPointSize; }
    void set_programPointSize(bool32_t inValue) { m_struct.programPointSize = inValue; }
    XGL_COORDINATE_ORIGIN get_pointOrigin() { return m_struct.pointOrigin; }
    void set_pointOrigin(XGL_COORDINATE_ORIGIN inValue) { m_struct.pointOrigin = inValue; }
    XGL_PROVOKING_VERTEX_CONVENTION get_provokingVertex() { return m_struct.provokingVertex; }
    void set_provokingVertex(XGL_PROVOKING_VERTEX_CONVENTION inValue) { m_struct.provokingVertex = inValue; }
    XGL_FILL_MODE get_fillMode() { return m_struct.fillMode; }
    void set_fillMode(XGL_FILL_MODE inValue) { m_struct.fillMode = inValue; }
    XGL_CULL_MODE get_cullMode() { return m_struct.cullMode; }
    void set_cullMode(XGL_CULL_MODE inValue) { m_struct.cullMode = inValue; }
    XGL_FACE_ORIENTATION get_frontFace() { return m_struct.frontFace; }
    void set_frontFace(XGL_FACE_ORIENTATION inValue) { m_struct.frontFace = inValue; }


private:
    XGL_PIPELINE_RS_STATE_CREATE_INFO m_struct;
    const XGL_PIPELINE_RS_STATE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_image_memory_bind_info_struct_wrapper
{
public:
    xgl_image_memory_bind_info_struct_wrapper();
    xgl_image_memory_bind_info_struct_wrapper(XGL_IMAGE_MEMORY_BIND_INFO* pInStruct);
    xgl_image_memory_bind_info_struct_wrapper(const XGL_IMAGE_MEMORY_BIND_INFO* pInStruct);

    virtual ~xgl_image_memory_bind_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_IMAGE_SUBRESOURCE get_subresource() { return m_struct.subresource; }
    void set_subresource(XGL_IMAGE_SUBRESOURCE inValue) { m_struct.subresource = inValue; }
    XGL_OFFSET3D get_offset() { return m_struct.offset; }
    void set_offset(XGL_OFFSET3D inValue) { m_struct.offset = inValue; }
    XGL_EXTENT3D get_extent() { return m_struct.extent; }
    void set_extent(XGL_EXTENT3D inValue) { m_struct.extent = inValue; }


private:
    XGL_IMAGE_MEMORY_BIND_INFO m_struct;
    const XGL_IMAGE_MEMORY_BIND_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_memory_barrier_struct_wrapper
{
public:
    xgl_memory_barrier_struct_wrapper();
    xgl_memory_barrier_struct_wrapper(XGL_MEMORY_BARRIER* pInStruct);
    xgl_memory_barrier_struct_wrapper(const XGL_MEMORY_BARRIER* pInStruct);

    virtual ~xgl_memory_barrier_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_FLAGS get_outputMask() { return m_struct.outputMask; }
    void set_outputMask(XGL_FLAGS inValue) { m_struct.outputMask = inValue; }
    XGL_FLAGS get_inputMask() { return m_struct.inputMask; }
    void set_inputMask(XGL_FLAGS inValue) { m_struct.inputMask = inValue; }


private:
    XGL_MEMORY_BARRIER m_struct;
    const XGL_MEMORY_BARRIER* m_origStructAddr;
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
class xgl_image_memory_requirements_struct_wrapper
{
public:
    xgl_image_memory_requirements_struct_wrapper();
    xgl_image_memory_requirements_struct_wrapper(XGL_IMAGE_MEMORY_REQUIREMENTS* pInStruct);
    xgl_image_memory_requirements_struct_wrapper(const XGL_IMAGE_MEMORY_REQUIREMENTS* pInStruct);

    virtual ~xgl_image_memory_requirements_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_FLAGS get_usage() { return m_struct.usage; }
    void set_usage(XGL_FLAGS inValue) { m_struct.usage = inValue; }
    XGL_IMAGE_FORMAT_CLASS get_formatClass() { return m_struct.formatClass; }
    void set_formatClass(XGL_IMAGE_FORMAT_CLASS inValue) { m_struct.formatClass = inValue; }
    uint32_t get_samples() { return m_struct.samples; }
    void set_samples(uint32_t inValue) { m_struct.samples = inValue; }


private:
    XGL_IMAGE_MEMORY_REQUIREMENTS m_struct;
    const XGL_IMAGE_MEMORY_REQUIREMENTS* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_image_memory_barrier_struct_wrapper
{
public:
    xgl_image_memory_barrier_struct_wrapper();
    xgl_image_memory_barrier_struct_wrapper(XGL_IMAGE_MEMORY_BARRIER* pInStruct);
    xgl_image_memory_barrier_struct_wrapper(const XGL_IMAGE_MEMORY_BARRIER* pInStruct);

    virtual ~xgl_image_memory_barrier_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_FLAGS get_outputMask() { return m_struct.outputMask; }
    void set_outputMask(XGL_FLAGS inValue) { m_struct.outputMask = inValue; }
    XGL_FLAGS get_inputMask() { return m_struct.inputMask; }
    void set_inputMask(XGL_FLAGS inValue) { m_struct.inputMask = inValue; }
    XGL_IMAGE_LAYOUT get_oldLayout() { return m_struct.oldLayout; }
    void set_oldLayout(XGL_IMAGE_LAYOUT inValue) { m_struct.oldLayout = inValue; }
    XGL_IMAGE_LAYOUT get_newLayout() { return m_struct.newLayout; }
    void set_newLayout(XGL_IMAGE_LAYOUT inValue) { m_struct.newLayout = inValue; }
    XGL_IMAGE get_image() { return m_struct.image; }
    void set_image(XGL_IMAGE inValue) { m_struct.image = inValue; }
    XGL_IMAGE_SUBRESOURCE_RANGE get_subresourceRange() { return m_struct.subresourceRange; }
    void set_subresourceRange(XGL_IMAGE_SUBRESOURCE_RANGE inValue) { m_struct.subresourceRange = inValue; }


private:
    XGL_IMAGE_MEMORY_BARRIER m_struct;
    const XGL_IMAGE_MEMORY_BARRIER* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_update_sampler_textures_struct_wrapper
{
public:
    xgl_update_sampler_textures_struct_wrapper();
    xgl_update_sampler_textures_struct_wrapper(XGL_UPDATE_SAMPLER_TEXTURES* pInStruct);
    xgl_update_sampler_textures_struct_wrapper(const XGL_UPDATE_SAMPLER_TEXTURES* pInStruct);

    virtual ~xgl_update_sampler_textures_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_index() { return m_struct.index; }
    void set_index(uint32_t inValue) { m_struct.index = inValue; }
    uint32_t get_count() { return m_struct.count; }
    void set_count(uint32_t inValue) { m_struct.count = inValue; }


private:
    XGL_UPDATE_SAMPLER_TEXTURES m_struct;
    const XGL_UPDATE_SAMPLER_TEXTURES* m_origStructAddr;
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
    uint32_t get_apiVersion() { return m_struct.apiVersion; }
    void set_apiVersion(uint32_t inValue) { m_struct.apiVersion = inValue; }
    uint32_t get_driverVersion() { return m_struct.driverVersion; }
    void set_driverVersion(uint32_t inValue) { m_struct.driverVersion = inValue; }
    uint32_t get_vendorId() { return m_struct.vendorId; }
    void set_vendorId(uint32_t inValue) { m_struct.vendorId = inValue; }
    uint32_t get_deviceId() { return m_struct.deviceId; }
    void set_deviceId(uint32_t inValue) { m_struct.deviceId = inValue; }
    XGL_PHYSICAL_GPU_TYPE get_gpuType() { return m_struct.gpuType; }
    void set_gpuType(XGL_PHYSICAL_GPU_TYPE inValue) { m_struct.gpuType = inValue; }
    uint32_t get_maxMemRefsPerSubmission() { return m_struct.maxMemRefsPerSubmission; }
    void set_maxMemRefsPerSubmission(uint32_t inValue) { m_struct.maxMemRefsPerSubmission = inValue; }
    XGL_GPU_SIZE get_maxInlineMemoryUpdateSize() { return m_struct.maxInlineMemoryUpdateSize; }
    void set_maxInlineMemoryUpdateSize(XGL_GPU_SIZE inValue) { m_struct.maxInlineMemoryUpdateSize = inValue; }
    uint32_t get_maxBoundDescriptorSets() { return m_struct.maxBoundDescriptorSets; }
    void set_maxBoundDescriptorSets(uint32_t inValue) { m_struct.maxBoundDescriptorSets = inValue; }
    uint32_t get_maxThreadGroupSize() { return m_struct.maxThreadGroupSize; }
    void set_maxThreadGroupSize(uint32_t inValue) { m_struct.maxThreadGroupSize = inValue; }
    uint64_t get_timestampFrequency() { return m_struct.timestampFrequency; }
    void set_timestampFrequency(uint64_t inValue) { m_struct.timestampFrequency = inValue; }
    bool32_t get_multiColorAttachmentClears() { return m_struct.multiColorAttachmentClears; }
    void set_multiColorAttachmentClears(bool32_t inValue) { m_struct.multiColorAttachmentClears = inValue; }
    uint32_t get_maxDescriptorSets() { return m_struct.maxDescriptorSets; }
    void set_maxDescriptorSets(uint32_t inValue) { m_struct.maxDescriptorSets = inValue; }
    uint32_t get_maxViewports() { return m_struct.maxViewports; }
    void set_maxViewports(uint32_t inValue) { m_struct.maxViewports = inValue; }
    uint32_t get_maxColorAttachments() { return m_struct.maxColorAttachments; }
    void set_maxColorAttachments(uint32_t inValue) { m_struct.maxColorAttachments = inValue; }


private:
    XGL_PHYSICAL_GPU_PROPERTIES m_struct;
    const XGL_PHYSICAL_GPU_PROPERTIES* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_buffer_memory_requirements_struct_wrapper
{
public:
    xgl_buffer_memory_requirements_struct_wrapper();
    xgl_buffer_memory_requirements_struct_wrapper(XGL_BUFFER_MEMORY_REQUIREMENTS* pInStruct);
    xgl_buffer_memory_requirements_struct_wrapper(const XGL_BUFFER_MEMORY_REQUIREMENTS* pInStruct);

    virtual ~xgl_buffer_memory_requirements_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_FLAGS get_usage() { return m_struct.usage; }
    void set_usage(XGL_FLAGS inValue) { m_struct.usage = inValue; }


private:
    XGL_BUFFER_MEMORY_REQUIREMENTS m_struct;
    const XGL_BUFFER_MEMORY_REQUIREMENTS* m_origStructAddr;
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
    bool32_t get_blendEnable() { return m_struct.blendEnable; }
    void set_blendEnable(bool32_t inValue) { m_struct.blendEnable = inValue; }
    XGL_FORMAT get_format() { return m_struct.format; }
    void set_format(XGL_FORMAT inValue) { m_struct.format = inValue; }
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
    uint8_t get_channelWriteMask() { return m_struct.channelWriteMask; }
    void set_channelWriteMask(uint8_t inValue) { m_struct.channelWriteMask = inValue; }


private:
    XGL_PIPELINE_CB_ATTACHMENT_STATE m_struct;
    const XGL_PIPELINE_CB_ATTACHMENT_STATE* m_origStructAddr;
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
class xgl_buffer_view_create_info_struct_wrapper
{
public:
    xgl_buffer_view_create_info_struct_wrapper();
    xgl_buffer_view_create_info_struct_wrapper(XGL_BUFFER_VIEW_CREATE_INFO* pInStruct);
    xgl_buffer_view_create_info_struct_wrapper(const XGL_BUFFER_VIEW_CREATE_INFO* pInStruct);

    virtual ~xgl_buffer_view_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_BUFFER get_buffer() { return m_struct.buffer; }
    void set_buffer(XGL_BUFFER inValue) { m_struct.buffer = inValue; }
    XGL_BUFFER_VIEW_TYPE get_viewType() { return m_struct.viewType; }
    void set_viewType(XGL_BUFFER_VIEW_TYPE inValue) { m_struct.viewType = inValue; }
    XGL_GPU_SIZE get_stride() { return m_struct.stride; }
    void set_stride(XGL_GPU_SIZE inValue) { m_struct.stride = inValue; }
    XGL_FORMAT get_format() { return m_struct.format; }
    void set_format(XGL_FORMAT inValue) { m_struct.format = inValue; }
    XGL_CHANNEL_MAPPING get_channels() { return m_struct.channels; }
    void set_channels(XGL_CHANNEL_MAPPING inValue) { m_struct.channels = inValue; }
    XGL_GPU_SIZE get_offset() { return m_struct.offset; }
    void set_offset(XGL_GPU_SIZE inValue) { m_struct.offset = inValue; }
    XGL_GPU_SIZE get_range() { return m_struct.range; }
    void set_range(XGL_GPU_SIZE inValue) { m_struct.range = inValue; }


private:
    XGL_BUFFER_VIEW_CREATE_INFO m_struct;
    const XGL_BUFFER_VIEW_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_pipeline_barrier_struct_wrapper
{
public:
    xgl_pipeline_barrier_struct_wrapper();
    xgl_pipeline_barrier_struct_wrapper(XGL_PIPELINE_BARRIER* pInStruct);
    xgl_pipeline_barrier_struct_wrapper(const XGL_PIPELINE_BARRIER* pInStruct);

    virtual ~xgl_pipeline_barrier_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_eventCount() { return m_struct.eventCount; }
    void set_eventCount(uint32_t inValue) { m_struct.eventCount = inValue; }
    XGL_WAIT_EVENT get_waitEvent() { return m_struct.waitEvent; }
    void set_waitEvent(XGL_WAIT_EVENT inValue) { m_struct.waitEvent = inValue; }
    uint32_t get_memBarrierCount() { return m_struct.memBarrierCount; }
    void set_memBarrierCount(uint32_t inValue) { m_struct.memBarrierCount = inValue; }


private:
    XGL_PIPELINE_BARRIER m_struct;
    const XGL_PIPELINE_BARRIER* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_buffer_create_info_struct_wrapper
{
public:
    xgl_buffer_create_info_struct_wrapper();
    xgl_buffer_create_info_struct_wrapper(XGL_BUFFER_CREATE_INFO* pInStruct);
    xgl_buffer_create_info_struct_wrapper(const XGL_BUFFER_CREATE_INFO* pInStruct);

    virtual ~xgl_buffer_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_GPU_SIZE get_size() { return m_struct.size; }
    void set_size(XGL_GPU_SIZE inValue) { m_struct.size = inValue; }
    XGL_FLAGS get_usage() { return m_struct.usage; }
    void set_usage(XGL_FLAGS inValue) { m_struct.usage = inValue; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }


private:
    XGL_BUFFER_CREATE_INFO m_struct;
    const XGL_BUFFER_CREATE_INFO* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
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
    XGL_GPU_SIZE get_granularity() { return m_struct.granularity; }
    void set_granularity(XGL_GPU_SIZE inValue) { m_struct.granularity = inValue; }
    XGL_FLAGS get_memProps() { return m_struct.memProps; }
    void set_memProps(XGL_FLAGS inValue) { m_struct.memProps = inValue; }
    XGL_MEMORY_TYPE get_memType() { return m_struct.memType; }
    void set_memType(XGL_MEMORY_TYPE inValue) { m_struct.memType = inValue; }


private:
    XGL_MEMORY_REQUIREMENTS m_struct;
    const XGL_MEMORY_REQUIREMENTS* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_pipeline_vertex_input_create_info_struct_wrapper
{
public:
    xgl_pipeline_vertex_input_create_info_struct_wrapper();
    xgl_pipeline_vertex_input_create_info_struct_wrapper(XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* pInStruct);
    xgl_pipeline_vertex_input_create_info_struct_wrapper(const XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* pInStruct);

    virtual ~xgl_pipeline_vertex_input_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_bindingCount() { return m_struct.bindingCount; }
    void set_bindingCount(uint32_t inValue) { m_struct.bindingCount = inValue; }
    uint32_t get_attributeCount() { return m_struct.attributeCount; }
    void set_attributeCount(uint32_t inValue) { m_struct.attributeCount = inValue; }


private:
    XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO m_struct;
    const XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* m_origStructAddr;
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
class xgl_pipeline_ds_state_create_info_struct_wrapper
{
public:
    xgl_pipeline_ds_state_create_info_struct_wrapper();
    xgl_pipeline_ds_state_create_info_struct_wrapper(XGL_PIPELINE_DS_STATE_CREATE_INFO* pInStruct);
    xgl_pipeline_ds_state_create_info_struct_wrapper(const XGL_PIPELINE_DS_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_pipeline_ds_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_FORMAT get_format() { return m_struct.format; }
    void set_format(XGL_FORMAT inValue) { m_struct.format = inValue; }
    bool32_t get_depthTestEnable() { return m_struct.depthTestEnable; }
    void set_depthTestEnable(bool32_t inValue) { m_struct.depthTestEnable = inValue; }
    bool32_t get_depthWriteEnable() { return m_struct.depthWriteEnable; }
    void set_depthWriteEnable(bool32_t inValue) { m_struct.depthWriteEnable = inValue; }
    XGL_COMPARE_FUNC get_depthFunc() { return m_struct.depthFunc; }
    void set_depthFunc(XGL_COMPARE_FUNC inValue) { m_struct.depthFunc = inValue; }
    bool32_t get_depthBoundsEnable() { return m_struct.depthBoundsEnable; }
    void set_depthBoundsEnable(bool32_t inValue) { m_struct.depthBoundsEnable = inValue; }
    bool32_t get_stencilTestEnable() { return m_struct.stencilTestEnable; }
    void set_stencilTestEnable(bool32_t inValue) { m_struct.stencilTestEnable = inValue; }
    XGL_STENCIL_OP_STATE get_front() { return m_struct.front; }
    void set_front(XGL_STENCIL_OP_STATE inValue) { m_struct.front = inValue; }
    XGL_STENCIL_OP_STATE get_back() { return m_struct.back; }
    void set_back(XGL_STENCIL_OP_STATE inValue) { m_struct.back = inValue; }


private:
    XGL_PIPELINE_DS_STATE_CREATE_INFO m_struct;
    const XGL_PIPELINE_DS_STATE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_memory_alloc_buffer_info_struct_wrapper
{
public:
    xgl_memory_alloc_buffer_info_struct_wrapper();
    xgl_memory_alloc_buffer_info_struct_wrapper(XGL_MEMORY_ALLOC_BUFFER_INFO* pInStruct);
    xgl_memory_alloc_buffer_info_struct_wrapper(const XGL_MEMORY_ALLOC_BUFFER_INFO* pInStruct);

    virtual ~xgl_memory_alloc_buffer_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_FLAGS get_usage() { return m_struct.usage; }
    void set_usage(XGL_FLAGS inValue) { m_struct.usage = inValue; }


private:
    XGL_MEMORY_ALLOC_BUFFER_INFO m_struct;
    const XGL_MEMORY_ALLOC_BUFFER_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_vertex_input_attribute_description_struct_wrapper
{
public:
    xgl_vertex_input_attribute_description_struct_wrapper();
    xgl_vertex_input_attribute_description_struct_wrapper(XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pInStruct);
    xgl_vertex_input_attribute_description_struct_wrapper(const XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pInStruct);

    virtual ~xgl_vertex_input_attribute_description_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_binding() { return m_struct.binding; }
    void set_binding(uint32_t inValue) { m_struct.binding = inValue; }
    XGL_FORMAT get_format() { return m_struct.format; }
    void set_format(XGL_FORMAT inValue) { m_struct.format = inValue; }
    uint32_t get_offsetInBytes() { return m_struct.offsetInBytes; }
    void set_offsetInBytes(uint32_t inValue) { m_struct.offsetInBytes = inValue; }


private:
    XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION m_struct;
    const XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_patchControlPoints() { return m_struct.patchControlPoints; }
    void set_patchControlPoints(uint32_t inValue) { m_struct.patchControlPoints = inValue; }
    float get_optimalTessFactor() { return m_struct.optimalTessFactor; }
    void set_optimalTessFactor(float inValue) { m_struct.optimalTessFactor = inValue; }
    float get_fixedTessFactor() { return m_struct.fixedTessFactor; }
    void set_fixedTessFactor(float inValue) { m_struct.fixedTessFactor = inValue; }


private:
    XGL_PIPELINE_TESS_STATE_CREATE_INFO m_struct;
    const XGL_PIPELINE_TESS_STATE_CREATE_INFO* m_origStructAddr;
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
    uint32_t get_mipLevel() { return m_struct.mipLevel; }
    void set_mipLevel(uint32_t inValue) { m_struct.mipLevel = inValue; }
    uint32_t get_arraySlice() { return m_struct.arraySlice; }
    void set_arraySlice(uint32_t inValue) { m_struct.arraySlice = inValue; }


private:
    XGL_IMAGE_SUBRESOURCE m_struct;
    const XGL_IMAGE_SUBRESOURCE* m_origStructAddr;
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
    bool32_t get_supportsMigration() { return m_struct.supportsMigration; }
    void set_supportsMigration(bool32_t inValue) { m_struct.supportsMigration = inValue; }
    bool32_t get_supportsPinning() { return m_struct.supportsPinning; }
    void set_supportsPinning(bool32_t inValue) { m_struct.supportsPinning = inValue; }


private:
    XGL_PHYSICAL_GPU_MEMORY_PROPERTIES m_struct;
    const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
    XGL_IMAGE get_image() { return m_struct.image; }
    void set_image(XGL_IMAGE inValue) { m_struct.image = inValue; }
    uint32_t get_mipLevel() { return m_struct.mipLevel; }
    void set_mipLevel(uint32_t inValue) { m_struct.mipLevel = inValue; }
    uint32_t get_baseArraySlice() { return m_struct.baseArraySlice; }
    void set_baseArraySlice(uint32_t inValue) { m_struct.baseArraySlice = inValue; }
    uint32_t get_arraySize() { return m_struct.arraySize; }
    void set_arraySize(uint32_t inValue) { m_struct.arraySize = inValue; }
    XGL_IMAGE get_msaaResolveImage() { return m_struct.msaaResolveImage; }
    void set_msaaResolveImage(XGL_IMAGE inValue) { m_struct.msaaResolveImage = inValue; }
    XGL_IMAGE_SUBRESOURCE_RANGE get_msaaResolveSubResource() { return m_struct.msaaResolveSubResource; }
    void set_msaaResolveSubResource(XGL_IMAGE_SUBRESOURCE_RANGE inValue) { m_struct.msaaResolveSubResource = inValue; }
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
    uint32_t get_baseMipLevel() { return m_struct.baseMipLevel; }
    void set_baseMipLevel(uint32_t inValue) { m_struct.baseMipLevel = inValue; }
    uint32_t get_mipLevels() { return m_struct.mipLevels; }
    void set_mipLevels(uint32_t inValue) { m_struct.mipLevels = inValue; }
    uint32_t get_baseArraySlice() { return m_struct.baseArraySlice; }
    void set_baseArraySlice(uint32_t inValue) { m_struct.baseArraySlice = inValue; }
    uint32_t get_arraySize() { return m_struct.arraySize; }
    void set_arraySize(uint32_t inValue) { m_struct.arraySize = inValue; }


private:
    XGL_IMAGE_SUBRESOURCE_RANGE m_struct;
    const XGL_IMAGE_SUBRESOURCE_RANGE* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_buffer_image_copy_struct_wrapper
{
public:
    xgl_buffer_image_copy_struct_wrapper();
    xgl_buffer_image_copy_struct_wrapper(XGL_BUFFER_IMAGE_COPY* pInStruct);
    xgl_buffer_image_copy_struct_wrapper(const XGL_BUFFER_IMAGE_COPY* pInStruct);

    virtual ~xgl_buffer_image_copy_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_GPU_SIZE get_bufferOffset() { return m_struct.bufferOffset; }
    void set_bufferOffset(XGL_GPU_SIZE inValue) { m_struct.bufferOffset = inValue; }
    XGL_IMAGE_SUBRESOURCE get_imageSubresource() { return m_struct.imageSubresource; }
    void set_imageSubresource(XGL_IMAGE_SUBRESOURCE inValue) { m_struct.imageSubresource = inValue; }
    XGL_OFFSET3D get_imageOffset() { return m_struct.imageOffset; }
    void set_imageOffset(XGL_OFFSET3D inValue) { m_struct.imageOffset = inValue; }
    XGL_EXTENT3D get_imageExtent() { return m_struct.imageExtent; }
    void set_imageExtent(XGL_EXTENT3D inValue) { m_struct.imageExtent = inValue; }


private:
    XGL_BUFFER_IMAGE_COPY m_struct;
    const XGL_BUFFER_IMAGE_COPY* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_dynamic_cb_state_create_info_struct_wrapper
{
public:
    xgl_dynamic_cb_state_create_info_struct_wrapper();
    xgl_dynamic_cb_state_create_info_struct_wrapper(XGL_DYNAMIC_CB_STATE_CREATE_INFO* pInStruct);
    xgl_dynamic_cb_state_create_info_struct_wrapper(const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_dynamic_cb_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }


private:
    XGL_DYNAMIC_CB_STATE_CREATE_INFO m_struct;
    const XGL_DYNAMIC_CB_STATE_CREATE_INFO* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
    XGL_GPU_SIZE get_allocationSize() { return m_struct.allocationSize; }
    void set_allocationSize(XGL_GPU_SIZE inValue) { m_struct.allocationSize = inValue; }
    XGL_FLAGS get_memProps() { return m_struct.memProps; }
    void set_memProps(XGL_FLAGS inValue) { m_struct.memProps = inValue; }
    XGL_MEMORY_TYPE get_memType() { return m_struct.memType; }
    void set_memType(XGL_MEMORY_TYPE inValue) { m_struct.memType = inValue; }
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
class xgl_pipeline_cb_state_create_info_struct_wrapper
{
public:
    xgl_pipeline_cb_state_create_info_struct_wrapper();
    xgl_pipeline_cb_state_create_info_struct_wrapper(XGL_PIPELINE_CB_STATE_CREATE_INFO* pInStruct);
    xgl_pipeline_cb_state_create_info_struct_wrapper(const XGL_PIPELINE_CB_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_pipeline_cb_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    bool32_t get_alphaToCoverageEnable() { return m_struct.alphaToCoverageEnable; }
    void set_alphaToCoverageEnable(bool32_t inValue) { m_struct.alphaToCoverageEnable = inValue; }
    bool32_t get_logicOpEnable() { return m_struct.logicOpEnable; }
    void set_logicOpEnable(bool32_t inValue) { m_struct.logicOpEnable = inValue; }
    XGL_LOGIC_OP get_logicOp() { return m_struct.logicOp; }
    void set_logicOp(XGL_LOGIC_OP inValue) { m_struct.logicOp = inValue; }
    uint32_t get_attachmentCount() { return m_struct.attachmentCount; }
    void set_attachmentCount(uint32_t inValue) { m_struct.attachmentCount = inValue; }


private:
    XGL_PIPELINE_CB_STATE_CREATE_INFO m_struct;
    const XGL_PIPELINE_CB_STATE_CREATE_INFO* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
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
class xgl_cmd_buffer_graphics_begin_info_struct_wrapper
{
public:
    xgl_cmd_buffer_graphics_begin_info_struct_wrapper();
    xgl_cmd_buffer_graphics_begin_info_struct_wrapper(XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pInStruct);
    xgl_cmd_buffer_graphics_begin_info_struct_wrapper(const XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pInStruct);

    virtual ~xgl_cmd_buffer_graphics_begin_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_RENDER_PASS get_renderPass() { return m_struct.renderPass; }
    void set_renderPass(XGL_RENDER_PASS inValue) { m_struct.renderPass = inValue; }


private:
    XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO m_struct;
    const XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* m_origStructAddr;
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
    int32_t get_width() { return m_struct.width; }
    void set_width(int32_t inValue) { m_struct.width = inValue; }
    int32_t get_height() { return m_struct.height; }
    void set_height(int32_t inValue) { m_struct.height = inValue; }
    int32_t get_depth() { return m_struct.depth; }
    void set_depth(int32_t inValue) { m_struct.depth = inValue; }


private:
    XGL_EXTENT3D m_struct;
    const XGL_EXTENT3D* m_origStructAddr;
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
    XGL_FLAGS get_queueFlags() { return m_struct.queueFlags; }
    void set_queueFlags(XGL_FLAGS inValue) { m_struct.queueFlags = inValue; }
    uint32_t get_queueCount() { return m_struct.queueCount; }
    void set_queueCount(uint32_t inValue) { m_struct.queueCount = inValue; }
    uint32_t get_maxAtomicCounters() { return m_struct.maxAtomicCounters; }
    void set_maxAtomicCounters(uint32_t inValue) { m_struct.maxAtomicCounters = inValue; }
    bool32_t get_supportsTimestamps() { return m_struct.supportsTimestamps; }
    void set_supportsTimestamps(bool32_t inValue) { m_struct.supportsTimestamps = inValue; }


private:
    XGL_PHYSICAL_GPU_QUEUE_PROPERTIES m_struct;
    const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* m_origStructAddr;
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
    int32_t get_x() { return m_struct.x; }
    void set_x(int32_t inValue) { m_struct.x = inValue; }
    int32_t get_y() { return m_struct.y; }
    void set_y(int32_t inValue) { m_struct.y = inValue; }
    int32_t get_z() { return m_struct.z; }
    void set_z(int32_t inValue) { m_struct.z = inValue; }


private:
    XGL_OFFSET3D m_struct;
    const XGL_OFFSET3D* m_origStructAddr;
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
    int32_t get_x() { return m_struct.x; }
    void set_x(int32_t inValue) { m_struct.x = inValue; }
    int32_t get_y() { return m_struct.y; }
    void set_y(int32_t inValue) { m_struct.y = inValue; }


private:
    XGL_OFFSET2D m_struct;
    const XGL_OFFSET2D* m_origStructAddr;
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
    uint32_t get_x() { return m_struct.x; }
    void set_x(uint32_t inValue) { m_struct.x = inValue; }
    uint32_t get_y() { return m_struct.y; }
    void set_y(uint32_t inValue) { m_struct.y = inValue; }
    uint32_t get_z() { return m_struct.z; }
    void set_z(uint32_t inValue) { m_struct.z = inValue; }


private:
    XGL_DISPATCH_INDIRECT_CMD m_struct;
    const XGL_DISPATCH_INDIRECT_CMD* m_origStructAddr;
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
    float get_originX() { return m_struct.originX; }
    void set_originX(float inValue) { m_struct.originX = inValue; }
    float get_originY() { return m_struct.originY; }
    void set_originY(float inValue) { m_struct.originY = inValue; }
    float get_width() { return m_struct.width; }
    void set_width(float inValue) { m_struct.width = inValue; }
    float get_height() { return m_struct.height; }
    void set_height(float inValue) { m_struct.height = inValue; }
    float get_minDepth() { return m_struct.minDepth; }
    void set_minDepth(float inValue) { m_struct.minDepth = inValue; }
    float get_maxDepth() { return m_struct.maxDepth; }
    void set_maxDepth(float inValue) { m_struct.maxDepth = inValue; }


private:
    XGL_VIEWPORT m_struct;
    const XGL_VIEWPORT* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_descriptor_type_count_struct_wrapper
{
public:
    xgl_descriptor_type_count_struct_wrapper();
    xgl_descriptor_type_count_struct_wrapper(XGL_DESCRIPTOR_TYPE_COUNT* pInStruct);
    xgl_descriptor_type_count_struct_wrapper(const XGL_DESCRIPTOR_TYPE_COUNT* pInStruct);

    virtual ~xgl_descriptor_type_count_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_DESCRIPTOR_TYPE get_type() { return m_struct.type; }
    void set_type(XGL_DESCRIPTOR_TYPE inValue) { m_struct.type = inValue; }
    uint32_t get_count() { return m_struct.count; }
    void set_count(uint32_t inValue) { m_struct.count = inValue; }


private:
    XGL_DESCRIPTOR_TYPE_COUNT m_struct;
    const XGL_DESCRIPTOR_TYPE_COUNT* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_memory_alloc_image_info_struct_wrapper
{
public:
    xgl_memory_alloc_image_info_struct_wrapper();
    xgl_memory_alloc_image_info_struct_wrapper(XGL_MEMORY_ALLOC_IMAGE_INFO* pInStruct);
    xgl_memory_alloc_image_info_struct_wrapper(const XGL_MEMORY_ALLOC_IMAGE_INFO* pInStruct);

    virtual ~xgl_memory_alloc_image_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_FLAGS get_usage() { return m_struct.usage; }
    void set_usage(XGL_FLAGS inValue) { m_struct.usage = inValue; }
    XGL_IMAGE_FORMAT_CLASS get_formatClass() { return m_struct.formatClass; }
    void set_formatClass(XGL_IMAGE_FORMAT_CLASS inValue) { m_struct.formatClass = inValue; }
    uint32_t get_samples() { return m_struct.samples; }
    void set_samples(uint32_t inValue) { m_struct.samples = inValue; }


private:
    XGL_MEMORY_ALLOC_IMAGE_INFO m_struct;
    const XGL_MEMORY_ALLOC_IMAGE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_update_samplers_struct_wrapper
{
public:
    xgl_update_samplers_struct_wrapper();
    xgl_update_samplers_struct_wrapper(XGL_UPDATE_SAMPLERS* pInStruct);
    xgl_update_samplers_struct_wrapper(const XGL_UPDATE_SAMPLERS* pInStruct);

    virtual ~xgl_update_samplers_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_index() { return m_struct.index; }
    void set_index(uint32_t inValue) { m_struct.index = inValue; }
    uint32_t get_count() { return m_struct.count; }
    void set_count(uint32_t inValue) { m_struct.count = inValue; }


private:
    XGL_UPDATE_SAMPLERS m_struct;
    const XGL_UPDATE_SAMPLERS* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
    XGL_IMAGE_VIEW get_view() { return m_struct.view; }
    void set_view(XGL_IMAGE_VIEW inValue) { m_struct.view = inValue; }
    XGL_IMAGE_LAYOUT get_layout() { return m_struct.layout; }
    void set_layout(XGL_IMAGE_LAYOUT inValue) { m_struct.layout = inValue; }


private:
    XGL_IMAGE_VIEW_ATTACH_INFO m_struct;
    const XGL_IMAGE_VIEW_ATTACH_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_descriptor_region_create_info_struct_wrapper
{
public:
    xgl_descriptor_region_create_info_struct_wrapper();
    xgl_descriptor_region_create_info_struct_wrapper(XGL_DESCRIPTOR_REGION_CREATE_INFO* pInStruct);
    xgl_descriptor_region_create_info_struct_wrapper(const XGL_DESCRIPTOR_REGION_CREATE_INFO* pInStruct);

    virtual ~xgl_descriptor_region_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_count() { return m_struct.count; }
    void set_count(uint32_t inValue) { m_struct.count = inValue; }


private:
    XGL_DESCRIPTOR_REGION_CREATE_INFO m_struct;
    const XGL_DESCRIPTOR_REGION_CREATE_INFO* m_origStructAddr;
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
    uint32_t get_indexCount() { return m_struct.indexCount; }
    void set_indexCount(uint32_t inValue) { m_struct.indexCount = inValue; }
    uint32_t get_instanceCount() { return m_struct.instanceCount; }
    void set_instanceCount(uint32_t inValue) { m_struct.instanceCount = inValue; }
    uint32_t get_firstIndex() { return m_struct.firstIndex; }
    void set_firstIndex(uint32_t inValue) { m_struct.firstIndex = inValue; }
    int32_t get_vertexOffset() { return m_struct.vertexOffset; }
    void set_vertexOffset(int32_t inValue) { m_struct.vertexOffset = inValue; }
    uint32_t get_firstInstance() { return m_struct.firstInstance; }
    void set_firstInstance(uint32_t inValue) { m_struct.firstInstance = inValue; }


private:
    XGL_DRAW_INDEXED_INDIRECT_CMD m_struct;
    const XGL_DRAW_INDEXED_INDIRECT_CMD* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
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
    void* get_pUserData() { return m_struct.pUserData; }
    void set_pUserData(void* inValue) { m_struct.pUserData = inValue; }
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
class xgl_dynamic_ds_state_create_info_struct_wrapper
{
public:
    xgl_dynamic_ds_state_create_info_struct_wrapper();
    xgl_dynamic_ds_state_create_info_struct_wrapper(XGL_DYNAMIC_DS_STATE_CREATE_INFO* pInStruct);
    xgl_dynamic_ds_state_create_info_struct_wrapper(const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_dynamic_ds_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    float get_minDepth() { return m_struct.minDepth; }
    void set_minDepth(float inValue) { m_struct.minDepth = inValue; }
    float get_maxDepth() { return m_struct.maxDepth; }
    void set_maxDepth(float inValue) { m_struct.maxDepth = inValue; }
    uint32_t get_stencilReadMask() { return m_struct.stencilReadMask; }
    void set_stencilReadMask(uint32_t inValue) { m_struct.stencilReadMask = inValue; }
    uint32_t get_stencilWriteMask() { return m_struct.stencilWriteMask; }
    void set_stencilWriteMask(uint32_t inValue) { m_struct.stencilWriteMask = inValue; }
    uint32_t get_stencilFrontRef() { return m_struct.stencilFrontRef; }
    void set_stencilFrontRef(uint32_t inValue) { m_struct.stencilFrontRef = inValue; }
    uint32_t get_stencilBackRef() { return m_struct.stencilBackRef; }
    void set_stencilBackRef(uint32_t inValue) { m_struct.stencilBackRef = inValue; }


private:
    XGL_DYNAMIC_DS_STATE_CREATE_INFO m_struct;
    const XGL_DYNAMIC_DS_STATE_CREATE_INFO* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_initialCount() { return m_struct.initialCount; }
    void set_initialCount(uint32_t inValue) { m_struct.initialCount = inValue; }
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
    uint64_t get_fsInvocations() { return m_struct.fsInvocations; }
    void set_fsInvocations(uint64_t inValue) { m_struct.fsInvocations = inValue; }
    uint64_t get_cPrimitives() { return m_struct.cPrimitives; }
    void set_cPrimitives(uint64_t inValue) { m_struct.cPrimitives = inValue; }
    uint64_t get_cInvocations() { return m_struct.cInvocations; }
    void set_cInvocations(uint64_t inValue) { m_struct.cInvocations = inValue; }
    uint64_t get_vsInvocations() { return m_struct.vsInvocations; }
    void set_vsInvocations(uint64_t inValue) { m_struct.vsInvocations = inValue; }
    uint64_t get_gsInvocations() { return m_struct.gsInvocations; }
    void set_gsInvocations(uint64_t inValue) { m_struct.gsInvocations = inValue; }
    uint64_t get_gsPrimitives() { return m_struct.gsPrimitives; }
    void set_gsPrimitives(uint64_t inValue) { m_struct.gsPrimitives = inValue; }
    uint64_t get_iaPrimitives() { return m_struct.iaPrimitives; }
    void set_iaPrimitives(uint64_t inValue) { m_struct.iaPrimitives = inValue; }
    uint64_t get_iaVertices() { return m_struct.iaVertices; }
    void set_iaVertices(uint64_t inValue) { m_struct.iaVertices = inValue; }
    uint64_t get_tcsInvocations() { return m_struct.tcsInvocations; }
    void set_tcsInvocations(uint64_t inValue) { m_struct.tcsInvocations = inValue; }
    uint64_t get_tesInvocations() { return m_struct.tesInvocations; }
    void set_tesInvocations(uint64_t inValue) { m_struct.tesInvocations = inValue; }
    uint64_t get_csInvocations() { return m_struct.csInvocations; }
    void set_csInvocations(uint64_t inValue) { m_struct.csInvocations = inValue; }


private:
    XGL_PIPELINE_STATISTICS_DATA m_struct;
    const XGL_PIPELINE_STATISTICS_DATA* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
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
    float get_mipLodBias() { return m_struct.mipLodBias; }
    void set_mipLodBias(float inValue) { m_struct.mipLodBias = inValue; }
    uint32_t get_maxAnisotropy() { return m_struct.maxAnisotropy; }
    void set_maxAnisotropy(uint32_t inValue) { m_struct.maxAnisotropy = inValue; }
    XGL_COMPARE_FUNC get_compareFunc() { return m_struct.compareFunc; }
    void set_compareFunc(XGL_COMPARE_FUNC inValue) { m_struct.compareFunc = inValue; }
    float get_minLod() { return m_struct.minLod; }
    void set_minLod(float inValue) { m_struct.minLod = inValue; }
    float get_maxLod() { return m_struct.maxLod; }
    void set_maxLod(float inValue) { m_struct.maxLod = inValue; }
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
    float get_maxGpuClock() { return m_struct.maxGpuClock; }
    void set_maxGpuClock(float inValue) { m_struct.maxGpuClock = inValue; }
    float get_aluPerClock() { return m_struct.aluPerClock; }
    void set_aluPerClock(float inValue) { m_struct.aluPerClock = inValue; }
    float get_texPerClock() { return m_struct.texPerClock; }
    void set_texPerClock(float inValue) { m_struct.texPerClock = inValue; }
    float get_primsPerClock() { return m_struct.primsPerClock; }
    void set_primsPerClock(float inValue) { m_struct.primsPerClock = inValue; }
    float get_pixelsPerClock() { return m_struct.pixelsPerClock; }
    void set_pixelsPerClock(float inValue) { m_struct.pixelsPerClock = inValue; }


private:
    XGL_PHYSICAL_GPU_PERFORMANCE m_struct;
    const XGL_PHYSICAL_GPU_PERFORMANCE* m_origStructAddr;
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
class xgl_update_buffers_struct_wrapper
{
public:
    xgl_update_buffers_struct_wrapper();
    xgl_update_buffers_struct_wrapper(XGL_UPDATE_BUFFERS* pInStruct);
    xgl_update_buffers_struct_wrapper(const XGL_UPDATE_BUFFERS* pInStruct);

    virtual ~xgl_update_buffers_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_DESCRIPTOR_TYPE get_descriptorType() { return m_struct.descriptorType; }
    void set_descriptorType(XGL_DESCRIPTOR_TYPE inValue) { m_struct.descriptorType = inValue; }
    uint32_t get_index() { return m_struct.index; }
    void set_index(uint32_t inValue) { m_struct.index = inValue; }
    uint32_t get_count() { return m_struct.count; }
    void set_count(uint32_t inValue) { m_struct.count = inValue; }


private:
    XGL_UPDATE_BUFFERS m_struct;
    const XGL_UPDATE_BUFFERS* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_clear_color_struct_wrapper
{
public:
    xgl_clear_color_struct_wrapper();
    xgl_clear_color_struct_wrapper(XGL_CLEAR_COLOR* pInStruct);
    xgl_clear_color_struct_wrapper(const XGL_CLEAR_COLOR* pInStruct);

    virtual ~xgl_clear_color_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_CLEAR_COLOR_VALUE get_color() { return m_struct.color; }
    void set_color(XGL_CLEAR_COLOR_VALUE inValue) { m_struct.color = inValue; }
    bool32_t get_useRawValue() { return m_struct.useRawValue; }
    void set_useRawValue(bool32_t inValue) { m_struct.useRawValue = inValue; }


private:
    XGL_CLEAR_COLOR m_struct;
    const XGL_CLEAR_COLOR* m_origStructAddr;
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
class xgl_dynamic_rs_state_create_info_struct_wrapper
{
public:
    xgl_dynamic_rs_state_create_info_struct_wrapper();
    xgl_dynamic_rs_state_create_info_struct_wrapper(XGL_DYNAMIC_RS_STATE_CREATE_INFO* pInStruct);
    xgl_dynamic_rs_state_create_info_struct_wrapper(const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_dynamic_rs_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    float get_depthBias() { return m_struct.depthBias; }
    void set_depthBias(float inValue) { m_struct.depthBias = inValue; }
    float get_depthBiasClamp() { return m_struct.depthBiasClamp; }
    void set_depthBiasClamp(float inValue) { m_struct.depthBiasClamp = inValue; }
    float get_slopeScaledDepthBias() { return m_struct.slopeScaledDepthBias; }
    void set_slopeScaledDepthBias(float inValue) { m_struct.slopeScaledDepthBias = inValue; }
    float get_pointSize() { return m_struct.pointSize; }
    void set_pointSize(float inValue) { m_struct.pointSize = inValue; }
    float get_pointFadeThreshold() { return m_struct.pointFadeThreshold; }
    void set_pointFadeThreshold(float inValue) { m_struct.pointFadeThreshold = inValue; }
    float get_lineWidth() { return m_struct.lineWidth; }
    void set_lineWidth(float inValue) { m_struct.lineWidth = inValue; }


private:
    XGL_DYNAMIC_RS_STATE_CREATE_INFO m_struct;
    const XGL_DYNAMIC_RS_STATE_CREATE_INFO* m_origStructAddr;
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


private:
    XGL_STENCIL_OP_STATE m_struct;
    const XGL_STENCIL_OP_STATE* m_origStructAddr;
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
    XGL_IMAGE_LAYOUT get_layout() { return m_struct.layout; }
    void set_layout(XGL_IMAGE_LAYOUT inValue) { m_struct.layout = inValue; }


private:
    XGL_COLOR_ATTACHMENT_BIND_INFO m_struct;
    const XGL_COLOR_ATTACHMENT_BIND_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_cmd_buffer_begin_info_struct_wrapper
{
public:
    xgl_cmd_buffer_begin_info_struct_wrapper();
    xgl_cmd_buffer_begin_info_struct_wrapper(XGL_CMD_BUFFER_BEGIN_INFO* pInStruct);
    xgl_cmd_buffer_begin_info_struct_wrapper(const XGL_CMD_BUFFER_BEGIN_INFO* pInStruct);

    virtual ~xgl_cmd_buffer_begin_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }


private:
    XGL_CMD_BUFFER_BEGIN_INFO m_struct;
    const XGL_CMD_BUFFER_BEGIN_INFO* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
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
    const void* get_pNext() { return m_struct.pNext; }
    XGL_PRIMITIVE_TOPOLOGY get_topology() { return m_struct.topology; }
    void set_topology(XGL_PRIMITIVE_TOPOLOGY inValue) { m_struct.topology = inValue; }
    bool32_t get_disableVertexReuse() { return m_struct.disableVertexReuse; }
    void set_disableVertexReuse(bool32_t inValue) { m_struct.disableVertexReuse = inValue; }
    bool32_t get_primitiveRestartEnable() { return m_struct.primitiveRestartEnable; }
    void set_primitiveRestartEnable(bool32_t inValue) { m_struct.primitiveRestartEnable = inValue; }
    uint32_t get_primitiveRestartIndex() { return m_struct.primitiveRestartIndex; }
    void set_primitiveRestartIndex(uint32_t inValue) { m_struct.primitiveRestartIndex = inValue; }


private:
    XGL_PIPELINE_IA_STATE_CREATE_INFO m_struct;
    const XGL_PIPELINE_IA_STATE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_update_images_struct_wrapper
{
public:
    xgl_update_images_struct_wrapper();
    xgl_update_images_struct_wrapper(XGL_UPDATE_IMAGES* pInStruct);
    xgl_update_images_struct_wrapper(const XGL_UPDATE_IMAGES* pInStruct);

    virtual ~xgl_update_images_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_DESCRIPTOR_TYPE get_descriptorType() { return m_struct.descriptorType; }
    void set_descriptorType(XGL_DESCRIPTOR_TYPE inValue) { m_struct.descriptorType = inValue; }
    uint32_t get_index() { return m_struct.index; }
    void set_index(uint32_t inValue) { m_struct.index = inValue; }
    uint32_t get_count() { return m_struct.count; }
    void set_count(uint32_t inValue) { m_struct.count = inValue; }


private:
    XGL_UPDATE_IMAGES m_struct;
    const XGL_UPDATE_IMAGES* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
    const char* get_pAppName() { return m_struct.pAppName; }
    uint32_t get_appVersion() { return m_struct.appVersion; }
    void set_appVersion(uint32_t inValue) { m_struct.appVersion = inValue; }
    const char* get_pEngineName() { return m_struct.pEngineName; }
    uint32_t get_engineVersion() { return m_struct.engineVersion; }
    void set_engineVersion(uint32_t inValue) { m_struct.engineVersion = inValue; }
    uint32_t get_apiVersion() { return m_struct.apiVersion; }
    void set_apiVersion(uint32_t inValue) { m_struct.apiVersion = inValue; }


private:
    XGL_APPLICATION_INFO m_struct;
    const XGL_APPLICATION_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_descriptor_set_layout_create_info_struct_wrapper
{
public:
    xgl_descriptor_set_layout_create_info_struct_wrapper();
    xgl_descriptor_set_layout_create_info_struct_wrapper(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pInStruct);
    xgl_descriptor_set_layout_create_info_struct_wrapper(const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pInStruct);

    virtual ~xgl_descriptor_set_layout_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_DESCRIPTOR_TYPE get_descriptorType() { return m_struct.descriptorType; }
    void set_descriptorType(XGL_DESCRIPTOR_TYPE inValue) { m_struct.descriptorType = inValue; }
    uint32_t get_count() { return m_struct.count; }
    void set_count(uint32_t inValue) { m_struct.count = inValue; }
    XGL_FLAGS get_stageFlags() { return m_struct.stageFlags; }
    void set_stageFlags(XGL_FLAGS inValue) { m_struct.stageFlags = inValue; }
    XGL_SAMPLER get_immutableSampler() { return m_struct.immutableSampler; }
    void set_immutableSampler(XGL_SAMPLER inValue) { m_struct.immutableSampler = inValue; }


private:
    XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO m_struct;
    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
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
    const void* get_pNext() { return m_struct.pNext; }
    XGL_FLAGS get_flags() { return m_struct.flags; }
    void set_flags(XGL_FLAGS inValue) { m_struct.flags = inValue; }
    XGL_DESCRIPTOR_SET_LAYOUT get_lastSetLayout() { return m_struct.lastSetLayout; }
    void set_lastSetLayout(XGL_DESCRIPTOR_SET_LAYOUT inValue) { m_struct.lastSetLayout = inValue; }


private:
    XGL_GRAPHICS_PIPELINE_CREATE_INFO m_struct;
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* m_origStructAddr;
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
    int32_t get_width() { return m_struct.width; }
    void set_width(int32_t inValue) { m_struct.width = inValue; }
    int32_t get_height() { return m_struct.height; }
    void set_height(int32_t inValue) { m_struct.height = inValue; }


private:
    XGL_EXTENT2D m_struct;
    const XGL_EXTENT2D* m_origStructAddr;
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
    XGL_IMAGE_LAYOUT get_layout() { return m_struct.layout; }
    void set_layout(XGL_IMAGE_LAYOUT inValue) { m_struct.layout = inValue; }


private:
    XGL_DEPTH_STENCIL_BIND_INFO m_struct;
    const XGL_DEPTH_STENCIL_BIND_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_event_wait_info_struct_wrapper
{
public:
    xgl_event_wait_info_struct_wrapper();
    xgl_event_wait_info_struct_wrapper(XGL_EVENT_WAIT_INFO* pInStruct);
    xgl_event_wait_info_struct_wrapper(const XGL_EVENT_WAIT_INFO* pInStruct);

    virtual ~xgl_event_wait_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_eventCount() { return m_struct.eventCount; }
    void set_eventCount(uint32_t inValue) { m_struct.eventCount = inValue; }
    XGL_WAIT_EVENT get_waitEvent() { return m_struct.waitEvent; }
    void set_waitEvent(XGL_WAIT_EVENT inValue) { m_struct.waitEvent = inValue; }
    uint32_t get_memBarrierCount() { return m_struct.memBarrierCount; }
    void set_memBarrierCount(uint32_t inValue) { m_struct.memBarrierCount = inValue; }


private:
    XGL_EVENT_WAIT_INFO m_struct;
    const XGL_EVENT_WAIT_INFO* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
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
class xgl_dynamic_vp_state_create_info_struct_wrapper
{
public:
    xgl_dynamic_vp_state_create_info_struct_wrapper();
    xgl_dynamic_vp_state_create_info_struct_wrapper(XGL_DYNAMIC_VP_STATE_CREATE_INFO* pInStruct);
    xgl_dynamic_vp_state_create_info_struct_wrapper(const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_dynamic_vp_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_viewportAndScissorCount() { return m_struct.viewportAndScissorCount; }
    void set_viewportAndScissorCount(uint32_t inValue) { m_struct.viewportAndScissorCount = inValue; }


private:
    XGL_DYNAMIC_VP_STATE_CREATE_INFO m_struct;
    const XGL_DYNAMIC_VP_STATE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_render_pass_create_info_struct_wrapper
{
public:
    xgl_render_pass_create_info_struct_wrapper();
    xgl_render_pass_create_info_struct_wrapper(XGL_RENDER_PASS_CREATE_INFO* pInStruct);
    xgl_render_pass_create_info_struct_wrapper(const XGL_RENDER_PASS_CREATE_INFO* pInStruct);

    virtual ~xgl_render_pass_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_RECT get_renderArea() { return m_struct.renderArea; }
    void set_renderArea(XGL_RECT inValue) { m_struct.renderArea = inValue; }
    XGL_FRAMEBUFFER get_framebuffer() { return m_struct.framebuffer; }
    void set_framebuffer(XGL_FRAMEBUFFER inValue) { m_struct.framebuffer = inValue; }
    uint32_t get_colorAttachmentCount() { return m_struct.colorAttachmentCount; }
    void set_colorAttachmentCount(uint32_t inValue) { m_struct.colorAttachmentCount = inValue; }
    XGL_ATTACHMENT_LOAD_OP get_depthLoadOp() { return m_struct.depthLoadOp; }
    void set_depthLoadOp(XGL_ATTACHMENT_LOAD_OP inValue) { m_struct.depthLoadOp = inValue; }
    float get_depthLoadClearValue() { return m_struct.depthLoadClearValue; }
    void set_depthLoadClearValue(float inValue) { m_struct.depthLoadClearValue = inValue; }
    XGL_ATTACHMENT_STORE_OP get_depthStoreOp() { return m_struct.depthStoreOp; }
    void set_depthStoreOp(XGL_ATTACHMENT_STORE_OP inValue) { m_struct.depthStoreOp = inValue; }
    XGL_ATTACHMENT_LOAD_OP get_stencilLoadOp() { return m_struct.stencilLoadOp; }
    void set_stencilLoadOp(XGL_ATTACHMENT_LOAD_OP inValue) { m_struct.stencilLoadOp = inValue; }
    uint32_t get_stencilLoadClearValue() { return m_struct.stencilLoadClearValue; }
    void set_stencilLoadClearValue(uint32_t inValue) { m_struct.stencilLoadClearValue = inValue; }
    XGL_ATTACHMENT_STORE_OP get_stencilStoreOp() { return m_struct.stencilStoreOp; }
    void set_stencilStoreOp(XGL_ATTACHMENT_STORE_OP inValue) { m_struct.stencilStoreOp = inValue; }


private:
    XGL_RENDER_PASS_CREATE_INFO m_struct;
    const XGL_RENDER_PASS_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_buffer_memory_barrier_struct_wrapper
{
public:
    xgl_buffer_memory_barrier_struct_wrapper();
    xgl_buffer_memory_barrier_struct_wrapper(XGL_BUFFER_MEMORY_BARRIER* pInStruct);
    xgl_buffer_memory_barrier_struct_wrapper(const XGL_BUFFER_MEMORY_BARRIER* pInStruct);

    virtual ~xgl_buffer_memory_barrier_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_FLAGS get_outputMask() { return m_struct.outputMask; }
    void set_outputMask(XGL_FLAGS inValue) { m_struct.outputMask = inValue; }
    XGL_FLAGS get_inputMask() { return m_struct.inputMask; }
    void set_inputMask(XGL_FLAGS inValue) { m_struct.inputMask = inValue; }
    XGL_BUFFER get_buffer() { return m_struct.buffer; }
    void set_buffer(XGL_BUFFER inValue) { m_struct.buffer = inValue; }
    XGL_GPU_SIZE get_offset() { return m_struct.offset; }
    void set_offset(XGL_GPU_SIZE inValue) { m_struct.offset = inValue; }
    XGL_GPU_SIZE get_size() { return m_struct.size; }
    void set_size(XGL_GPU_SIZE inValue) { m_struct.size = inValue; }


private:
    XGL_BUFFER_MEMORY_BARRIER m_struct;
    const XGL_BUFFER_MEMORY_BARRIER* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_pipeline_vp_state_create_info_struct_wrapper
{
public:
    xgl_pipeline_vp_state_create_info_struct_wrapper();
    xgl_pipeline_vp_state_create_info_struct_wrapper(XGL_PIPELINE_VP_STATE_CREATE_INFO* pInStruct);
    xgl_pipeline_vp_state_create_info_struct_wrapper(const XGL_PIPELINE_VP_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_pipeline_vp_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_numViewports() { return m_struct.numViewports; }
    void set_numViewports(uint32_t inValue) { m_struct.numViewports = inValue; }
    XGL_COORDINATE_ORIGIN get_clipOrigin() { return m_struct.clipOrigin; }
    void set_clipOrigin(XGL_COORDINATE_ORIGIN inValue) { m_struct.clipOrigin = inValue; }
    XGL_DEPTH_MODE get_depthMode() { return m_struct.depthMode; }
    void set_depthMode(XGL_DEPTH_MODE inValue) { m_struct.depthMode = inValue; }


private:
    XGL_PIPELINE_VP_STATE_CREATE_INFO m_struct;
    const XGL_PIPELINE_VP_STATE_CREATE_INFO* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_pipeline_ms_state_create_info_struct_wrapper
{
public:
    xgl_pipeline_ms_state_create_info_struct_wrapper();
    xgl_pipeline_ms_state_create_info_struct_wrapper(XGL_PIPELINE_MS_STATE_CREATE_INFO* pInStruct);
    xgl_pipeline_ms_state_create_info_struct_wrapper(const XGL_PIPELINE_MS_STATE_CREATE_INFO* pInStruct);

    virtual ~xgl_pipeline_ms_state_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_samples() { return m_struct.samples; }
    void set_samples(uint32_t inValue) { m_struct.samples = inValue; }
    bool32_t get_multisampleEnable() { return m_struct.multisampleEnable; }
    void set_multisampleEnable(bool32_t inValue) { m_struct.multisampleEnable = inValue; }
    bool32_t get_sampleShadingEnable() { return m_struct.sampleShadingEnable; }
    void set_sampleShadingEnable(bool32_t inValue) { m_struct.sampleShadingEnable = inValue; }
    float get_minSampleShading() { return m_struct.minSampleShading; }
    void set_minSampleShading(float inValue) { m_struct.minSampleShading = inValue; }
    XGL_SAMPLE_MASK get_sampleMask() { return m_struct.sampleMask; }
    void set_sampleMask(XGL_SAMPLE_MASK inValue) { m_struct.sampleMask = inValue; }


private:
    XGL_PIPELINE_MS_STATE_CREATE_INFO m_struct;
    const XGL_PIPELINE_MS_STATE_CREATE_INFO* m_origStructAddr;
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
    uint32_t get_linkConstBufferCount() { return m_struct.linkConstBufferCount; }
    void set_linkConstBufferCount(uint32_t inValue) { m_struct.linkConstBufferCount = inValue; }


private:
    XGL_PIPELINE_SHADER m_struct;
    const XGL_PIPELINE_SHADER* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_update_as_copy_struct_wrapper
{
public:
    xgl_update_as_copy_struct_wrapper();
    xgl_update_as_copy_struct_wrapper(XGL_UPDATE_AS_COPY* pInStruct);
    xgl_update_as_copy_struct_wrapper(const XGL_UPDATE_AS_COPY* pInStruct);

    virtual ~xgl_update_as_copy_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    XGL_DESCRIPTOR_TYPE get_descriptorType() { return m_struct.descriptorType; }
    void set_descriptorType(XGL_DESCRIPTOR_TYPE inValue) { m_struct.descriptorType = inValue; }
    XGL_DESCRIPTOR_SET get_descriptorSet() { return m_struct.descriptorSet; }
    void set_descriptorSet(XGL_DESCRIPTOR_SET inValue) { m_struct.descriptorSet = inValue; }
    uint32_t get_descriptorIndex() { return m_struct.descriptorIndex; }
    void set_descriptorIndex(uint32_t inValue) { m_struct.descriptorIndex = inValue; }
    uint32_t get_count() { return m_struct.count; }
    void set_count(uint32_t inValue) { m_struct.count = inValue; }


private:
    XGL_UPDATE_AS_COPY m_struct;
    const XGL_UPDATE_AS_COPY* m_origStructAddr;
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
    uint32_t get_vertexCount() { return m_struct.vertexCount; }
    void set_vertexCount(uint32_t inValue) { m_struct.vertexCount = inValue; }
    uint32_t get_instanceCount() { return m_struct.instanceCount; }
    void set_instanceCount(uint32_t inValue) { m_struct.instanceCount = inValue; }
    uint32_t get_firstVertex() { return m_struct.firstVertex; }
    void set_firstVertex(uint32_t inValue) { m_struct.firstVertex = inValue; }
    uint32_t get_firstInstance() { return m_struct.firstInstance; }
    void set_firstInstance(uint32_t inValue) { m_struct.firstInstance = inValue; }


private:
    XGL_DRAW_INDIRECT_CMD m_struct;
    const XGL_DRAW_INDIRECT_CMD* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_queueRecordCount() { return m_struct.queueRecordCount; }
    void set_queueRecordCount(uint32_t inValue) { m_struct.queueRecordCount = inValue; }
    uint32_t get_extensionCount() { return m_struct.extensionCount; }
    void set_extensionCount(uint32_t inValue) { m_struct.extensionCount = inValue; }
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
class xgl_sampler_image_view_info_struct_wrapper
{
public:
    xgl_sampler_image_view_info_struct_wrapper();
    xgl_sampler_image_view_info_struct_wrapper(XGL_SAMPLER_IMAGE_VIEW_INFO* pInStruct);
    xgl_sampler_image_view_info_struct_wrapper(const XGL_SAMPLER_IMAGE_VIEW_INFO* pInStruct);

    virtual ~xgl_sampler_image_view_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_SAMPLER get_pSampler() { return m_struct.pSampler; }
    void set_pSampler(XGL_SAMPLER inValue) { m_struct.pSampler = inValue; }
    const XGL_IMAGE_VIEW_ATTACH_INFO* get_pImageView() { return m_struct.pImageView; }


private:
    XGL_SAMPLER_IMAGE_VIEW_INFO m_struct;
    const XGL_SAMPLER_IMAGE_VIEW_INFO* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
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
class xgl_vertex_input_binding_description_struct_wrapper
{
public:
    xgl_vertex_input_binding_description_struct_wrapper();
    xgl_vertex_input_binding_description_struct_wrapper(XGL_VERTEX_INPUT_BINDING_DESCRIPTION* pInStruct);
    xgl_vertex_input_binding_description_struct_wrapper(const XGL_VERTEX_INPUT_BINDING_DESCRIPTION* pInStruct);

    virtual ~xgl_vertex_input_binding_description_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    uint32_t get_strideInBytes() { return m_struct.strideInBytes; }
    void set_strideInBytes(uint32_t inValue) { m_struct.strideInBytes = inValue; }
    XGL_VERTEX_INPUT_STEP_RATE get_stepRate() { return m_struct.stepRate; }
    void set_stepRate(XGL_VERTEX_INPUT_STEP_RATE inValue) { m_struct.stepRate = inValue; }


private:
    XGL_VERTEX_INPUT_BINDING_DESCRIPTION m_struct;
    const XGL_VERTEX_INPUT_BINDING_DESCRIPTION* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
    size_t get_codeSize() { return m_struct.codeSize; }
    void set_codeSize(size_t inValue) { m_struct.codeSize = inValue; }
    const void* get_pCode() { return m_struct.pCode; }
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
class xgl_buffer_copy_struct_wrapper
{
public:
    xgl_buffer_copy_struct_wrapper();
    xgl_buffer_copy_struct_wrapper(XGL_BUFFER_COPY* pInStruct);
    xgl_buffer_copy_struct_wrapper(const XGL_BUFFER_COPY* pInStruct);

    virtual ~xgl_buffer_copy_struct_wrapper();

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
    XGL_BUFFER_COPY m_struct;
    const XGL_BUFFER_COPY* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};


//class declaration
class xgl_framebuffer_create_info_struct_wrapper
{
public:
    xgl_framebuffer_create_info_struct_wrapper();
    xgl_framebuffer_create_info_struct_wrapper(XGL_FRAMEBUFFER_CREATE_INFO* pInStruct);
    xgl_framebuffer_create_info_struct_wrapper(const XGL_FRAMEBUFFER_CREATE_INFO* pInStruct);

    virtual ~xgl_framebuffer_create_info_struct_wrapper();

    void display_txt();
    void display_single_txt();
    void display_full_txt();

    void set_indent(uint32_t indent) { m_indent = indent; }
    XGL_STRUCTURE_TYPE get_sType() { return m_struct.sType; }
    void set_sType(XGL_STRUCTURE_TYPE inValue) { m_struct.sType = inValue; }
    const void* get_pNext() { return m_struct.pNext; }
    uint32_t get_colorAttachmentCount() { return m_struct.colorAttachmentCount; }
    void set_colorAttachmentCount(uint32_t inValue) { m_struct.colorAttachmentCount = inValue; }
    const XGL_DEPTH_STENCIL_BIND_INFO* get_pDepthStencilAttachment() { return m_struct.pDepthStencilAttachment; }
    uint32_t get_sampleCount() { return m_struct.sampleCount; }
    void set_sampleCount(uint32_t inValue) { m_struct.sampleCount = inValue; }
    uint32_t get_width() { return m_struct.width; }
    void set_width(uint32_t inValue) { m_struct.width = inValue; }
    uint32_t get_height() { return m_struct.height; }
    void set_height(uint32_t inValue) { m_struct.height = inValue; }
    uint32_t get_layers() { return m_struct.layers; }
    void set_layers(uint32_t inValue) { m_struct.layers = inValue; }


private:
    XGL_FRAMEBUFFER_CREATE_INFO m_struct;
    const XGL_FRAMEBUFFER_CREATE_INFO* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
    XGL_QUERY_TYPE get_queryType() { return m_struct.queryType; }
    void set_queryType(XGL_QUERY_TYPE inValue) { m_struct.queryType = inValue; }
    uint32_t get_slots() { return m_struct.slots; }
    void set_slots(uint32_t inValue) { m_struct.slots = inValue; }


private:
    XGL_QUERY_POOL_CREATE_INFO m_struct;
    const XGL_QUERY_POOL_CREATE_INFO* m_origStructAddr;
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
    const void* get_pNext() { return m_struct.pNext; }
    XGL_IMAGE get_image() { return m_struct.image; }
    void set_image(XGL_IMAGE inValue) { m_struct.image = inValue; }
    XGL_FORMAT get_format() { return m_struct.format; }
    void set_format(XGL_FORMAT inValue) { m_struct.format = inValue; }
    uint32_t get_mipLevel() { return m_struct.mipLevel; }
    void set_mipLevel(uint32_t inValue) { m_struct.mipLevel = inValue; }
    uint32_t get_baseArraySlice() { return m_struct.baseArraySlice; }
    void set_baseArraySlice(uint32_t inValue) { m_struct.baseArraySlice = inValue; }
    uint32_t get_arraySize() { return m_struct.arraySize; }
    void set_arraySize(uint32_t inValue) { m_struct.arraySize = inValue; }
    XGL_IMAGE get_msaaResolveImage() { return m_struct.msaaResolveImage; }
    void set_msaaResolveImage(XGL_IMAGE inValue) { m_struct.msaaResolveImage = inValue; }
    XGL_IMAGE_SUBRESOURCE_RANGE get_msaaResolveSubResource() { return m_struct.msaaResolveSubResource; }
    void set_msaaResolveSubResource(XGL_IMAGE_SUBRESOURCE_RANGE inValue) { m_struct.msaaResolveSubResource = inValue; }


private:
    XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO m_struct;
    const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* m_origStructAddr;
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
    uint32_t get_bufferId() { return m_struct.bufferId; }
    void set_bufferId(uint32_t inValue) { m_struct.bufferId = inValue; }
    size_t get_bufferSize() { return m_struct.bufferSize; }
    void set_bufferSize(size_t inValue) { m_struct.bufferSize = inValue; }
    const void* get_pBufferData() { return m_struct.pBufferData; }


private:
    XGL_LINK_CONST_BUFFER m_struct;
    const XGL_LINK_CONST_BUFFER* m_origStructAddr;
    uint32_t m_indent;
    const char m_dummy_prefix;
    void display_struct_members();

};

//any footer info for class
