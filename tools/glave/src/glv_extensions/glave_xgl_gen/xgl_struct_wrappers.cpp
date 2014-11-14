//This is the copyright
//#includes, #defines, globals and such...
#include <stdio.h>
#include <xgl_struct_wrappers.h>
#include <xgl_string_helper.h>

void dynamic_display_full_txt(const XGL_VOID* pStruct, uint32_t indent)
{
    // Cast to APP_INFO ptr initially just to pull sType off struct
    XGL_STRUCTURE_TYPE sType = ((XGL_APPLICATION_INFO*)pStruct)->sType;    switch (sType)
    {
        case XGL_STRUCTURE_TYPE_APPLICATION_INFO:
        {
            xgl_application_info_struct_wrapper swc0((XGL_APPLICATION_INFO*)pStruct);
            swc0.set_indent(indent);
            swc0.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO:
        {
            xgl_cmd_buffer_create_info_struct_wrapper swc1((XGL_CMD_BUFFER_CREATE_INFO*)pStruct);
            swc1.set_indent(indent);
            swc1.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO:
        {
            xgl_color_attachment_view_create_info_struct_wrapper swc2((XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO*)pStruct);
            swc2.set_indent(indent);
            swc2.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_COLOR_BLEND_STATE_CREATE_INFO:
        {
            xgl_color_blend_state_create_info_struct_wrapper swc3((XGL_COLOR_BLEND_STATE_CREATE_INFO*)pStruct);
            swc3.set_indent(indent);
            swc3.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
        {
            xgl_compute_pipeline_create_info_struct_wrapper swc4((XGL_COMPUTE_PIPELINE_CREATE_INFO*)pStruct);
            swc4.set_indent(indent);
            swc4.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_DEPTH_STENCIL_STATE_CREATE_INFO:
        {
            xgl_depth_stencil_state_create_info_struct_wrapper swc5((XGL_DEPTH_STENCIL_STATE_CREATE_INFO*)pStruct);
            swc5.set_indent(indent);
            swc5.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO:
        {
            xgl_depth_stencil_view_create_info_struct_wrapper swc6((XGL_DEPTH_STENCIL_VIEW_CREATE_INFO*)pStruct);
            swc6.set_indent(indent);
            swc6.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO:
        {
            xgl_descriptor_set_create_info_struct_wrapper swc7((XGL_DESCRIPTOR_SET_CREATE_INFO*)pStruct);
            swc7.set_indent(indent);
            swc7.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO:
        {
            xgl_device_create_info_struct_wrapper swc8((XGL_DEVICE_CREATE_INFO*)pStruct);
            swc8.set_indent(indent);
            swc8.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO:
        {
            xgl_event_create_info_struct_wrapper swc9((XGL_EVENT_CREATE_INFO*)pStruct);
            swc9.set_indent(indent);
            swc9.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO:
        {
            xgl_fence_create_info_struct_wrapper swc10((XGL_FENCE_CREATE_INFO*)pStruct);
            swc10.set_indent(indent);
            swc10.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
        {
            xgl_graphics_pipeline_create_info_struct_wrapper swc11((XGL_GRAPHICS_PIPELINE_CREATE_INFO*)pStruct);
            swc11.set_indent(indent);
            swc11.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
        {
            xgl_image_create_info_struct_wrapper swc12((XGL_IMAGE_CREATE_INFO*)pStruct);
            swc12.set_indent(indent);
            swc12.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO:
        {
            xgl_image_view_attach_info_struct_wrapper swc13((XGL_IMAGE_VIEW_ATTACH_INFO*)pStruct);
            swc13.set_indent(indent);
            swc13.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO:
        {
            xgl_image_view_create_info_struct_wrapper swc14((XGL_IMAGE_VIEW_CREATE_INFO*)pStruct);
            swc14.set_indent(indent);
            swc14.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO:
        {
            xgl_memory_alloc_info_struct_wrapper swc15((XGL_MEMORY_ALLOC_INFO*)pStruct);
            swc15.set_indent(indent);
            swc15.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_MEMORY_OPEN_INFO:
        {
            xgl_memory_open_info_struct_wrapper swc16((XGL_MEMORY_OPEN_INFO*)pStruct);
            swc16.set_indent(indent);
            swc16.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_MEMORY_STATE_TRANSITION:
        {
            xgl_memory_state_transition_struct_wrapper swc17((XGL_MEMORY_STATE_TRANSITION*)pStruct);
            swc17.set_indent(indent);
            swc17.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO:
        {
            xgl_memory_view_attach_info_struct_wrapper swc18((XGL_MEMORY_VIEW_ATTACH_INFO*)pStruct);
            swc18.set_indent(indent);
            swc18.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_MSAA_STATE_CREATE_INFO:
        {
            xgl_msaa_state_create_info_struct_wrapper swc19((XGL_MSAA_STATE_CREATE_INFO*)pStruct);
            swc19.set_indent(indent);
            swc19.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO:
        {
            xgl_peer_memory_open_info_struct_wrapper swc20((XGL_PEER_MEMORY_OPEN_INFO*)pStruct);
            swc20.set_indent(indent);
            swc20.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
        {
            xgl_pipeline_cb_state_create_info_struct_wrapper swc21((XGL_PIPELINE_CB_STATE*)pStruct);
            swc21.set_indent(indent);
            swc21.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO:
        {
            xgl_pipeline_db_state_create_info_struct_wrapper swc22((XGL_PIPELINE_DB_STATE_CREATE_INFO*)pStruct);
            swc22.set_indent(indent);
            swc22.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
        {
            xgl_pipeline_ia_state_create_info_struct_wrapper swc23((XGL_PIPELINE_IA_STATE_CREATE_INFO*)pStruct);
            swc23.set_indent(indent);
            swc23.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
        {
            xgl_pipeline_rs_state_create_info_struct_wrapper swc24((XGL_PIPELINE_RS_STATE_CREATE_INFO*)pStruct);
            swc24.set_indent(indent);
            swc24.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
        {
            xgl_pipeline_shader_stage_create_info_struct_wrapper swc25((XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pStruct);
            swc25.set_indent(indent);
            swc25.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
        {
            xgl_pipeline_tess_state_create_info_struct_wrapper swc26((XGL_PIPELINE_TESS_STATE_CREATE_INFO*)pStruct);
            swc26.set_indent(indent);
            swc26.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
        {
            xgl_query_pool_create_info_struct_wrapper swc27((XGL_QUERY_POOL_CREATE_INFO*)pStruct);
            swc27.set_indent(indent);
            swc27.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_RASTER_STATE_CREATE_INFO:
        {
            xgl_raster_state_create_info_struct_wrapper swc28((XGL_RASTER_STATE_CREATE_INFO*)pStruct);
            swc28.set_indent(indent);
            swc28.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO:
        {
            xgl_sampler_create_info_struct_wrapper swc29((XGL_SAMPLER_CREATE_INFO*)pStruct);
            swc29.set_indent(indent);
            swc29.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO:
        {
            xgl_queue_semaphore_create_info_struct_wrapper swc30((XGL_QUEUE_SEMAPHORE_CREATE_INFO*)pStruct);
            swc30.set_indent(indent);
            swc30.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_SEMAPHORE_OPEN_INFO:
        {
            xgl_queue_semaphore_open_info_struct_wrapper swc31((XGL_QUEUE_SEMAPHORE_OPEN_INFO*)pStruct);
            swc31.set_indent(indent);
            swc31.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO:
        {
            xgl_shader_create_info_struct_wrapper swc32((XGL_SHADER_CREATE_INFO*)pStruct);
            swc32.set_indent(indent);
            swc32.display_full_txt();
        }
        break;
    }
}


// xgl_raster_state_create_info_struct_wrapper class definition
xgl_raster_state_create_info_struct_wrapper::xgl_raster_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_raster_state_create_info_struct_wrapper::xgl_raster_state_create_info_struct_wrapper(XGL_RASTER_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_raster_state_create_info_struct_wrapper::xgl_raster_state_create_info_struct_wrapper(const XGL_RASTER_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_raster_state_create_info_struct_wrapper::~xgl_raster_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_raster_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_RASTER_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_raster_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sfillMode = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_FILL_MODE(m_struct.fillMode));
    printf("%*s    %scullMode = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_CULL_MODE(m_struct.cullMode));
    printf("%*s    %sfrontFace = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_FACE_ORIENTATION(m_struct.frontFace));
    printf("%*s    %sdepthBias = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.depthBias));
    printf("%*s    %sdepthBiasClamp = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.depthBiasClamp));
    printf("%*s    %sslopeScaledDepthBias = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.slopeScaledDepthBias));
}

// Output all struct elements, each on their own line
void xgl_raster_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_RASTER_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_raster_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_RASTER_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_gpu_compatibility_info_struct_wrapper class definition
xgl_gpu_compatibility_info_struct_wrapper::xgl_gpu_compatibility_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_gpu_compatibility_info_struct_wrapper::xgl_gpu_compatibility_info_struct_wrapper(XGL_GPU_COMPATIBILITY_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_gpu_compatibility_info_struct_wrapper::xgl_gpu_compatibility_info_struct_wrapper(const XGL_GPU_COMPATIBILITY_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_gpu_compatibility_info_struct_wrapper::~xgl_gpu_compatibility_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_gpu_compatibility_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_GPU_COMPATIBILITY_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_gpu_compatibility_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %scompatibilityFlags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.compatibilityFlags));
}

// Output all struct elements, each on their own line
void xgl_gpu_compatibility_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_GPU_COMPATIBILITY_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_gpu_compatibility_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_GPU_COMPATIBILITY_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_image_view_create_info_struct_wrapper class definition
xgl_image_view_create_info_struct_wrapper::xgl_image_view_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_image_view_create_info_struct_wrapper::xgl_image_view_create_info_struct_wrapper(XGL_IMAGE_VIEW_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_view_create_info_struct_wrapper::xgl_image_view_create_info_struct_wrapper(const XGL_IMAGE_VIEW_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_view_create_info_struct_wrapper::~xgl_image_view_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_image_view_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_IMAGE_VIEW_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_image_view_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %simage = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.image));
    printf("%*s    %sviewType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_VIEW_TYPE(m_struct.viewType));
    printf("%*s    %sformat = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.format));
    printf("%*s    %schannels = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.channels));
    printf("%*s    %ssubresourceRange = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.subresourceRange));
    printf("%*s    %sminLod = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.minLod));
}

// Output all struct elements, each on their own line
void xgl_image_view_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_IMAGE_VIEW_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_image_view_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_IMAGE_VIEW_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.subresourceRange) {
        xgl_image_subresource_range_struct_wrapper class0(&m_struct.subresourceRange);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.channels) {
        xgl_channel_mapping_struct_wrapper class1(&m_struct.channels);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (&m_struct.format) {
        xgl_format_struct_wrapper class2(&m_struct.format);
        class2.set_indent(m_indent + 4);
        class2.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_memory_open_info_struct_wrapper class definition
xgl_memory_open_info_struct_wrapper::xgl_memory_open_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_memory_open_info_struct_wrapper::xgl_memory_open_info_struct_wrapper(XGL_MEMORY_OPEN_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_open_info_struct_wrapper::xgl_memory_open_info_struct_wrapper(const XGL_MEMORY_OPEN_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_open_info_struct_wrapper::~xgl_memory_open_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_memory_open_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_MEMORY_OPEN_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_memory_open_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %ssharedMem = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.sharedMem));
}

// Output all struct elements, each on their own line
void xgl_memory_open_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_MEMORY_OPEN_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_memory_open_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_MEMORY_OPEN_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_memory_heap_properties_struct_wrapper class definition
xgl_memory_heap_properties_struct_wrapper::xgl_memory_heap_properties_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_memory_heap_properties_struct_wrapper::xgl_memory_heap_properties_struct_wrapper(XGL_MEMORY_HEAP_PROPERTIES* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_heap_properties_struct_wrapper::xgl_memory_heap_properties_struct_wrapper(const XGL_MEMORY_HEAP_PROPERTIES* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_heap_properties_struct_wrapper::~xgl_memory_heap_properties_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_memory_heap_properties_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_MEMORY_HEAP_PROPERTIES = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_memory_heap_properties_struct_wrapper::display_struct_members()
{
    printf("%*s    %sstructSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.structSize));
    printf("%*s    %sheapMemoryType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_HEAP_MEMORY_TYPE(m_struct.heapMemoryType));
    printf("%*s    %sheapSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.heapSize));
    printf("%*s    %spageSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.pageSize));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sgpuReadPerfRating = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.gpuReadPerfRating));
    printf("%*s    %sgpuWritePerfRating = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.gpuWritePerfRating));
    printf("%*s    %scpuReadPerfRating = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.cpuReadPerfRating));
    printf("%*s    %scpuWritePerfRating = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.cpuWritePerfRating));
}

// Output all struct elements, each on their own line
void xgl_memory_heap_properties_struct_wrapper::display_txt()
{
    printf("%*sXGL_MEMORY_HEAP_PROPERTIES struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_memory_heap_properties_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_MEMORY_HEAP_PROPERTIES struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_image_subresource_struct_wrapper class definition
xgl_image_subresource_struct_wrapper::xgl_image_subresource_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_image_subresource_struct_wrapper::xgl_image_subresource_struct_wrapper(XGL_IMAGE_SUBRESOURCE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_subresource_struct_wrapper::xgl_image_subresource_struct_wrapper(const XGL_IMAGE_SUBRESOURCE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_subresource_struct_wrapper::~xgl_image_subresource_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_image_subresource_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_IMAGE_SUBRESOURCE = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_image_subresource_struct_wrapper::display_struct_members()
{
    printf("%*s    %saspect = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_ASPECT(m_struct.aspect));
    printf("%*s    %smipLevel = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.mipLevel));
    printf("%*s    %sarraySlice = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.arraySlice));
}

// Output all struct elements, each on their own line
void xgl_image_subresource_struct_wrapper::display_txt()
{
    printf("%*sXGL_IMAGE_SUBRESOURCE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_image_subresource_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_IMAGE_SUBRESOURCE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_physical_gpu_performance_struct_wrapper class definition
xgl_physical_gpu_performance_struct_wrapper::xgl_physical_gpu_performance_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_physical_gpu_performance_struct_wrapper::xgl_physical_gpu_performance_struct_wrapper(XGL_PHYSICAL_GPU_PERFORMANCE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_physical_gpu_performance_struct_wrapper::xgl_physical_gpu_performance_struct_wrapper(const XGL_PHYSICAL_GPU_PERFORMANCE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_physical_gpu_performance_struct_wrapper::~xgl_physical_gpu_performance_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_physical_gpu_performance_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PHYSICAL_GPU_PERFORMANCE = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_physical_gpu_performance_struct_wrapper::display_struct_members()
{
    printf("%*s    %smaxGpuClock = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.maxGpuClock));
    printf("%*s    %saluPerClock = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.aluPerClock));
    printf("%*s    %stexPerClock = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.texPerClock));
    printf("%*s    %sprimsPerClock = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.primsPerClock));
    printf("%*s    %spixelsPerClock = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.pixelsPerClock));
}

// Output all struct elements, each on their own line
void xgl_physical_gpu_performance_struct_wrapper::display_txt()
{
    printf("%*sXGL_PHYSICAL_GPU_PERFORMANCE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_physical_gpu_performance_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PHYSICAL_GPU_PERFORMANCE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_physical_gpu_memory_properties_struct_wrapper class definition
xgl_physical_gpu_memory_properties_struct_wrapper::xgl_physical_gpu_memory_properties_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_physical_gpu_memory_properties_struct_wrapper::xgl_physical_gpu_memory_properties_struct_wrapper(XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_physical_gpu_memory_properties_struct_wrapper::xgl_physical_gpu_memory_properties_struct_wrapper(const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_physical_gpu_memory_properties_struct_wrapper::~xgl_physical_gpu_memory_properties_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_physical_gpu_memory_properties_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PHYSICAL_GPU_MEMORY_PROPERTIES = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_physical_gpu_memory_properties_struct_wrapper::display_struct_members()
{
    printf("%*s    %sstructSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.structSize));
    printf("%*s    %ssupportsMigration = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.supportsMigration) ? "TRUE" : "FALSE");
    printf("%*s    %ssupportsVirtualMemoryRemapping = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.supportsVirtualMemoryRemapping) ? "TRUE" : "FALSE");
    printf("%*s    %ssupportsPinning = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.supportsPinning) ? "TRUE" : "FALSE");
}

// Output all struct elements, each on their own line
void xgl_physical_gpu_memory_properties_struct_wrapper::display_txt()
{
    printf("%*sXGL_PHYSICAL_GPU_MEMORY_PROPERTIES struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_physical_gpu_memory_properties_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PHYSICAL_GPU_MEMORY_PROPERTIES struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_pipeline_shader_struct_wrapper class definition
xgl_pipeline_shader_struct_wrapper::xgl_pipeline_shader_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_shader_struct_wrapper::xgl_pipeline_shader_struct_wrapper(XGL_PIPELINE_SHADER* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_shader_struct_wrapper::xgl_pipeline_shader_struct_wrapper(const XGL_PIPELINE_SHADER* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_shader_struct_wrapper::~xgl_pipeline_shader_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_shader_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_SHADER = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_shader_struct_wrapper::display_struct_members()
{
    printf("%*s    %sstage = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_PIPELINE_SHADER_STAGE(m_struct.stage));
    printf("%*s    %sshader = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.shader));
    uint32_t i;
    for (i = 0; i<XGL_MAX_DESCRIPTOR_SETS; i++) {
        printf("%*s    %sdescriptorSetMapping[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)&(m_struct.descriptorSetMapping)[i]);
    }
    printf("%*s    %slinkConstBufferCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.linkConstBufferCount));
    printf("%*s    %spLinkConstBufferInfo = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pLinkConstBufferInfo));
    printf("%*s    %sdynamicMemoryViewMapping = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.dynamicMemoryViewMapping));
}

// Output all struct elements, each on their own line
void xgl_pipeline_shader_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_SHADER struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_shader_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_SHADER struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.dynamicMemoryViewMapping) {
        xgl_dynamic_memory_view_slot_info_struct_wrapper class0(&m_struct.dynamicMemoryViewMapping);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pLinkConstBufferInfo) {
        xgl_link_const_buffer_struct_wrapper class1(m_struct.pLinkConstBufferInfo);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    uint32_t i;
    for (i = 0; i<XGL_MAX_DESCRIPTOR_SETS; i++) {
            xgl_descriptor_set_mapping_struct_wrapper class2(&(m_struct.descriptorSetMapping[i]));
            class2.set_indent(m_indent + 4);
            class2.display_full_txt();
    }
}


// xgl_fence_create_info_struct_wrapper class definition
xgl_fence_create_info_struct_wrapper::xgl_fence_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_fence_create_info_struct_wrapper::xgl_fence_create_info_struct_wrapper(XGL_FENCE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_fence_create_info_struct_wrapper::xgl_fence_create_info_struct_wrapper(const XGL_FENCE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_fence_create_info_struct_wrapper::~xgl_fence_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_fence_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_FENCE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_fence_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void xgl_fence_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_FENCE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_fence_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_FENCE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_pipeline_cb_attachment_state_struct_wrapper class definition
xgl_pipeline_cb_attachment_state_struct_wrapper::xgl_pipeline_cb_attachment_state_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_cb_attachment_state_struct_wrapper::xgl_pipeline_cb_attachment_state_struct_wrapper(XGL_PIPELINE_CB_ATTACHMENT_STATE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_cb_attachment_state_struct_wrapper::xgl_pipeline_cb_attachment_state_struct_wrapper(const XGL_PIPELINE_CB_ATTACHMENT_STATE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_cb_attachment_state_struct_wrapper::~xgl_pipeline_cb_attachment_state_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_cb_attachment_state_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_CB_ATTACHMENT_STATE = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_cb_attachment_state_struct_wrapper::display_struct_members()
{
    printf("%*s    %sblendEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.blendEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sformat = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.format));
    printf("%*s    %schannelWriteMask = %hu\n", m_indent, "", &m_dummy_prefix, (m_struct.channelWriteMask));
}

// Output all struct elements, each on their own line
void xgl_pipeline_cb_attachment_state_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_CB_ATTACHMENT_STATE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_cb_attachment_state_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_CB_ATTACHMENT_STATE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.format) {
        xgl_format_struct_wrapper class0(&m_struct.format);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
}


// xgl_alloc_callbacks_struct_wrapper class definition
xgl_alloc_callbacks_struct_wrapper::xgl_alloc_callbacks_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_alloc_callbacks_struct_wrapper::xgl_alloc_callbacks_struct_wrapper(XGL_ALLOC_CALLBACKS* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_alloc_callbacks_struct_wrapper::xgl_alloc_callbacks_struct_wrapper(const XGL_ALLOC_CALLBACKS* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_alloc_callbacks_struct_wrapper::~xgl_alloc_callbacks_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_alloc_callbacks_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_ALLOC_CALLBACKS = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_alloc_callbacks_struct_wrapper::display_struct_members()
{
    printf("%*s    %spUserData = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pUserData));
    printf("%*s    %spfnAlloc = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pfnAlloc));
    printf("%*s    %spfnFree = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pfnFree));
}

// Output all struct elements, each on their own line
void xgl_alloc_callbacks_struct_wrapper::display_txt()
{
    printf("%*sXGL_ALLOC_CALLBACKS struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_alloc_callbacks_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_ALLOC_CALLBACKS struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_color_attachment_view_create_info_struct_wrapper class definition
xgl_color_attachment_view_create_info_struct_wrapper::xgl_color_attachment_view_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_color_attachment_view_create_info_struct_wrapper::xgl_color_attachment_view_create_info_struct_wrapper(XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_color_attachment_view_create_info_struct_wrapper::xgl_color_attachment_view_create_info_struct_wrapper(const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_color_attachment_view_create_info_struct_wrapper::~xgl_color_attachment_view_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_color_attachment_view_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_color_attachment_view_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %simage = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.image));
    printf("%*s    %sformat = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.format));
    printf("%*s    %smipLevel = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.mipLevel));
    printf("%*s    %sbaseArraySlice = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.baseArraySlice));
    printf("%*s    %sarraySize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.arraySize));
}

// Output all struct elements, each on their own line
void xgl_color_attachment_view_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_color_attachment_view_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.format) {
        xgl_format_struct_wrapper class0(&m_struct.format);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_image_copy_struct_wrapper class definition
xgl_image_copy_struct_wrapper::xgl_image_copy_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_image_copy_struct_wrapper::xgl_image_copy_struct_wrapper(XGL_IMAGE_COPY* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_copy_struct_wrapper::xgl_image_copy_struct_wrapper(const XGL_IMAGE_COPY* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_copy_struct_wrapper::~xgl_image_copy_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_image_copy_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_IMAGE_COPY = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_image_copy_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssrcSubresource = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.srcSubresource));
    printf("%*s    %ssrcOffset = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.srcOffset));
    printf("%*s    %sdestSubresource = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.destSubresource));
    printf("%*s    %sdestOffset = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.destOffset));
    printf("%*s    %sextent = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.extent));
}

// Output all struct elements, each on their own line
void xgl_image_copy_struct_wrapper::display_txt()
{
    printf("%*sXGL_IMAGE_COPY struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_image_copy_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_IMAGE_COPY struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.extent) {
        xgl_extent3d_struct_wrapper class0(&m_struct.extent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.destOffset) {
        xgl_offset3d_struct_wrapper class1(&m_struct.destOffset);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (&m_struct.destSubresource) {
        xgl_image_subresource_struct_wrapper class2(&m_struct.destSubresource);
        class2.set_indent(m_indent + 4);
        class2.display_full_txt();
    }
    if (&m_struct.srcOffset) {
        xgl_offset3d_struct_wrapper class3(&m_struct.srcOffset);
        class3.set_indent(m_indent + 4);
        class3.display_full_txt();
    }
    if (&m_struct.srcSubresource) {
        xgl_image_subresource_struct_wrapper class4(&m_struct.srcSubresource);
        class4.set_indent(m_indent + 4);
        class4.display_full_txt();
    }
}


// xgl_msaa_state_create_info_struct_wrapper class definition
xgl_msaa_state_create_info_struct_wrapper::xgl_msaa_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_msaa_state_create_info_struct_wrapper::xgl_msaa_state_create_info_struct_wrapper(XGL_MSAA_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_msaa_state_create_info_struct_wrapper::xgl_msaa_state_create_info_struct_wrapper(const XGL_MSAA_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_msaa_state_create_info_struct_wrapper::~xgl_msaa_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_msaa_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_MSAA_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_msaa_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %ssamples = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.samples));
    printf("%*s    %ssampleMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.sampleMask));
}

// Output all struct elements, each on their own line
void xgl_msaa_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_MSAA_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_msaa_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_MSAA_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_descriptor_set_create_info_struct_wrapper class definition
xgl_descriptor_set_create_info_struct_wrapper::xgl_descriptor_set_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_descriptor_set_create_info_struct_wrapper::xgl_descriptor_set_create_info_struct_wrapper(XGL_DESCRIPTOR_SET_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_descriptor_set_create_info_struct_wrapper::xgl_descriptor_set_create_info_struct_wrapper(const XGL_DESCRIPTOR_SET_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_descriptor_set_create_info_struct_wrapper::~xgl_descriptor_set_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_descriptor_set_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DESCRIPTOR_SET_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_descriptor_set_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sslots = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.slots));
}

// Output all struct elements, each on their own line
void xgl_descriptor_set_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DESCRIPTOR_SET_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_descriptor_set_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DESCRIPTOR_SET_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_color_attachment_bind_info_struct_wrapper class definition
xgl_color_attachment_bind_info_struct_wrapper::xgl_color_attachment_bind_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_color_attachment_bind_info_struct_wrapper::xgl_color_attachment_bind_info_struct_wrapper(XGL_COLOR_ATTACHMENT_BIND_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_color_attachment_bind_info_struct_wrapper::xgl_color_attachment_bind_info_struct_wrapper(const XGL_COLOR_ATTACHMENT_BIND_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_color_attachment_bind_info_struct_wrapper::~xgl_color_attachment_bind_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_color_attachment_bind_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_COLOR_ATTACHMENT_BIND_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_color_attachment_bind_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %sview = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.view));
    printf("%*s    %scolorAttachmentState = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_STATE(m_struct.colorAttachmentState));
}

// Output all struct elements, each on their own line
void xgl_color_attachment_bind_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_COLOR_ATTACHMENT_BIND_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_color_attachment_bind_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_COLOR_ATTACHMENT_BIND_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_event_create_info_struct_wrapper class definition
xgl_event_create_info_struct_wrapper::xgl_event_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_event_create_info_struct_wrapper::xgl_event_create_info_struct_wrapper(XGL_EVENT_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_event_create_info_struct_wrapper::xgl_event_create_info_struct_wrapper(const XGL_EVENT_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_event_create_info_struct_wrapper::~xgl_event_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_event_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_EVENT_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_event_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void xgl_event_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_EVENT_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_event_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_EVENT_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_memory_requirements_struct_wrapper class definition
xgl_memory_requirements_struct_wrapper::xgl_memory_requirements_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_memory_requirements_struct_wrapper::xgl_memory_requirements_struct_wrapper(XGL_MEMORY_REQUIREMENTS* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_requirements_struct_wrapper::xgl_memory_requirements_struct_wrapper(const XGL_MEMORY_REQUIREMENTS* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_requirements_struct_wrapper::~xgl_memory_requirements_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_memory_requirements_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_MEMORY_REQUIREMENTS = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_memory_requirements_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.size));
    printf("%*s    %salignment = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.alignment));
    printf("%*s    %sheapCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.heapCount));
    uint32_t i;
    for (i = 0; i<XGL_MAX_MEMORY_HEAPS; i++) {
        printf("%*s    %sheaps[%u] = %u\n", m_indent, "", &m_dummy_prefix, i, (m_struct.heaps)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_memory_requirements_struct_wrapper::display_txt()
{
    printf("%*sXGL_MEMORY_REQUIREMENTS struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_memory_requirements_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_MEMORY_REQUIREMENTS struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_queue_semaphore_open_info_struct_wrapper class definition
xgl_queue_semaphore_open_info_struct_wrapper::xgl_queue_semaphore_open_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_queue_semaphore_open_info_struct_wrapper::xgl_queue_semaphore_open_info_struct_wrapper(XGL_QUEUE_SEMAPHORE_OPEN_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_queue_semaphore_open_info_struct_wrapper::xgl_queue_semaphore_open_info_struct_wrapper(const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_queue_semaphore_open_info_struct_wrapper::~xgl_queue_semaphore_open_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_queue_semaphore_open_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_QUEUE_SEMAPHORE_OPEN_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_queue_semaphore_open_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %ssharedSemaphore = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.sharedSemaphore));
}

// Output all struct elements, each on their own line
void xgl_queue_semaphore_open_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_QUEUE_SEMAPHORE_OPEN_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_queue_semaphore_open_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_QUEUE_SEMAPHORE_OPEN_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_image_resolve_struct_wrapper class definition
xgl_image_resolve_struct_wrapper::xgl_image_resolve_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_image_resolve_struct_wrapper::xgl_image_resolve_struct_wrapper(XGL_IMAGE_RESOLVE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_resolve_struct_wrapper::xgl_image_resolve_struct_wrapper(const XGL_IMAGE_RESOLVE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_resolve_struct_wrapper::~xgl_image_resolve_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_image_resolve_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_IMAGE_RESOLVE = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_image_resolve_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssrcSubresource = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.srcSubresource));
    printf("%*s    %ssrcOffset = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.srcOffset));
    printf("%*s    %sdestSubresource = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.destSubresource));
    printf("%*s    %sdestOffset = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.destOffset));
    printf("%*s    %sextent = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.extent));
}

// Output all struct elements, each on their own line
void xgl_image_resolve_struct_wrapper::display_txt()
{
    printf("%*sXGL_IMAGE_RESOLVE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_image_resolve_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_IMAGE_RESOLVE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.extent) {
        xgl_extent2d_struct_wrapper class0(&m_struct.extent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.destOffset) {
        xgl_offset2d_struct_wrapper class1(&m_struct.destOffset);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (&m_struct.destSubresource) {
        xgl_image_subresource_struct_wrapper class2(&m_struct.destSubresource);
        class2.set_indent(m_indent + 4);
        class2.display_full_txt();
    }
    if (&m_struct.srcOffset) {
        xgl_offset2d_struct_wrapper class3(&m_struct.srcOffset);
        class3.set_indent(m_indent + 4);
        class3.display_full_txt();
    }
    if (&m_struct.srcSubresource) {
        xgl_image_subresource_struct_wrapper class4(&m_struct.srcSubresource);
        class4.set_indent(m_indent + 4);
        class4.display_full_txt();
    }
}


// xgl_draw_indexed_indirect_cmd_struct_wrapper class definition
xgl_draw_indexed_indirect_cmd_struct_wrapper::xgl_draw_indexed_indirect_cmd_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_draw_indexed_indirect_cmd_struct_wrapper::xgl_draw_indexed_indirect_cmd_struct_wrapper(XGL_DRAW_INDEXED_INDIRECT_CMD* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_draw_indexed_indirect_cmd_struct_wrapper::xgl_draw_indexed_indirect_cmd_struct_wrapper(const XGL_DRAW_INDEXED_INDIRECT_CMD* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_draw_indexed_indirect_cmd_struct_wrapper::~xgl_draw_indexed_indirect_cmd_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_draw_indexed_indirect_cmd_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DRAW_INDEXED_INDIRECT_CMD = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_draw_indexed_indirect_cmd_struct_wrapper::display_struct_members()
{
    printf("%*s    %sindexCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.indexCount));
    printf("%*s    %sinstanceCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.instanceCount));
    printf("%*s    %sfirstIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.firstIndex));
    printf("%*s    %svertexOffset = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.vertexOffset));
    printf("%*s    %sfirstInstance = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.firstInstance));
}

// Output all struct elements, each on their own line
void xgl_draw_indexed_indirect_cmd_struct_wrapper::display_txt()
{
    printf("%*sXGL_DRAW_INDEXED_INDIRECT_CMD struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_draw_indexed_indirect_cmd_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DRAW_INDEXED_INDIRECT_CMD struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_compute_pipeline_create_info_struct_wrapper class definition
xgl_compute_pipeline_create_info_struct_wrapper::xgl_compute_pipeline_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_compute_pipeline_create_info_struct_wrapper::xgl_compute_pipeline_create_info_struct_wrapper(XGL_COMPUTE_PIPELINE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_compute_pipeline_create_info_struct_wrapper::xgl_compute_pipeline_create_info_struct_wrapper(const XGL_COMPUTE_PIPELINE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_compute_pipeline_create_info_struct_wrapper::~xgl_compute_pipeline_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_compute_pipeline_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_COMPUTE_PIPELINE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_compute_pipeline_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %scs = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.cs));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void xgl_compute_pipeline_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_COMPUTE_PIPELINE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_compute_pipeline_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_COMPUTE_PIPELINE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.cs) {
        xgl_pipeline_shader_struct_wrapper class0(&m_struct.cs);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_peer_image_open_info_struct_wrapper class definition
xgl_peer_image_open_info_struct_wrapper::xgl_peer_image_open_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_peer_image_open_info_struct_wrapper::xgl_peer_image_open_info_struct_wrapper(XGL_PEER_IMAGE_OPEN_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_peer_image_open_info_struct_wrapper::xgl_peer_image_open_info_struct_wrapper(const XGL_PEER_IMAGE_OPEN_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_peer_image_open_info_struct_wrapper::~xgl_peer_image_open_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_peer_image_open_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PEER_IMAGE_OPEN_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_peer_image_open_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %soriginalImage = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.originalImage));
}

// Output all struct elements, each on their own line
void xgl_peer_image_open_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_PEER_IMAGE_OPEN_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_peer_image_open_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PEER_IMAGE_OPEN_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_physical_gpu_queue_properties_struct_wrapper class definition
xgl_physical_gpu_queue_properties_struct_wrapper::xgl_physical_gpu_queue_properties_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_physical_gpu_queue_properties_struct_wrapper::xgl_physical_gpu_queue_properties_struct_wrapper(XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_physical_gpu_queue_properties_struct_wrapper::xgl_physical_gpu_queue_properties_struct_wrapper(const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_physical_gpu_queue_properties_struct_wrapper::~xgl_physical_gpu_queue_properties_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_physical_gpu_queue_properties_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PHYSICAL_GPU_QUEUE_PROPERTIES = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_physical_gpu_queue_properties_struct_wrapper::display_struct_members()
{
    printf("%*s    %sstructSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.structSize));
    printf("%*s    %squeueFlags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queueFlags));
    printf("%*s    %squeueCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queueCount));
    printf("%*s    %smaxAtomicCounters = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxAtomicCounters));
    printf("%*s    %ssupportsTimestamps = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.supportsTimestamps) ? "TRUE" : "FALSE");
}

// Output all struct elements, each on their own line
void xgl_physical_gpu_queue_properties_struct_wrapper::display_txt()
{
    printf("%*sXGL_PHYSICAL_GPU_QUEUE_PROPERTIES struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_physical_gpu_queue_properties_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PHYSICAL_GPU_QUEUE_PROPERTIES struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_pipeline_statistics_data_struct_wrapper class definition
xgl_pipeline_statistics_data_struct_wrapper::xgl_pipeline_statistics_data_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_statistics_data_struct_wrapper::xgl_pipeline_statistics_data_struct_wrapper(XGL_PIPELINE_STATISTICS_DATA* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_statistics_data_struct_wrapper::xgl_pipeline_statistics_data_struct_wrapper(const XGL_PIPELINE_STATISTICS_DATA* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_statistics_data_struct_wrapper::~xgl_pipeline_statistics_data_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_statistics_data_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_STATISTICS_DATA = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_statistics_data_struct_wrapper::display_struct_members()
{
    printf("%*s    %sfsInvocations = %lu\n", m_indent, "", &m_dummy_prefix, (m_struct.fsInvocations));
    printf("%*s    %scPrimitives = %lu\n", m_indent, "", &m_dummy_prefix, (m_struct.cPrimitives));
    printf("%*s    %scInvocations = %lu\n", m_indent, "", &m_dummy_prefix, (m_struct.cInvocations));
    printf("%*s    %svsInvocations = %lu\n", m_indent, "", &m_dummy_prefix, (m_struct.vsInvocations));
    printf("%*s    %sgsInvocations = %lu\n", m_indent, "", &m_dummy_prefix, (m_struct.gsInvocations));
    printf("%*s    %sgsPrimitives = %lu\n", m_indent, "", &m_dummy_prefix, (m_struct.gsPrimitives));
    printf("%*s    %siaPrimitives = %lu\n", m_indent, "", &m_dummy_prefix, (m_struct.iaPrimitives));
    printf("%*s    %siaVertices = %lu\n", m_indent, "", &m_dummy_prefix, (m_struct.iaVertices));
    printf("%*s    %stcsInvocations = %lu\n", m_indent, "", &m_dummy_prefix, (m_struct.tcsInvocations));
    printf("%*s    %stesInvocations = %lu\n", m_indent, "", &m_dummy_prefix, (m_struct.tesInvocations));
    printf("%*s    %scsInvocations = %lu\n", m_indent, "", &m_dummy_prefix, (m_struct.csInvocations));
}

// Output all struct elements, each on their own line
void xgl_pipeline_statistics_data_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_STATISTICS_DATA struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_statistics_data_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_STATISTICS_DATA struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_device_queue_create_info_struct_wrapper class definition
xgl_device_queue_create_info_struct_wrapper::xgl_device_queue_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_device_queue_create_info_struct_wrapper::xgl_device_queue_create_info_struct_wrapper(XGL_DEVICE_QUEUE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_device_queue_create_info_struct_wrapper::xgl_device_queue_create_info_struct_wrapper(const XGL_DEVICE_QUEUE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_device_queue_create_info_struct_wrapper::~xgl_device_queue_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_device_queue_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DEVICE_QUEUE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_device_queue_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %squeueNodeIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queueNodeIndex));
    printf("%*s    %squeueCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queueCount));
}

// Output all struct elements, each on their own line
void xgl_device_queue_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DEVICE_QUEUE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_device_queue_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DEVICE_QUEUE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_sampler_create_info_struct_wrapper class definition
xgl_sampler_create_info_struct_wrapper::xgl_sampler_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_sampler_create_info_struct_wrapper::xgl_sampler_create_info_struct_wrapper(XGL_SAMPLER_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_sampler_create_info_struct_wrapper::xgl_sampler_create_info_struct_wrapper(const XGL_SAMPLER_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_sampler_create_info_struct_wrapper::~xgl_sampler_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_sampler_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_SAMPLER_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_sampler_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %smagFilter = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_TEX_FILTER(m_struct.magFilter));
    printf("%*s    %sminFilter = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_TEX_FILTER(m_struct.minFilter));
    printf("%*s    %smipMode = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_TEX_MIPMAP_MODE(m_struct.mipMode));
    printf("%*s    %saddressU = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_TEX_ADDRESS(m_struct.addressU));
    printf("%*s    %saddressV = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_TEX_ADDRESS(m_struct.addressV));
    printf("%*s    %saddressW = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_TEX_ADDRESS(m_struct.addressW));
    printf("%*s    %smipLodBias = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.mipLodBias));
    printf("%*s    %smaxAnisotropy = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxAnisotropy));
    printf("%*s    %scompareFunc = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_COMPARE_FUNC(m_struct.compareFunc));
    printf("%*s    %sminLod = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.minLod));
    printf("%*s    %smaxLod = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.maxLod));
    printf("%*s    %sborderColorType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_BORDER_COLOR_TYPE(m_struct.borderColorType));
}

// Output all struct elements, each on their own line
void xgl_sampler_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_SAMPLER_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_sampler_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_SAMPLER_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_queue_semaphore_create_info_struct_wrapper class definition
xgl_queue_semaphore_create_info_struct_wrapper::xgl_queue_semaphore_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_queue_semaphore_create_info_struct_wrapper::xgl_queue_semaphore_create_info_struct_wrapper(XGL_QUEUE_SEMAPHORE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_queue_semaphore_create_info_struct_wrapper::xgl_queue_semaphore_create_info_struct_wrapper(const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_queue_semaphore_create_info_struct_wrapper::~xgl_queue_semaphore_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_queue_semaphore_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_QUEUE_SEMAPHORE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_queue_semaphore_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sinitialCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.initialCount));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void xgl_queue_semaphore_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_QUEUE_SEMAPHORE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_queue_semaphore_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_QUEUE_SEMAPHORE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_format_struct_wrapper class definition
xgl_format_struct_wrapper::xgl_format_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_format_struct_wrapper::xgl_format_struct_wrapper(XGL_FORMAT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_format_struct_wrapper::xgl_format_struct_wrapper(const XGL_FORMAT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_format_struct_wrapper::~xgl_format_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_format_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_FORMAT = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_format_struct_wrapper::display_struct_members()
{
    printf("%*s    %schannelFormat = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_CHANNEL_FORMAT(m_struct.channelFormat));
    printf("%*s    %snumericFormat = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_NUM_FORMAT(m_struct.numericFormat));
}

// Output all struct elements, each on their own line
void xgl_format_struct_wrapper::display_txt()
{
    printf("%*sXGL_FORMAT struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_format_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_FORMAT struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_memory_state_transition_struct_wrapper class definition
xgl_memory_state_transition_struct_wrapper::xgl_memory_state_transition_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_memory_state_transition_struct_wrapper::xgl_memory_state_transition_struct_wrapper(XGL_MEMORY_STATE_TRANSITION* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_state_transition_struct_wrapper::xgl_memory_state_transition_struct_wrapper(const XGL_MEMORY_STATE_TRANSITION* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_state_transition_struct_wrapper::~xgl_memory_state_transition_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_memory_state_transition_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_MEMORY_STATE_TRANSITION = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_memory_state_transition_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %smem = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.mem));
    printf("%*s    %soldState = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_MEMORY_STATE(m_struct.oldState));
    printf("%*s    %snewState = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_MEMORY_STATE(m_struct.newState));
    printf("%*s    %soffset = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.offset));
    printf("%*s    %sregionSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.regionSize));
}

// Output all struct elements, each on their own line
void xgl_memory_state_transition_struct_wrapper::display_txt()
{
    printf("%*sXGL_MEMORY_STATE_TRANSITION struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_memory_state_transition_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_MEMORY_STATE_TRANSITION struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_extent3d_struct_wrapper class definition
xgl_extent3d_struct_wrapper::xgl_extent3d_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_extent3d_struct_wrapper::xgl_extent3d_struct_wrapper(XGL_EXTENT3D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_extent3d_struct_wrapper::xgl_extent3d_struct_wrapper(const XGL_EXTENT3D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_extent3d_struct_wrapper::~xgl_extent3d_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_extent3d_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_EXTENT3D = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_extent3d_struct_wrapper::display_struct_members()
{
    printf("%*s    %swidth = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.width));
    printf("%*s    %sheight = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.height));
    printf("%*s    %sdepth = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.depth));
}

// Output all struct elements, each on their own line
void xgl_extent3d_struct_wrapper::display_txt()
{
    printf("%*sXGL_EXTENT3D struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_extent3d_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_EXTENT3D struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_dynamic_memory_view_slot_info_struct_wrapper class definition
xgl_dynamic_memory_view_slot_info_struct_wrapper::xgl_dynamic_memory_view_slot_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_dynamic_memory_view_slot_info_struct_wrapper::xgl_dynamic_memory_view_slot_info_struct_wrapper(XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_dynamic_memory_view_slot_info_struct_wrapper::xgl_dynamic_memory_view_slot_info_struct_wrapper(const XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_dynamic_memory_view_slot_info_struct_wrapper::~xgl_dynamic_memory_view_slot_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_dynamic_memory_view_slot_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_dynamic_memory_view_slot_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %sslotObjectType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_DESCRIPTOR_SET_SLOT_TYPE(m_struct.slotObjectType));
    printf("%*s    %sshaderEntityIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderEntityIndex));
}

// Output all struct elements, each on their own line
void xgl_dynamic_memory_view_slot_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_dynamic_memory_view_slot_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_image_view_attach_info_struct_wrapper class definition
xgl_image_view_attach_info_struct_wrapper::xgl_image_view_attach_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_image_view_attach_info_struct_wrapper::xgl_image_view_attach_info_struct_wrapper(XGL_IMAGE_VIEW_ATTACH_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_view_attach_info_struct_wrapper::xgl_image_view_attach_info_struct_wrapper(const XGL_IMAGE_VIEW_ATTACH_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_view_attach_info_struct_wrapper::~xgl_image_view_attach_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_image_view_attach_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_IMAGE_VIEW_ATTACH_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_image_view_attach_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sview = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.view));
    printf("%*s    %sstate = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_STATE(m_struct.state));
}

// Output all struct elements, each on their own line
void xgl_image_view_attach_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_IMAGE_VIEW_ATTACH_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_image_view_attach_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_IMAGE_VIEW_ATTACH_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_image_subresource_range_struct_wrapper class definition
xgl_image_subresource_range_struct_wrapper::xgl_image_subresource_range_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_image_subresource_range_struct_wrapper::xgl_image_subresource_range_struct_wrapper(XGL_IMAGE_SUBRESOURCE_RANGE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_subresource_range_struct_wrapper::xgl_image_subresource_range_struct_wrapper(const XGL_IMAGE_SUBRESOURCE_RANGE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_subresource_range_struct_wrapper::~xgl_image_subresource_range_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_image_subresource_range_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_IMAGE_SUBRESOURCE_RANGE = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_image_subresource_range_struct_wrapper::display_struct_members()
{
    printf("%*s    %saspect = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_ASPECT(m_struct.aspect));
    printf("%*s    %sbaseMipLevel = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.baseMipLevel));
    printf("%*s    %smipLevels = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.mipLevels));
    printf("%*s    %sbaseArraySlice = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.baseArraySlice));
    printf("%*s    %sarraySize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.arraySize));
}

// Output all struct elements, each on their own line
void xgl_image_subresource_range_struct_wrapper::display_txt()
{
    printf("%*sXGL_IMAGE_SUBRESOURCE_RANGE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_image_subresource_range_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_IMAGE_SUBRESOURCE_RANGE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_pipeline_db_state_create_info_struct_wrapper class definition
xgl_pipeline_db_state_create_info_struct_wrapper::xgl_pipeline_db_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_db_state_create_info_struct_wrapper::xgl_pipeline_db_state_create_info_struct_wrapper(XGL_PIPELINE_DB_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_db_state_create_info_struct_wrapper::xgl_pipeline_db_state_create_info_struct_wrapper(const XGL_PIPELINE_DB_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_db_state_create_info_struct_wrapper::~xgl_pipeline_db_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_db_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_DB_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_db_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sformat = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.format));
}

// Output all struct elements, each on their own line
void xgl_pipeline_db_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_DB_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_db_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_DB_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.format) {
        xgl_format_struct_wrapper class0(&m_struct.format);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_application_info_struct_wrapper class definition
xgl_application_info_struct_wrapper::xgl_application_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_application_info_struct_wrapper::xgl_application_info_struct_wrapper(XGL_APPLICATION_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_application_info_struct_wrapper::xgl_application_info_struct_wrapper(const XGL_APPLICATION_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_application_info_struct_wrapper::~xgl_application_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_application_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_APPLICATION_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_application_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %spAppName = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pAppName));
    printf("%*s    %sappVersion = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.appVersion));
    printf("%*s    %spEngineName = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pEngineName));
    printf("%*s    %sengineVersion = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.engineVersion));
    printf("%*s    %sapiVersion = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.apiVersion));
}

// Output all struct elements, each on their own line
void xgl_application_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_APPLICATION_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_application_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_APPLICATION_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_offset2d_struct_wrapper class definition
xgl_offset2d_struct_wrapper::xgl_offset2d_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_offset2d_struct_wrapper::xgl_offset2d_struct_wrapper(XGL_OFFSET2D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_offset2d_struct_wrapper::xgl_offset2d_struct_wrapper(const XGL_OFFSET2D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_offset2d_struct_wrapper::~xgl_offset2d_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_offset2d_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_OFFSET2D = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_offset2d_struct_wrapper::display_struct_members()
{
    printf("%*s    %sx = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.x));
    printf("%*s    %sy = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.y));
}

// Output all struct elements, each on their own line
void xgl_offset2d_struct_wrapper::display_txt()
{
    printf("%*sXGL_OFFSET2D struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_offset2d_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_OFFSET2D struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_viewport_state_create_info_struct_wrapper class definition
xgl_viewport_state_create_info_struct_wrapper::xgl_viewport_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_viewport_state_create_info_struct_wrapper::xgl_viewport_state_create_info_struct_wrapper(XGL_VIEWPORT_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_viewport_state_create_info_struct_wrapper::xgl_viewport_state_create_info_struct_wrapper(const XGL_VIEWPORT_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_viewport_state_create_info_struct_wrapper::~xgl_viewport_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_viewport_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_VIEWPORT_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_viewport_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %sviewportCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.viewportCount));
    printf("%*s    %sscissorEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.scissorEnable) ? "TRUE" : "FALSE");
    uint32_t i;
    for (i = 0; i<XGL_MAX_VIEWPORTS; i++) {
        printf("%*s    %sviewports[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)&(m_struct.viewports)[i]);
    }
    for (i = 0; i<XGL_MAX_VIEWPORTS; i++) {
        printf("%*s    %sscissors[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)&(m_struct.scissors)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_viewport_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_VIEWPORT_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_viewport_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_VIEWPORT_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<XGL_MAX_VIEWPORTS; i++) {
            xgl_rect_struct_wrapper class0(&(m_struct.scissors[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    for (i = 0; i<XGL_MAX_VIEWPORTS; i++) {
            xgl_viewport_struct_wrapper class1(&(m_struct.viewports[i]));
            class1.set_indent(m_indent + 4);
            class1.display_full_txt();
    }
}


// xgl_image_state_transition_struct_wrapper class definition
xgl_image_state_transition_struct_wrapper::xgl_image_state_transition_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_image_state_transition_struct_wrapper::xgl_image_state_transition_struct_wrapper(XGL_IMAGE_STATE_TRANSITION* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_state_transition_struct_wrapper::xgl_image_state_transition_struct_wrapper(const XGL_IMAGE_STATE_TRANSITION* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_state_transition_struct_wrapper::~xgl_image_state_transition_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_image_state_transition_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_IMAGE_STATE_TRANSITION = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_image_state_transition_struct_wrapper::display_struct_members()
{
    printf("%*s    %simage = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.image));
    printf("%*s    %soldState = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_STATE(m_struct.oldState));
    printf("%*s    %snewState = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_STATE(m_struct.newState));
    printf("%*s    %ssubresourceRange = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.subresourceRange));
}

// Output all struct elements, each on their own line
void xgl_image_state_transition_struct_wrapper::display_txt()
{
    printf("%*sXGL_IMAGE_STATE_TRANSITION struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_image_state_transition_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_IMAGE_STATE_TRANSITION struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.subresourceRange) {
        xgl_image_subresource_range_struct_wrapper class0(&m_struct.subresourceRange);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
}


// xgl_device_create_info_struct_wrapper class definition
xgl_device_create_info_struct_wrapper::xgl_device_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_device_create_info_struct_wrapper::xgl_device_create_info_struct_wrapper(XGL_DEVICE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_device_create_info_struct_wrapper::xgl_device_create_info_struct_wrapper(const XGL_DEVICE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_device_create_info_struct_wrapper::~xgl_device_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_device_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DEVICE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_device_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %squeueRecordCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.queueRecordCount));
    printf("%*s    %spRequestedQueues = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pRequestedQueues));
    printf("%*s    %sextensionCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.extensionCount));
    printf("%*s    %sppEnabledExtensionNames = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.ppEnabledExtensionNames));
    printf("%*s    %smaxValidationLevel = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_VALIDATION_LEVEL(m_struct.maxValidationLevel));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void xgl_device_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DEVICE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_device_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DEVICE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pRequestedQueues) {
        xgl_device_queue_create_info_struct_wrapper class0(m_struct.pRequestedQueues);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_image_create_info_struct_wrapper class definition
xgl_image_create_info_struct_wrapper::xgl_image_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_image_create_info_struct_wrapper::xgl_image_create_info_struct_wrapper(XGL_IMAGE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_create_info_struct_wrapper::xgl_image_create_info_struct_wrapper(const XGL_IMAGE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_create_info_struct_wrapper::~xgl_image_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_image_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_IMAGE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_image_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %simageType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_TYPE(m_struct.imageType));
    printf("%*s    %sformat = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.format));
    printf("%*s    %sextent = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.extent));
    printf("%*s    %smipLevels = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.mipLevels));
    printf("%*s    %sarraySize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.arraySize));
    printf("%*s    %ssamples = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.samples));
    printf("%*s    %stiling = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_TILING(m_struct.tiling));
    printf("%*s    %susage = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.usage));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void xgl_image_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_IMAGE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_image_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_IMAGE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.extent) {
        xgl_extent3d_struct_wrapper class0(&m_struct.extent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.format) {
        xgl_format_struct_wrapper class1(&m_struct.format);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_rect_struct_wrapper class definition
xgl_rect_struct_wrapper::xgl_rect_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_rect_struct_wrapper::xgl_rect_struct_wrapper(XGL_RECT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_rect_struct_wrapper::xgl_rect_struct_wrapper(const XGL_RECT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_rect_struct_wrapper::~xgl_rect_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_rect_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_RECT = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_rect_struct_wrapper::display_struct_members()
{
    printf("%*s    %soffset = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.offset));
    printf("%*s    %sextent = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.extent));
}

// Output all struct elements, each on their own line
void xgl_rect_struct_wrapper::display_txt()
{
    printf("%*sXGL_RECT struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_rect_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_RECT struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.extent) {
        xgl_extent2d_struct_wrapper class0(&m_struct.extent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.offset) {
        xgl_offset2d_struct_wrapper class1(&m_struct.offset);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
}


// xgl_memory_copy_struct_wrapper class definition
xgl_memory_copy_struct_wrapper::xgl_memory_copy_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_memory_copy_struct_wrapper::xgl_memory_copy_struct_wrapper(XGL_MEMORY_COPY* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_copy_struct_wrapper::xgl_memory_copy_struct_wrapper(const XGL_MEMORY_COPY* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_copy_struct_wrapper::~xgl_memory_copy_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_memory_copy_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_MEMORY_COPY = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_memory_copy_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssrcOffset = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.srcOffset));
    printf("%*s    %sdestOffset = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.destOffset));
    printf("%*s    %scopySize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.copySize));
}

// Output all struct elements, each on their own line
void xgl_memory_copy_struct_wrapper::display_txt()
{
    printf("%*sXGL_MEMORY_COPY struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_memory_copy_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_MEMORY_COPY struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_descriptor_slot_info_struct_wrapper class definition
xgl_descriptor_slot_info_struct_wrapper::xgl_descriptor_slot_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_descriptor_slot_info_struct_wrapper::xgl_descriptor_slot_info_struct_wrapper(XGL_DESCRIPTOR_SLOT_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_descriptor_slot_info_struct_wrapper::xgl_descriptor_slot_info_struct_wrapper(const XGL_DESCRIPTOR_SLOT_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_descriptor_slot_info_struct_wrapper::~xgl_descriptor_slot_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_descriptor_slot_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DESCRIPTOR_SLOT_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_descriptor_slot_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %sslotObjectType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_DESCRIPTOR_SET_SLOT_TYPE(m_struct.slotObjectType));
    printf("%*s    %sshaderEntityIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.shaderEntityIndex));
    printf("%*s    %spNextLevelSet = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNextLevelSet));
}

// Output all struct elements, each on their own line
void xgl_descriptor_slot_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DESCRIPTOR_SLOT_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_descriptor_slot_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DESCRIPTOR_SLOT_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_link_const_buffer_struct_wrapper class definition
xgl_link_const_buffer_struct_wrapper::xgl_link_const_buffer_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_link_const_buffer_struct_wrapper::xgl_link_const_buffer_struct_wrapper(XGL_LINK_CONST_BUFFER* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_link_const_buffer_struct_wrapper::xgl_link_const_buffer_struct_wrapper(const XGL_LINK_CONST_BUFFER* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_link_const_buffer_struct_wrapper::~xgl_link_const_buffer_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_link_const_buffer_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_LINK_CONST_BUFFER = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_link_const_buffer_struct_wrapper::display_struct_members()
{
    printf("%*s    %sbufferId = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.bufferId));
    printf("%*s    %sbufferSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.bufferSize));
    printf("%*s    %spBufferData = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pBufferData));
}

// Output all struct elements, each on their own line
void xgl_link_const_buffer_struct_wrapper::display_txt()
{
    printf("%*sXGL_LINK_CONST_BUFFER struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_link_const_buffer_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_LINK_CONST_BUFFER struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_memory_image_copy_struct_wrapper class definition
xgl_memory_image_copy_struct_wrapper::xgl_memory_image_copy_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_memory_image_copy_struct_wrapper::xgl_memory_image_copy_struct_wrapper(XGL_MEMORY_IMAGE_COPY* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_image_copy_struct_wrapper::xgl_memory_image_copy_struct_wrapper(const XGL_MEMORY_IMAGE_COPY* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_image_copy_struct_wrapper::~xgl_memory_image_copy_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_memory_image_copy_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_MEMORY_IMAGE_COPY = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_memory_image_copy_struct_wrapper::display_struct_members()
{
    printf("%*s    %smemOffset = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.memOffset));
    printf("%*s    %simageSubresource = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.imageSubresource));
    printf("%*s    %simageOffset = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.imageOffset));
    printf("%*s    %simageExtent = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.imageExtent));
}

// Output all struct elements, each on their own line
void xgl_memory_image_copy_struct_wrapper::display_txt()
{
    printf("%*sXGL_MEMORY_IMAGE_COPY struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_memory_image_copy_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_MEMORY_IMAGE_COPY struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.imageExtent) {
        xgl_extent3d_struct_wrapper class0(&m_struct.imageExtent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.imageOffset) {
        xgl_offset3d_struct_wrapper class1(&m_struct.imageOffset);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (&m_struct.imageSubresource) {
        xgl_image_subresource_struct_wrapper class2(&m_struct.imageSubresource);
        class2.set_indent(m_indent + 4);
        class2.display_full_txt();
    }
}


// xgl_depth_stencil_state_create_info_struct_wrapper class definition
xgl_depth_stencil_state_create_info_struct_wrapper::xgl_depth_stencil_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_depth_stencil_state_create_info_struct_wrapper::xgl_depth_stencil_state_create_info_struct_wrapper(XGL_DEPTH_STENCIL_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_depth_stencil_state_create_info_struct_wrapper::xgl_depth_stencil_state_create_info_struct_wrapper(const XGL_DEPTH_STENCIL_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_depth_stencil_state_create_info_struct_wrapper::~xgl_depth_stencil_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_depth_stencil_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DEPTH_STENCIL_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_depth_stencil_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sdepthTestEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthTestEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sdepthWriteEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthWriteEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sdepthFunc = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_COMPARE_FUNC(m_struct.depthFunc));
    printf("%*s    %sdepthBoundsEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthBoundsEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sminDepth = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.minDepth));
    printf("%*s    %smaxDepth = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDepth));
    printf("%*s    %sstencilTestEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.stencilTestEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sstencilReadMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stencilReadMask));
    printf("%*s    %sstencilWriteMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stencilWriteMask));
    printf("%*s    %sfront = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.front));
    printf("%*s    %sback = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.back));
}

// Output all struct elements, each on their own line
void xgl_depth_stencil_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DEPTH_STENCIL_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_depth_stencil_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DEPTH_STENCIL_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.back) {
        xgl_stencil_op_state_struct_wrapper class0(&m_struct.back);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.front) {
        xgl_stencil_op_state_struct_wrapper class1(&m_struct.front);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_viewport_struct_wrapper class definition
xgl_viewport_struct_wrapper::xgl_viewport_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_viewport_struct_wrapper::xgl_viewport_struct_wrapper(XGL_VIEWPORT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_viewport_struct_wrapper::xgl_viewport_struct_wrapper(const XGL_VIEWPORT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_viewport_struct_wrapper::~xgl_viewport_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_viewport_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_VIEWPORT = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_viewport_struct_wrapper::display_struct_members()
{
    printf("%*s    %soriginX = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.originX));
    printf("%*s    %soriginY = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.originY));
    printf("%*s    %swidth = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.width));
    printf("%*s    %sheight = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.height));
    printf("%*s    %sminDepth = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.minDepth));
    printf("%*s    %smaxDepth = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDepth));
}

// Output all struct elements, each on their own line
void xgl_viewport_struct_wrapper::display_txt()
{
    printf("%*sXGL_VIEWPORT struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_viewport_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_VIEWPORT struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_descriptor_set_mapping_struct_wrapper class definition
xgl_descriptor_set_mapping_struct_wrapper::xgl_descriptor_set_mapping_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_descriptor_set_mapping_struct_wrapper::xgl_descriptor_set_mapping_struct_wrapper(XGL_DESCRIPTOR_SET_MAPPING* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_descriptor_set_mapping_struct_wrapper::xgl_descriptor_set_mapping_struct_wrapper(const XGL_DESCRIPTOR_SET_MAPPING* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_descriptor_set_mapping_struct_wrapper::~xgl_descriptor_set_mapping_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_descriptor_set_mapping_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DESCRIPTOR_SET_MAPPING = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_descriptor_set_mapping_struct_wrapper::display_struct_members()
{
    printf("%*s    %sdescriptorCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.descriptorCount));
    printf("%*s    %spDescriptorInfo = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pDescriptorInfo));
}

// Output all struct elements, each on their own line
void xgl_descriptor_set_mapping_struct_wrapper::display_txt()
{
    printf("%*sXGL_DESCRIPTOR_SET_MAPPING struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_descriptor_set_mapping_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DESCRIPTOR_SET_MAPPING struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pDescriptorInfo) {
        xgl_descriptor_slot_info_struct_wrapper class0(m_struct.pDescriptorInfo);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
}


// xgl_peer_memory_open_info_struct_wrapper class definition
xgl_peer_memory_open_info_struct_wrapper::xgl_peer_memory_open_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_peer_memory_open_info_struct_wrapper::xgl_peer_memory_open_info_struct_wrapper(XGL_PEER_MEMORY_OPEN_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_peer_memory_open_info_struct_wrapper::xgl_peer_memory_open_info_struct_wrapper(const XGL_PEER_MEMORY_OPEN_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_peer_memory_open_info_struct_wrapper::~xgl_peer_memory_open_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_peer_memory_open_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PEER_MEMORY_OPEN_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_peer_memory_open_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %soriginalMem = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.originalMem));
}

// Output all struct elements, each on their own line
void xgl_peer_memory_open_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_PEER_MEMORY_OPEN_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_peer_memory_open_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PEER_MEMORY_OPEN_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_subresource_layout_struct_wrapper class definition
xgl_subresource_layout_struct_wrapper::xgl_subresource_layout_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_subresource_layout_struct_wrapper::xgl_subresource_layout_struct_wrapper(XGL_SUBRESOURCE_LAYOUT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_subresource_layout_struct_wrapper::xgl_subresource_layout_struct_wrapper(const XGL_SUBRESOURCE_LAYOUT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_subresource_layout_struct_wrapper::~xgl_subresource_layout_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_subresource_layout_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_SUBRESOURCE_LAYOUT = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_subresource_layout_struct_wrapper::display_struct_members()
{
    printf("%*s    %soffset = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.offset));
    printf("%*s    %ssize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.size));
    printf("%*s    %srowPitch = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.rowPitch));
    printf("%*s    %sdepthPitch = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.depthPitch));
}

// Output all struct elements, each on their own line
void xgl_subresource_layout_struct_wrapper::display_txt()
{
    printf("%*sXGL_SUBRESOURCE_LAYOUT struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_subresource_layout_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_SUBRESOURCE_LAYOUT struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_descriptor_set_attach_info_struct_wrapper class definition
xgl_descriptor_set_attach_info_struct_wrapper::xgl_descriptor_set_attach_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_descriptor_set_attach_info_struct_wrapper::xgl_descriptor_set_attach_info_struct_wrapper(XGL_DESCRIPTOR_SET_ATTACH_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_descriptor_set_attach_info_struct_wrapper::xgl_descriptor_set_attach_info_struct_wrapper(const XGL_DESCRIPTOR_SET_ATTACH_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_descriptor_set_attach_info_struct_wrapper::~xgl_descriptor_set_attach_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_descriptor_set_attach_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DESCRIPTOR_SET_ATTACH_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_descriptor_set_attach_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %sdescriptorSet = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.descriptorSet));
    printf("%*s    %sslotOffset = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.slotOffset));
}

// Output all struct elements, each on their own line
void xgl_descriptor_set_attach_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DESCRIPTOR_SET_ATTACH_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_descriptor_set_attach_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DESCRIPTOR_SET_ATTACH_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_pipeline_tess_state_create_info_struct_wrapper class definition
xgl_pipeline_tess_state_create_info_struct_wrapper::xgl_pipeline_tess_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_tess_state_create_info_struct_wrapper::xgl_pipeline_tess_state_create_info_struct_wrapper(XGL_PIPELINE_TESS_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_tess_state_create_info_struct_wrapper::xgl_pipeline_tess_state_create_info_struct_wrapper(const XGL_PIPELINE_TESS_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_tess_state_create_info_struct_wrapper::~xgl_pipeline_tess_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_tess_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_TESS_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_tess_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %spatchControlPoints = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.patchControlPoints));
    printf("%*s    %soptimalTessFactor = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.optimalTessFactor));
    printf("%*s    %sfixedTessFactor = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.fixedTessFactor));
}

// Output all struct elements, each on their own line
void xgl_pipeline_tess_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_TESS_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_tess_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_TESS_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_pipeline_rs_state_create_info_struct_wrapper class definition
xgl_pipeline_rs_state_create_info_struct_wrapper::xgl_pipeline_rs_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_rs_state_create_info_struct_wrapper::xgl_pipeline_rs_state_create_info_struct_wrapper(XGL_PIPELINE_RS_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_rs_state_create_info_struct_wrapper::xgl_pipeline_rs_state_create_info_struct_wrapper(const XGL_PIPELINE_RS_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_rs_state_create_info_struct_wrapper::~xgl_pipeline_rs_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_rs_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_RS_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_rs_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sdepthClipEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthClipEnable) ? "TRUE" : "FALSE");
    printf("%*s    %srasterizerDiscardEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.rasterizerDiscardEnable) ? "TRUE" : "FALSE");
    printf("%*s    %spointSize = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.pointSize));
}

// Output all struct elements, each on their own line
void xgl_pipeline_rs_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_RS_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_rs_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_RS_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_stencil_op_state_struct_wrapper class definition
xgl_stencil_op_state_struct_wrapper::xgl_stencil_op_state_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_stencil_op_state_struct_wrapper::xgl_stencil_op_state_struct_wrapper(XGL_STENCIL_OP_STATE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_stencil_op_state_struct_wrapper::xgl_stencil_op_state_struct_wrapper(const XGL_STENCIL_OP_STATE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_stencil_op_state_struct_wrapper::~xgl_stencil_op_state_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_stencil_op_state_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_STENCIL_OP_STATE = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_stencil_op_state_struct_wrapper::display_struct_members()
{
    printf("%*s    %sstencilFailOp = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STENCIL_OP(m_struct.stencilFailOp));
    printf("%*s    %sstencilPassOp = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STENCIL_OP(m_struct.stencilPassOp));
    printf("%*s    %sstencilDepthFailOp = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STENCIL_OP(m_struct.stencilDepthFailOp));
    printf("%*s    %sstencilFunc = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_COMPARE_FUNC(m_struct.stencilFunc));
    printf("%*s    %sstencilRef = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stencilRef));
}

// Output all struct elements, each on their own line
void xgl_stencil_op_state_struct_wrapper::display_txt()
{
    printf("%*sXGL_STENCIL_OP_STATE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_stencil_op_state_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_STENCIL_OP_STATE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_shader_create_info_struct_wrapper class definition
xgl_shader_create_info_struct_wrapper::xgl_shader_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_shader_create_info_struct_wrapper::xgl_shader_create_info_struct_wrapper(XGL_SHADER_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_shader_create_info_struct_wrapper::xgl_shader_create_info_struct_wrapper(const XGL_SHADER_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_shader_create_info_struct_wrapper::~xgl_shader_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_shader_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_SHADER_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_shader_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %scodeSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.codeSize));
    printf("%*s    %spCode = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pCode));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void xgl_shader_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_SHADER_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_shader_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_SHADER_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_color_blend_state_create_info_struct_wrapper class definition
xgl_color_blend_state_create_info_struct_wrapper::xgl_color_blend_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_color_blend_state_create_info_struct_wrapper::xgl_color_blend_state_create_info_struct_wrapper(XGL_COLOR_BLEND_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_color_blend_state_create_info_struct_wrapper::xgl_color_blend_state_create_info_struct_wrapper(const XGL_COLOR_BLEND_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_color_blend_state_create_info_struct_wrapper::~xgl_color_blend_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_color_blend_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_COLOR_BLEND_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_color_blend_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    uint32_t i;
    for (i = 0; i<XGL_MAX_COLOR_ATTACHMENTS; i++) {
        printf("%*s    %sattachment[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)&(m_struct.attachment)[i]);
    }
    for (i = 0; i<4; i++) {
        printf("%*s    %sblendConst[%u] = %f\n", m_indent, "", &m_dummy_prefix, i, (m_struct.blendConst)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_color_blend_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_COLOR_BLEND_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_color_blend_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_COLOR_BLEND_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<XGL_MAX_COLOR_ATTACHMENTS; i++) {
            xgl_color_attachment_blend_state_struct_wrapper class0(&(m_struct.attachment[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_pipeline_cb_state_create_info_struct_wrapper class definition
xgl_pipeline_cb_state_create_info_struct_wrapper::xgl_pipeline_cb_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_cb_state_create_info_struct_wrapper::xgl_pipeline_cb_state_create_info_struct_wrapper(XGL_PIPELINE_CB_STATE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_cb_state_create_info_struct_wrapper::xgl_pipeline_cb_state_create_info_struct_wrapper(const XGL_PIPELINE_CB_STATE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_cb_state_create_info_struct_wrapper::~xgl_pipeline_cb_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_cb_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_CB_STATE = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_cb_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %salphaToCoverageEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.alphaToCoverageEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sdualSourceBlendEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.dualSourceBlendEnable) ? "TRUE" : "FALSE");
    printf("%*s    %slogicOp = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_LOGIC_OP(m_struct.logicOp));
    uint32_t i;
    for (i = 0; i<XGL_MAX_COLOR_ATTACHMENTS; i++) {
        printf("%*s    %sattachment[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)&(m_struct.attachment)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_pipeline_cb_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_CB_STATE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_cb_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_CB_STATE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<XGL_MAX_COLOR_ATTACHMENTS; i++) {
            xgl_pipeline_cb_attachment_state_struct_wrapper class0(&(m_struct.attachment[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_channel_mapping_struct_wrapper class definition
xgl_channel_mapping_struct_wrapper::xgl_channel_mapping_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_channel_mapping_struct_wrapper::xgl_channel_mapping_struct_wrapper(XGL_CHANNEL_MAPPING* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_channel_mapping_struct_wrapper::xgl_channel_mapping_struct_wrapper(const XGL_CHANNEL_MAPPING* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_channel_mapping_struct_wrapper::~xgl_channel_mapping_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_channel_mapping_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_CHANNEL_MAPPING = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_channel_mapping_struct_wrapper::display_struct_members()
{
    printf("%*s    %sr = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_CHANNEL_SWIZZLE(m_struct.r));
    printf("%*s    %sg = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_CHANNEL_SWIZZLE(m_struct.g));
    printf("%*s    %sb = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_CHANNEL_SWIZZLE(m_struct.b));
    printf("%*s    %sa = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_CHANNEL_SWIZZLE(m_struct.a));
}

// Output all struct elements, each on their own line
void xgl_channel_mapping_struct_wrapper::display_txt()
{
    printf("%*sXGL_CHANNEL_MAPPING struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_channel_mapping_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_CHANNEL_MAPPING struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_depth_stencil_view_create_info_struct_wrapper class definition
xgl_depth_stencil_view_create_info_struct_wrapper::xgl_depth_stencil_view_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_depth_stencil_view_create_info_struct_wrapper::xgl_depth_stencil_view_create_info_struct_wrapper(XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_depth_stencil_view_create_info_struct_wrapper::xgl_depth_stencil_view_create_info_struct_wrapper(const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_depth_stencil_view_create_info_struct_wrapper::~xgl_depth_stencil_view_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_depth_stencil_view_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DEPTH_STENCIL_VIEW_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_depth_stencil_view_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %simage = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.image));
    printf("%*s    %smipLevel = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.mipLevel));
    printf("%*s    %sbaseArraySlice = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.baseArraySlice));
    printf("%*s    %sarraySize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.arraySize));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void xgl_depth_stencil_view_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DEPTH_STENCIL_VIEW_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_depth_stencil_view_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DEPTH_STENCIL_VIEW_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_virtual_memory_remap_range_struct_wrapper class definition
xgl_virtual_memory_remap_range_struct_wrapper::xgl_virtual_memory_remap_range_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_virtual_memory_remap_range_struct_wrapper::xgl_virtual_memory_remap_range_struct_wrapper(XGL_VIRTUAL_MEMORY_REMAP_RANGE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_virtual_memory_remap_range_struct_wrapper::xgl_virtual_memory_remap_range_struct_wrapper(const XGL_VIRTUAL_MEMORY_REMAP_RANGE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_virtual_memory_remap_range_struct_wrapper::~xgl_virtual_memory_remap_range_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_virtual_memory_remap_range_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_VIRTUAL_MEMORY_REMAP_RANGE = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_virtual_memory_remap_range_struct_wrapper::display_struct_members()
{
    printf("%*s    %svirtualMem = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.virtualMem));
    printf("%*s    %svirtualStartPage = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.virtualStartPage));
    printf("%*s    %srealMem = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.realMem));
    printf("%*s    %srealStartPage = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.realStartPage));
    printf("%*s    %spageCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.pageCount));
}

// Output all struct elements, each on their own line
void xgl_virtual_memory_remap_range_struct_wrapper::display_txt()
{
    printf("%*sXGL_VIRTUAL_MEMORY_REMAP_RANGE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_virtual_memory_remap_range_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_VIRTUAL_MEMORY_REMAP_RANGE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_cmd_buffer_create_info_struct_wrapper class definition
xgl_cmd_buffer_create_info_struct_wrapper::xgl_cmd_buffer_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_cmd_buffer_create_info_struct_wrapper::xgl_cmd_buffer_create_info_struct_wrapper(XGL_CMD_BUFFER_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_cmd_buffer_create_info_struct_wrapper::xgl_cmd_buffer_create_info_struct_wrapper(const XGL_CMD_BUFFER_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_cmd_buffer_create_info_struct_wrapper::~xgl_cmd_buffer_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_cmd_buffer_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_CMD_BUFFER_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_cmd_buffer_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %squeueType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_QUEUE_TYPE(m_struct.queueType));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void xgl_cmd_buffer_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_CMD_BUFFER_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_cmd_buffer_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_CMD_BUFFER_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_format_properties_struct_wrapper class definition
xgl_format_properties_struct_wrapper::xgl_format_properties_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_format_properties_struct_wrapper::xgl_format_properties_struct_wrapper(XGL_FORMAT_PROPERTIES* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_format_properties_struct_wrapper::xgl_format_properties_struct_wrapper(const XGL_FORMAT_PROPERTIES* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_format_properties_struct_wrapper::~xgl_format_properties_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_format_properties_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_FORMAT_PROPERTIES = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_format_properties_struct_wrapper::display_struct_members()
{
    printf("%*s    %slinearTilingFeatures = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.linearTilingFeatures));
    printf("%*s    %soptimalTilingFeatures = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.optimalTilingFeatures));
}

// Output all struct elements, each on their own line
void xgl_format_properties_struct_wrapper::display_txt()
{
    printf("%*sXGL_FORMAT_PROPERTIES struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_format_properties_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_FORMAT_PROPERTIES struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_physical_gpu_properties_struct_wrapper class definition
xgl_physical_gpu_properties_struct_wrapper::xgl_physical_gpu_properties_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_physical_gpu_properties_struct_wrapper::xgl_physical_gpu_properties_struct_wrapper(XGL_PHYSICAL_GPU_PROPERTIES* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_physical_gpu_properties_struct_wrapper::xgl_physical_gpu_properties_struct_wrapper(const XGL_PHYSICAL_GPU_PROPERTIES* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_physical_gpu_properties_struct_wrapper::~xgl_physical_gpu_properties_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_physical_gpu_properties_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PHYSICAL_GPU_PROPERTIES = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_physical_gpu_properties_struct_wrapper::display_struct_members()
{
    printf("%*s    %sstructSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.structSize));
    printf("%*s    %sapiVersion = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.apiVersion));
    printf("%*s    %sdriverVersion = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.driverVersion));
    printf("%*s    %svendorId = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.vendorId));
    printf("%*s    %sdeviceId = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.deviceId));
    printf("%*s    %sgpuType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_PHYSICAL_GPU_TYPE(m_struct.gpuType));
    uint32_t i;
    for (i = 0; i<XGL_MAX_PHYSICAL_GPU_NAME; i++) {
        printf("%*s    %sgpuName = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.gpuName));
    }
    printf("%*s    %smaxMemRefsPerSubmission = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxMemRefsPerSubmission));
    printf("%*s    %svirtualMemPageSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.virtualMemPageSize));
    printf("%*s    %smaxInlineMemoryUpdateSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxInlineMemoryUpdateSize));
    printf("%*s    %smaxBoundDescriptorSets = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxBoundDescriptorSets));
    printf("%*s    %smaxThreadGroupSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxThreadGroupSize));
    printf("%*s    %stimestampFrequency = %lu\n", m_indent, "", &m_dummy_prefix, (m_struct.timestampFrequency));
    printf("%*s    %smultiColorAttachmentClears = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.multiColorAttachmentClears) ? "TRUE" : "FALSE");
}

// Output all struct elements, each on their own line
void xgl_physical_gpu_properties_struct_wrapper::display_txt()
{
    printf("%*sXGL_PHYSICAL_GPU_PROPERTIES struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_physical_gpu_properties_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PHYSICAL_GPU_PROPERTIES struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_depth_stencil_bind_info_struct_wrapper class definition
xgl_depth_stencil_bind_info_struct_wrapper::xgl_depth_stencil_bind_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_depth_stencil_bind_info_struct_wrapper::xgl_depth_stencil_bind_info_struct_wrapper(XGL_DEPTH_STENCIL_BIND_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_depth_stencil_bind_info_struct_wrapper::xgl_depth_stencil_bind_info_struct_wrapper(const XGL_DEPTH_STENCIL_BIND_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_depth_stencil_bind_info_struct_wrapper::~xgl_depth_stencil_bind_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_depth_stencil_bind_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DEPTH_STENCIL_BIND_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_depth_stencil_bind_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %sview = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.view));
    printf("%*s    %sdepthState = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_STATE(m_struct.depthState));
    printf("%*s    %sstencilState = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_STATE(m_struct.stencilState));
}

// Output all struct elements, each on their own line
void xgl_depth_stencil_bind_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DEPTH_STENCIL_BIND_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_depth_stencil_bind_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DEPTH_STENCIL_BIND_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_draw_indirect_cmd_struct_wrapper class definition
xgl_draw_indirect_cmd_struct_wrapper::xgl_draw_indirect_cmd_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_draw_indirect_cmd_struct_wrapper::xgl_draw_indirect_cmd_struct_wrapper(XGL_DRAW_INDIRECT_CMD* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_draw_indirect_cmd_struct_wrapper::xgl_draw_indirect_cmd_struct_wrapper(const XGL_DRAW_INDIRECT_CMD* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_draw_indirect_cmd_struct_wrapper::~xgl_draw_indirect_cmd_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_draw_indirect_cmd_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DRAW_INDIRECT_CMD = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_draw_indirect_cmd_struct_wrapper::display_struct_members()
{
    printf("%*s    %svertexCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.vertexCount));
    printf("%*s    %sinstanceCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.instanceCount));
    printf("%*s    %sfirstVertex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.firstVertex));
    printf("%*s    %sfirstInstance = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.firstInstance));
}

// Output all struct elements, each on their own line
void xgl_draw_indirect_cmd_struct_wrapper::display_txt()
{
    printf("%*sXGL_DRAW_INDIRECT_CMD struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_draw_indirect_cmd_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DRAW_INDIRECT_CMD struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_graphics_pipeline_create_info_struct_wrapper class definition
xgl_graphics_pipeline_create_info_struct_wrapper::xgl_graphics_pipeline_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_graphics_pipeline_create_info_struct_wrapper::xgl_graphics_pipeline_create_info_struct_wrapper(XGL_GRAPHICS_PIPELINE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_graphics_pipeline_create_info_struct_wrapper::xgl_graphics_pipeline_create_info_struct_wrapper(const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_graphics_pipeline_create_info_struct_wrapper::~xgl_graphics_pipeline_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_graphics_pipeline_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_GRAPHICS_PIPELINE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_graphics_pipeline_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void xgl_graphics_pipeline_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_GRAPHICS_PIPELINE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_graphics_pipeline_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_GRAPHICS_PIPELINE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_pipeline_ia_state_create_info_struct_wrapper class definition
xgl_pipeline_ia_state_create_info_struct_wrapper::xgl_pipeline_ia_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_ia_state_create_info_struct_wrapper::xgl_pipeline_ia_state_create_info_struct_wrapper(XGL_PIPELINE_IA_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_ia_state_create_info_struct_wrapper::xgl_pipeline_ia_state_create_info_struct_wrapper(const XGL_PIPELINE_IA_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_ia_state_create_info_struct_wrapper::~xgl_pipeline_ia_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_ia_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_IA_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_ia_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %stopology = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_PRIMITIVE_TOPOLOGY(m_struct.topology));
    printf("%*s    %sdisableVertexReuse = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.disableVertexReuse) ? "TRUE" : "FALSE");
    printf("%*s    %sprovokingVertex = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_PROVOKING_VERTEX_CONVENTION(m_struct.provokingVertex));
    printf("%*s    %sprimitiveRestartEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.primitiveRestartEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sprimitiveRestartIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.primitiveRestartIndex));
}

// Output all struct elements, each on their own line
void xgl_pipeline_ia_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_IA_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_ia_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_IA_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_color_attachment_blend_state_struct_wrapper class definition
xgl_color_attachment_blend_state_struct_wrapper::xgl_color_attachment_blend_state_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_color_attachment_blend_state_struct_wrapper::xgl_color_attachment_blend_state_struct_wrapper(XGL_COLOR_ATTACHMENT_BLEND_STATE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_color_attachment_blend_state_struct_wrapper::xgl_color_attachment_blend_state_struct_wrapper(const XGL_COLOR_ATTACHMENT_BLEND_STATE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_color_attachment_blend_state_struct_wrapper::~xgl_color_attachment_blend_state_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_color_attachment_blend_state_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_COLOR_ATTACHMENT_BLEND_STATE = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_color_attachment_blend_state_struct_wrapper::display_struct_members()
{
    printf("%*s    %sblendEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.blendEnable) ? "TRUE" : "FALSE");
    printf("%*s    %ssrcBlendColor = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_BLEND(m_struct.srcBlendColor));
    printf("%*s    %sdestBlendColor = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_BLEND(m_struct.destBlendColor));
    printf("%*s    %sblendFuncColor = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_BLEND_FUNC(m_struct.blendFuncColor));
    printf("%*s    %ssrcBlendAlpha = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_BLEND(m_struct.srcBlendAlpha));
    printf("%*s    %sdestBlendAlpha = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_BLEND(m_struct.destBlendAlpha));
    printf("%*s    %sblendFuncAlpha = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_BLEND_FUNC(m_struct.blendFuncAlpha));
}

// Output all struct elements, each on their own line
void xgl_color_attachment_blend_state_struct_wrapper::display_txt()
{
    printf("%*sXGL_COLOR_ATTACHMENT_BLEND_STATE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_color_attachment_blend_state_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_COLOR_ATTACHMENT_BLEND_STATE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_extent2d_struct_wrapper class definition
xgl_extent2d_struct_wrapper::xgl_extent2d_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_extent2d_struct_wrapper::xgl_extent2d_struct_wrapper(XGL_EXTENT2D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_extent2d_struct_wrapper::xgl_extent2d_struct_wrapper(const XGL_EXTENT2D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_extent2d_struct_wrapper::~xgl_extent2d_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_extent2d_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_EXTENT2D = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_extent2d_struct_wrapper::display_struct_members()
{
    printf("%*s    %swidth = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.width));
    printf("%*s    %sheight = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.height));
}

// Output all struct elements, each on their own line
void xgl_extent2d_struct_wrapper::display_txt()
{
    printf("%*sXGL_EXTENT2D struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_extent2d_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_EXTENT2D struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_memory_alloc_info_struct_wrapper class definition
xgl_memory_alloc_info_struct_wrapper::xgl_memory_alloc_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_memory_alloc_info_struct_wrapper::xgl_memory_alloc_info_struct_wrapper(XGL_MEMORY_ALLOC_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_alloc_info_struct_wrapper::xgl_memory_alloc_info_struct_wrapper(const XGL_MEMORY_ALLOC_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_alloc_info_struct_wrapper::~xgl_memory_alloc_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_memory_alloc_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_MEMORY_ALLOC_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_memory_alloc_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sallocationSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.allocationSize));
    printf("%*s    %salignment = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.alignment));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
    printf("%*s    %sheapCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.heapCount));
    uint32_t i;
    for (i = 0; i<XGL_MAX_MEMORY_HEAPS; i++) {
        printf("%*s    %sheaps[%u] = %u\n", m_indent, "", &m_dummy_prefix, i, (m_struct.heaps)[i]);
    }
    printf("%*s    %smemPriority = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_MEMORY_PRIORITY(m_struct.memPriority));
}

// Output all struct elements, each on their own line
void xgl_memory_alloc_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_MEMORY_ALLOC_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_memory_alloc_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_MEMORY_ALLOC_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_memory_ref_struct_wrapper class definition
xgl_memory_ref_struct_wrapper::xgl_memory_ref_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_memory_ref_struct_wrapper::xgl_memory_ref_struct_wrapper(XGL_MEMORY_REF* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_ref_struct_wrapper::xgl_memory_ref_struct_wrapper(const XGL_MEMORY_REF* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_ref_struct_wrapper::~xgl_memory_ref_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_memory_ref_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_MEMORY_REF = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_memory_ref_struct_wrapper::display_struct_members()
{
    printf("%*s    %smem = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.mem));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void xgl_memory_ref_struct_wrapper::display_txt()
{
    printf("%*sXGL_MEMORY_REF struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_memory_ref_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_MEMORY_REF struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_query_pool_create_info_struct_wrapper class definition
xgl_query_pool_create_info_struct_wrapper::xgl_query_pool_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_query_pool_create_info_struct_wrapper::xgl_query_pool_create_info_struct_wrapper(XGL_QUERY_POOL_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_query_pool_create_info_struct_wrapper::xgl_query_pool_create_info_struct_wrapper(const XGL_QUERY_POOL_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_query_pool_create_info_struct_wrapper::~xgl_query_pool_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_query_pool_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_QUERY_POOL_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_query_pool_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %squeryType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_QUERY_TYPE(m_struct.queryType));
    printf("%*s    %sslots = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.slots));
}

// Output all struct elements, each on their own line
void xgl_query_pool_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_QUERY_POOL_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_query_pool_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_QUERY_POOL_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_offset3d_struct_wrapper class definition
xgl_offset3d_struct_wrapper::xgl_offset3d_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_offset3d_struct_wrapper::xgl_offset3d_struct_wrapper(XGL_OFFSET3D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_offset3d_struct_wrapper::xgl_offset3d_struct_wrapper(const XGL_OFFSET3D* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_offset3d_struct_wrapper::~xgl_offset3d_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_offset3d_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_OFFSET3D = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_offset3d_struct_wrapper::display_struct_members()
{
    printf("%*s    %sx = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.x));
    printf("%*s    %sy = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.y));
    printf("%*s    %sz = %i\n", m_indent, "", &m_dummy_prefix, (m_struct.z));
}

// Output all struct elements, each on their own line
void xgl_offset3d_struct_wrapper::display_txt()
{
    printf("%*sXGL_OFFSET3D struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_offset3d_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_OFFSET3D struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_pipeline_shader_stage_create_info_struct_wrapper class definition
xgl_pipeline_shader_stage_create_info_struct_wrapper::xgl_pipeline_shader_stage_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_shader_stage_create_info_struct_wrapper::xgl_pipeline_shader_stage_create_info_struct_wrapper(XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_shader_stage_create_info_struct_wrapper::xgl_pipeline_shader_stage_create_info_struct_wrapper(const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_shader_stage_create_info_struct_wrapper::~xgl_pipeline_shader_stage_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_shader_stage_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_SHADER_STAGE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_shader_stage_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sshader = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.shader));
}

// Output all struct elements, each on their own line
void xgl_pipeline_shader_stage_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_SHADER_STAGE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_shader_stage_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_SHADER_STAGE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.shader) {
        xgl_pipeline_shader_struct_wrapper class0(&m_struct.shader);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_memory_view_attach_info_struct_wrapper class definition
xgl_memory_view_attach_info_struct_wrapper::xgl_memory_view_attach_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_memory_view_attach_info_struct_wrapper::xgl_memory_view_attach_info_struct_wrapper(XGL_MEMORY_VIEW_ATTACH_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_view_attach_info_struct_wrapper::xgl_memory_view_attach_info_struct_wrapper(const XGL_MEMORY_VIEW_ATTACH_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_view_attach_info_struct_wrapper::~xgl_memory_view_attach_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_memory_view_attach_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_MEMORY_VIEW_ATTACH_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_memory_view_attach_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %smem = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.mem));
    printf("%*s    %soffset = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.offset));
    printf("%*s    %srange = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.range));
    printf("%*s    %sstride = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stride));
    printf("%*s    %sformat = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.format));
    printf("%*s    %sstate = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_MEMORY_STATE(m_struct.state));
}

// Output all struct elements, each on their own line
void xgl_memory_view_attach_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_MEMORY_VIEW_ATTACH_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_memory_view_attach_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_MEMORY_VIEW_ATTACH_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.format) {
        xgl_format_struct_wrapper class0(&m_struct.format);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_dispatch_indirect_cmd_struct_wrapper class definition
xgl_dispatch_indirect_cmd_struct_wrapper::xgl_dispatch_indirect_cmd_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_dispatch_indirect_cmd_struct_wrapper::xgl_dispatch_indirect_cmd_struct_wrapper(XGL_DISPATCH_INDIRECT_CMD* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_dispatch_indirect_cmd_struct_wrapper::xgl_dispatch_indirect_cmd_struct_wrapper(const XGL_DISPATCH_INDIRECT_CMD* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_dispatch_indirect_cmd_struct_wrapper::~xgl_dispatch_indirect_cmd_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_dispatch_indirect_cmd_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DISPATCH_INDIRECT_CMD = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_dispatch_indirect_cmd_struct_wrapper::display_struct_members()
{
    printf("%*s    %sx = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.x));
    printf("%*s    %sy = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.y));
    printf("%*s    %sz = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.z));
}

// Output all struct elements, each on their own line
void xgl_dispatch_indirect_cmd_struct_wrapper::display_txt()
{
    printf("%*sXGL_DISPATCH_INDIRECT_CMD struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_dispatch_indirect_cmd_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DISPATCH_INDIRECT_CMD struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

//any footer info for class
