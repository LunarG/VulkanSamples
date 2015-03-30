//This is the copyright
//#includes, #defines, globals and such...
#include <stdio.h>
#include <xgl_struct_wrappers.h>
#include <xgl_enum_string_helper.h>

void dynamic_display_full_txt(const void* pStruct, uint32_t indent)
{
    // Cast to APP_INFO ptr initially just to pull sType off struct
    XGL_STRUCTURE_TYPE sType = ((XGL_APPLICATION_INFO*)pStruct)->sType;

    switch (sType)
    {
        case XGL_STRUCTURE_TYPE_APPLICATION_INFO:
        {
            xgl_application_info_struct_wrapper swc0((XGL_APPLICATION_INFO*)pStruct);
            swc0.set_indent(indent);
            swc0.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_BUFFER_CREATE_INFO:
        {
            xgl_buffer_create_info_struct_wrapper swc1((XGL_BUFFER_CREATE_INFO*)pStruct);
            swc1.set_indent(indent);
            swc1.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
        {
            xgl_buffer_memory_barrier_struct_wrapper swc2((XGL_BUFFER_MEMORY_BARRIER*)pStruct);
            swc2.set_indent(indent);
            swc2.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO:
        {
            xgl_buffer_view_attach_info_struct_wrapper swc3((XGL_BUFFER_VIEW_ATTACH_INFO*)pStruct);
            swc3.set_indent(indent);
            swc3.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO:
        {
            xgl_buffer_view_create_info_struct_wrapper swc4((XGL_BUFFER_VIEW_CREATE_INFO*)pStruct);
            swc4.set_indent(indent);
            swc4.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO:
        {
            xgl_cmd_buffer_begin_info_struct_wrapper swc5((XGL_CMD_BUFFER_BEGIN_INFO*)pStruct);
            swc5.set_indent(indent);
            swc5.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO:
        {
            xgl_cmd_buffer_create_info_struct_wrapper swc6((XGL_CMD_BUFFER_CREATE_INFO*)pStruct);
            swc6.set_indent(indent);
            swc6.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:
        {
            xgl_cmd_buffer_graphics_begin_info_struct_wrapper swc7((XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO*)pStruct);
            swc7.set_indent(indent);
            swc7.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO:
        {
            xgl_color_attachment_view_create_info_struct_wrapper swc8((XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO*)pStruct);
            swc8.set_indent(indent);
            swc8.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
        {
            xgl_compute_pipeline_create_info_struct_wrapper swc9((XGL_COMPUTE_PIPELINE_CREATE_INFO*)pStruct);
            swc9.set_indent(indent);
            swc9.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO:
        {
            xgl_depth_stencil_view_create_info_struct_wrapper swc10((XGL_DEPTH_STENCIL_VIEW_CREATE_INFO*)pStruct);
            swc10.set_indent(indent);
            swc10.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_DESCRIPTOR_REGION_CREATE_INFO:
        {
            xgl_descriptor_region_create_info_struct_wrapper swc11((XGL_DESCRIPTOR_REGION_CREATE_INFO*)pStruct);
            swc11.set_indent(indent);
            swc11.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:
        {
            xgl_descriptor_set_layout_create_info_struct_wrapper swc12((XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pStruct);
            swc12.set_indent(indent);
            swc12.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO:
        {
            xgl_device_create_info_struct_wrapper swc13((XGL_DEVICE_CREATE_INFO*)pStruct);
            swc13.set_indent(indent);
            swc13.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO:
        {
            xgl_dynamic_cb_state_create_info_struct_wrapper swc14((XGL_DYNAMIC_CB_STATE_CREATE_INFO*)pStruct);
            swc14.set_indent(indent);
            swc14.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO:
        {
            xgl_dynamic_ds_state_create_info_struct_wrapper swc15((XGL_DYNAMIC_DS_STATE_CREATE_INFO*)pStruct);
            swc15.set_indent(indent);
            swc15.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO:
        {
            xgl_dynamic_rs_state_create_info_struct_wrapper swc16((XGL_DYNAMIC_RS_STATE_CREATE_INFO*)pStruct);
            swc16.set_indent(indent);
            swc16.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO:
        {
            xgl_dynamic_vp_state_create_info_struct_wrapper swc17((XGL_DYNAMIC_VP_STATE_CREATE_INFO*)pStruct);
            swc17.set_indent(indent);
            swc17.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO:
        {
            xgl_event_create_info_struct_wrapper swc18((XGL_EVENT_CREATE_INFO*)pStruct);
            swc18.set_indent(indent);
            swc18.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_EVENT_WAIT_INFO:
        {
            xgl_event_wait_info_struct_wrapper swc19((XGL_EVENT_WAIT_INFO*)pStruct);
            swc19.set_indent(indent);
            swc19.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO:
        {
            xgl_fence_create_info_struct_wrapper swc20((XGL_FENCE_CREATE_INFO*)pStruct);
            swc20.set_indent(indent);
            swc20.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO:
        {
            xgl_framebuffer_create_info_struct_wrapper swc21((XGL_FRAMEBUFFER_CREATE_INFO*)pStruct);
            swc21.set_indent(indent);
            swc21.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
        {
            xgl_graphics_pipeline_create_info_struct_wrapper swc22((XGL_GRAPHICS_PIPELINE_CREATE_INFO*)pStruct);
            swc22.set_indent(indent);
            swc22.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
        {
            xgl_image_create_info_struct_wrapper swc23((XGL_IMAGE_CREATE_INFO*)pStruct);
            swc23.set_indent(indent);
            swc23.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
        {
            xgl_image_memory_barrier_struct_wrapper swc24((XGL_IMAGE_MEMORY_BARRIER*)pStruct);
            swc24.set_indent(indent);
            swc24.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO:
        {
            xgl_image_view_attach_info_struct_wrapper swc25((XGL_IMAGE_VIEW_ATTACH_INFO*)pStruct);
            swc25.set_indent(indent);
            swc25.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO:
        {
            xgl_image_view_create_info_struct_wrapper swc26((XGL_IMAGE_VIEW_CREATE_INFO*)pStruct);
            swc26.set_indent(indent);
            swc26.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO:
        {
            xgl_layer_create_info_struct_wrapper swc27((XGL_LAYER_CREATE_INFO*)pStruct);
            swc27.set_indent(indent);
            swc27.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:
        {
            xgl_memory_alloc_buffer_info_struct_wrapper swc28((XGL_MEMORY_ALLOC_BUFFER_INFO*)pStruct);
            swc28.set_indent(indent);
            swc28.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:
        {
            xgl_memory_alloc_image_info_struct_wrapper swc29((XGL_MEMORY_ALLOC_IMAGE_INFO*)pStruct);
            swc29.set_indent(indent);
            swc29.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO:
        {
            xgl_memory_alloc_info_struct_wrapper swc30((XGL_MEMORY_ALLOC_INFO*)pStruct);
            swc30.set_indent(indent);
            swc30.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_MEMORY_BARRIER:
        {
            xgl_memory_barrier_struct_wrapper swc31((XGL_MEMORY_BARRIER*)pStruct);
            swc31.set_indent(indent);
            swc31.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_MEMORY_OPEN_INFO:
        {
            xgl_memory_open_info_struct_wrapper swc32((XGL_MEMORY_OPEN_INFO*)pStruct);
            swc32.set_indent(indent);
            swc32.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO:
        {
            xgl_peer_memory_open_info_struct_wrapper swc33((XGL_PEER_MEMORY_OPEN_INFO*)pStruct);
            swc33.set_indent(indent);
            swc33.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_BARRIER:
        {
            xgl_pipeline_barrier_struct_wrapper swc34((XGL_PIPELINE_BARRIER*)pStruct);
            swc34.set_indent(indent);
            swc34.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
        {
            xgl_pipeline_cb_state_create_info_struct_wrapper swc35((XGL_PIPELINE_CB_STATE_CREATE_INFO*)pStruct);
            swc35.set_indent(indent);
            swc35.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO:
        {
            xgl_pipeline_ds_state_create_info_struct_wrapper swc36((XGL_PIPELINE_DS_STATE_CREATE_INFO*)pStruct);
            swc36.set_indent(indent);
            swc36.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
        {
            xgl_pipeline_ia_state_create_info_struct_wrapper swc37((XGL_PIPELINE_IA_STATE_CREATE_INFO*)pStruct);
            swc37.set_indent(indent);
            swc37.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO:
        {
            xgl_pipeline_ms_state_create_info_struct_wrapper swc38((XGL_PIPELINE_MS_STATE_CREATE_INFO*)pStruct);
            swc38.set_indent(indent);
            swc38.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
        {
            xgl_pipeline_rs_state_create_info_struct_wrapper swc39((XGL_PIPELINE_RS_STATE_CREATE_INFO*)pStruct);
            swc39.set_indent(indent);
            swc39.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
        {
            xgl_pipeline_shader_stage_create_info_struct_wrapper swc40((XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pStruct);
            swc40.set_indent(indent);
            swc40.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
        {
            xgl_pipeline_tess_state_create_info_struct_wrapper swc41((XGL_PIPELINE_TESS_STATE_CREATE_INFO*)pStruct);
            swc41.set_indent(indent);
            swc41.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:
        {
            xgl_pipeline_vertex_input_create_info_struct_wrapper swc42((XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO*)pStruct);
            swc42.set_indent(indent);
            swc42.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO:
        {
            xgl_pipeline_vp_state_create_info_struct_wrapper swc43((XGL_PIPELINE_VP_STATE_CREATE_INFO*)pStruct);
            swc43.set_indent(indent);
            swc43.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
        {
            xgl_query_pool_create_info_struct_wrapper swc44((XGL_QUERY_POOL_CREATE_INFO*)pStruct);
            swc44.set_indent(indent);
            swc44.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_CREATE_INFO:
        {
            xgl_queue_semaphore_create_info_struct_wrapper swc45((XGL_QUEUE_SEMAPHORE_CREATE_INFO*)pStruct);
            swc45.set_indent(indent);
            swc45.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_OPEN_INFO:
        {
            xgl_queue_semaphore_open_info_struct_wrapper swc46((XGL_QUEUE_SEMAPHORE_OPEN_INFO*)pStruct);
            swc46.set_indent(indent);
            swc46.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO:
        {
            xgl_render_pass_create_info_struct_wrapper swc47((XGL_RENDER_PASS_CREATE_INFO*)pStruct);
            swc47.set_indent(indent);
            swc47.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO:
        {
            xgl_sampler_create_info_struct_wrapper swc48((XGL_SAMPLER_CREATE_INFO*)pStruct);
            swc48.set_indent(indent);
            swc48.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO:
        {
            xgl_shader_create_info_struct_wrapper swc49((XGL_SHADER_CREATE_INFO*)pStruct);
            swc49.set_indent(indent);
            swc49.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
        {
            xgl_update_as_copy_struct_wrapper swc50((XGL_UPDATE_AS_COPY*)pStruct);
            swc50.set_indent(indent);
            swc50.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
        {
            xgl_update_buffers_struct_wrapper swc51((XGL_UPDATE_BUFFERS*)pStruct);
            swc51.set_indent(indent);
            swc51.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
        {
            xgl_update_images_struct_wrapper swc52((XGL_UPDATE_IMAGES*)pStruct);
            swc52.set_indent(indent);
            swc52.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
        {
            xgl_update_samplers_struct_wrapper swc53((XGL_UPDATE_SAMPLERS*)pStruct);
            swc53.set_indent(indent);
            swc53.display_full_txt();
        }
        break;
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
        {
            xgl_update_sampler_textures_struct_wrapper swc54((XGL_UPDATE_SAMPLER_TEXTURES*)pStruct);
            swc54.set_indent(indent);
            swc54.display_full_txt();
        }
        break;
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


// xgl_buffer_copy_struct_wrapper class definition
xgl_buffer_copy_struct_wrapper::xgl_buffer_copy_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_buffer_copy_struct_wrapper::xgl_buffer_copy_struct_wrapper(XGL_BUFFER_COPY* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_buffer_copy_struct_wrapper::xgl_buffer_copy_struct_wrapper(const XGL_BUFFER_COPY* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_buffer_copy_struct_wrapper::~xgl_buffer_copy_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_buffer_copy_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_BUFFER_COPY = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_buffer_copy_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssrcOffset = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.srcOffset));
    printf("%*s    %sdestOffset = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.destOffset));
    printf("%*s    %scopySize = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.copySize));
}

// Output all struct elements, each on their own line
void xgl_buffer_copy_struct_wrapper::display_txt()
{
    printf("%*sXGL_BUFFER_COPY struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_buffer_copy_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_BUFFER_COPY struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_buffer_create_info_struct_wrapper class definition
xgl_buffer_create_info_struct_wrapper::xgl_buffer_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_buffer_create_info_struct_wrapper::xgl_buffer_create_info_struct_wrapper(XGL_BUFFER_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_buffer_create_info_struct_wrapper::xgl_buffer_create_info_struct_wrapper(const XGL_BUFFER_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_buffer_create_info_struct_wrapper::~xgl_buffer_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_buffer_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_BUFFER_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_buffer_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %ssize = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.size));
    printf("%*s    %susage = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.usage));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void xgl_buffer_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_BUFFER_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_buffer_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_BUFFER_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_buffer_image_copy_struct_wrapper class definition
xgl_buffer_image_copy_struct_wrapper::xgl_buffer_image_copy_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_buffer_image_copy_struct_wrapper::xgl_buffer_image_copy_struct_wrapper(XGL_BUFFER_IMAGE_COPY* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_buffer_image_copy_struct_wrapper::xgl_buffer_image_copy_struct_wrapper(const XGL_BUFFER_IMAGE_COPY* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_buffer_image_copy_struct_wrapper::~xgl_buffer_image_copy_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_buffer_image_copy_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_BUFFER_IMAGE_COPY = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_buffer_image_copy_struct_wrapper::display_struct_members()
{
    printf("%*s    %sbufferOffset = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.bufferOffset));
    printf("%*s    %simageSubresource = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.imageSubresource));
    printf("%*s    %simageOffset = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.imageOffset));
    printf("%*s    %simageExtent = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.imageExtent));
}

// Output all struct elements, each on their own line
void xgl_buffer_image_copy_struct_wrapper::display_txt()
{
    printf("%*sXGL_BUFFER_IMAGE_COPY struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_buffer_image_copy_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_BUFFER_IMAGE_COPY struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
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


// xgl_buffer_memory_barrier_struct_wrapper class definition
xgl_buffer_memory_barrier_struct_wrapper::xgl_buffer_memory_barrier_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_buffer_memory_barrier_struct_wrapper::xgl_buffer_memory_barrier_struct_wrapper(XGL_BUFFER_MEMORY_BARRIER* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_buffer_memory_barrier_struct_wrapper::xgl_buffer_memory_barrier_struct_wrapper(const XGL_BUFFER_MEMORY_BARRIER* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_buffer_memory_barrier_struct_wrapper::~xgl_buffer_memory_barrier_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_buffer_memory_barrier_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_BUFFER_MEMORY_BARRIER = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_buffer_memory_barrier_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %soutputMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.outputMask));
    printf("%*s    %sinputMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.inputMask));
    printf("%*s    %sbuffer = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.buffer));
    printf("%*s    %soffset = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.offset));
    printf("%*s    %ssize = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.size));
}

// Output all struct elements, each on their own line
void xgl_buffer_memory_barrier_struct_wrapper::display_txt()
{
    printf("%*sXGL_BUFFER_MEMORY_BARRIER struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_buffer_memory_barrier_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_BUFFER_MEMORY_BARRIER struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_buffer_memory_requirements_struct_wrapper class definition
xgl_buffer_memory_requirements_struct_wrapper::xgl_buffer_memory_requirements_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_buffer_memory_requirements_struct_wrapper::xgl_buffer_memory_requirements_struct_wrapper(XGL_BUFFER_MEMORY_REQUIREMENTS* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_buffer_memory_requirements_struct_wrapper::xgl_buffer_memory_requirements_struct_wrapper(const XGL_BUFFER_MEMORY_REQUIREMENTS* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_buffer_memory_requirements_struct_wrapper::~xgl_buffer_memory_requirements_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_buffer_memory_requirements_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_BUFFER_MEMORY_REQUIREMENTS = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_buffer_memory_requirements_struct_wrapper::display_struct_members()
{
    printf("%*s    %susage = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.usage));
}

// Output all struct elements, each on their own line
void xgl_buffer_memory_requirements_struct_wrapper::display_txt()
{
    printf("%*sXGL_BUFFER_MEMORY_REQUIREMENTS struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_buffer_memory_requirements_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_BUFFER_MEMORY_REQUIREMENTS struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_buffer_view_attach_info_struct_wrapper class definition
xgl_buffer_view_attach_info_struct_wrapper::xgl_buffer_view_attach_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_buffer_view_attach_info_struct_wrapper::xgl_buffer_view_attach_info_struct_wrapper(XGL_BUFFER_VIEW_ATTACH_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_buffer_view_attach_info_struct_wrapper::xgl_buffer_view_attach_info_struct_wrapper(const XGL_BUFFER_VIEW_ATTACH_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_buffer_view_attach_info_struct_wrapper::~xgl_buffer_view_attach_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_buffer_view_attach_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_BUFFER_VIEW_ATTACH_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_buffer_view_attach_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sview = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.view));
}

// Output all struct elements, each on their own line
void xgl_buffer_view_attach_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_BUFFER_VIEW_ATTACH_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_buffer_view_attach_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_BUFFER_VIEW_ATTACH_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_buffer_view_create_info_struct_wrapper class definition
xgl_buffer_view_create_info_struct_wrapper::xgl_buffer_view_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_buffer_view_create_info_struct_wrapper::xgl_buffer_view_create_info_struct_wrapper(XGL_BUFFER_VIEW_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_buffer_view_create_info_struct_wrapper::xgl_buffer_view_create_info_struct_wrapper(const XGL_BUFFER_VIEW_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_buffer_view_create_info_struct_wrapper::~xgl_buffer_view_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_buffer_view_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_BUFFER_VIEW_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_buffer_view_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sbuffer = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.buffer));
    printf("%*s    %sviewType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_BUFFER_VIEW_TYPE(m_struct.viewType));
    printf("%*s    %sstride = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.stride));
    printf("%*s    %sformat = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_FORMAT(m_struct.format));
    printf("%*s    %schannels = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.channels));
    printf("%*s    %soffset = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.offset));
    printf("%*s    %srange = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.range));
}

// Output all struct elements, each on their own line
void xgl_buffer_view_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_BUFFER_VIEW_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_buffer_view_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_BUFFER_VIEW_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.channels) {
        xgl_channel_mapping_struct_wrapper class0(&m_struct.channels);
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


// xgl_clear_color_struct_wrapper class definition
xgl_clear_color_struct_wrapper::xgl_clear_color_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_clear_color_struct_wrapper::xgl_clear_color_struct_wrapper(XGL_CLEAR_COLOR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_clear_color_struct_wrapper::xgl_clear_color_struct_wrapper(const XGL_CLEAR_COLOR* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_clear_color_struct_wrapper::~xgl_clear_color_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_clear_color_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_CLEAR_COLOR = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_clear_color_struct_wrapper::display_struct_members()
{
    printf("%*s    %scolor = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.color));
    printf("%*s    %suseRawValue = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.useRawValue) ? "TRUE" : "FALSE");
}

// Output all struct elements, each on their own line
void xgl_clear_color_struct_wrapper::display_txt()
{
    printf("%*sXGL_CLEAR_COLOR struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_clear_color_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_CLEAR_COLOR struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.color) {
        xgl_clear_color_value_struct_wrapper class0(&m_struct.color);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
}


// xgl_clear_color_value_struct_wrapper class definition
xgl_clear_color_value_struct_wrapper::xgl_clear_color_value_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_clear_color_value_struct_wrapper::xgl_clear_color_value_struct_wrapper(XGL_CLEAR_COLOR_VALUE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_clear_color_value_struct_wrapper::xgl_clear_color_value_struct_wrapper(const XGL_CLEAR_COLOR_VALUE* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_clear_color_value_struct_wrapper::~xgl_clear_color_value_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_clear_color_value_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_CLEAR_COLOR_VALUE = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_clear_color_value_struct_wrapper::display_struct_members()
{
    uint32_t i;
    for (i = 0; i<4; i++) {
        printf("%*s    %sfloatColor[%u] = %f\n", m_indent, "", &m_dummy_prefix, i, (m_struct.floatColor)[i]);
    }
    for (i = 0; i<4; i++) {
        printf("%*s    %srawColor[%u] = %u\n", m_indent, "", &m_dummy_prefix, i, (m_struct.rawColor)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_clear_color_value_struct_wrapper::display_txt()
{
    printf("%*sXGL_CLEAR_COLOR_VALUE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_clear_color_value_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_CLEAR_COLOR_VALUE struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_cmd_buffer_begin_info_struct_wrapper class definition
xgl_cmd_buffer_begin_info_struct_wrapper::xgl_cmd_buffer_begin_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_cmd_buffer_begin_info_struct_wrapper::xgl_cmd_buffer_begin_info_struct_wrapper(XGL_CMD_BUFFER_BEGIN_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_cmd_buffer_begin_info_struct_wrapper::xgl_cmd_buffer_begin_info_struct_wrapper(const XGL_CMD_BUFFER_BEGIN_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_cmd_buffer_begin_info_struct_wrapper::~xgl_cmd_buffer_begin_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_cmd_buffer_begin_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_CMD_BUFFER_BEGIN_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_cmd_buffer_begin_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sflags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.flags));
}

// Output all struct elements, each on their own line
void xgl_cmd_buffer_begin_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_CMD_BUFFER_BEGIN_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_cmd_buffer_begin_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_CMD_BUFFER_BEGIN_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
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


// xgl_cmd_buffer_graphics_begin_info_struct_wrapper class definition
xgl_cmd_buffer_graphics_begin_info_struct_wrapper::xgl_cmd_buffer_graphics_begin_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_cmd_buffer_graphics_begin_info_struct_wrapper::xgl_cmd_buffer_graphics_begin_info_struct_wrapper(XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_cmd_buffer_graphics_begin_info_struct_wrapper::xgl_cmd_buffer_graphics_begin_info_struct_wrapper(const XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_cmd_buffer_graphics_begin_info_struct_wrapper::~xgl_cmd_buffer_graphics_begin_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_cmd_buffer_graphics_begin_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_cmd_buffer_graphics_begin_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %srenderPass = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.renderPass));
}

// Output all struct elements, each on their own line
void xgl_cmd_buffer_graphics_begin_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_cmd_buffer_graphics_begin_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
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
    printf("%*s    %slayout = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_LAYOUT(m_struct.layout));
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
    printf("%*s    %sformat = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_FORMAT(m_struct.format));
    printf("%*s    %smipLevel = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.mipLevel));
    printf("%*s    %sbaseArraySlice = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.baseArraySlice));
    printf("%*s    %sarraySize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.arraySize));
    printf("%*s    %smsaaResolveImage = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.msaaResolveImage));
    printf("%*s    %smsaaResolveSubResource = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.msaaResolveSubResource));
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
    if (&m_struct.msaaResolveSubResource) {
        xgl_image_subresource_range_struct_wrapper class0(&m_struct.msaaResolveSubResource);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
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
    printf("%*s    %slastSetLayout = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.lastSetLayout));
    printf("%*s    %slocalSizeX = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.localSizeX));
    printf("%*s    %slocalSizeY = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.localSizeY));
    printf("%*s    %slocalSizeZ = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.localSizeZ));
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
    printf("%*s    %slayout = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_LAYOUT(m_struct.layout));
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
    printf("%*s    %smsaaResolveImage = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.msaaResolveImage));
    printf("%*s    %smsaaResolveSubResource = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.msaaResolveSubResource));
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
    if (&m_struct.msaaResolveSubResource) {
        xgl_image_subresource_range_struct_wrapper class0(&m_struct.msaaResolveSubResource);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_descriptor_region_create_info_struct_wrapper class definition
xgl_descriptor_region_create_info_struct_wrapper::xgl_descriptor_region_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_descriptor_region_create_info_struct_wrapper::xgl_descriptor_region_create_info_struct_wrapper(XGL_DESCRIPTOR_REGION_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_descriptor_region_create_info_struct_wrapper::xgl_descriptor_region_create_info_struct_wrapper(const XGL_DESCRIPTOR_REGION_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_descriptor_region_create_info_struct_wrapper::~xgl_descriptor_region_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_descriptor_region_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DESCRIPTOR_REGION_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_descriptor_region_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %scount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.count));
    uint32_t i;
    for (i = 0; i<count; i++) {
        printf("%*s    %spTypeCount[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pTypeCount)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_descriptor_region_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DESCRIPTOR_REGION_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_descriptor_region_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DESCRIPTOR_REGION_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<count; i++) {
            xgl_descriptor_type_count_struct_wrapper class0(&(m_struct.pTypeCount[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_descriptor_set_layout_create_info_struct_wrapper class definition
xgl_descriptor_set_layout_create_info_struct_wrapper::xgl_descriptor_set_layout_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_descriptor_set_layout_create_info_struct_wrapper::xgl_descriptor_set_layout_create_info_struct_wrapper(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_descriptor_set_layout_create_info_struct_wrapper::xgl_descriptor_set_layout_create_info_struct_wrapper(const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_descriptor_set_layout_create_info_struct_wrapper::~xgl_descriptor_set_layout_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_descriptor_set_layout_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_descriptor_set_layout_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sdescriptorType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_DESCRIPTOR_TYPE(m_struct.descriptorType));
    printf("%*s    %scount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.count));
    printf("%*s    %sstageFlags = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stageFlags));
    printf("%*s    %simmutableSampler = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.immutableSampler));
}

// Output all struct elements, each on their own line
void xgl_descriptor_set_layout_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_descriptor_set_layout_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_descriptor_type_count_struct_wrapper class definition
xgl_descriptor_type_count_struct_wrapper::xgl_descriptor_type_count_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_descriptor_type_count_struct_wrapper::xgl_descriptor_type_count_struct_wrapper(XGL_DESCRIPTOR_TYPE_COUNT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_descriptor_type_count_struct_wrapper::xgl_descriptor_type_count_struct_wrapper(const XGL_DESCRIPTOR_TYPE_COUNT* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_descriptor_type_count_struct_wrapper::~xgl_descriptor_type_count_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_descriptor_type_count_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DESCRIPTOR_TYPE_COUNT = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_descriptor_type_count_struct_wrapper::display_struct_members()
{
    printf("%*s    %stype = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_DESCRIPTOR_TYPE(m_struct.type));
    printf("%*s    %scount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.count));
}

// Output all struct elements, each on their own line
void xgl_descriptor_type_count_struct_wrapper::display_txt()
{
    printf("%*sXGL_DESCRIPTOR_TYPE_COUNT struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_descriptor_type_count_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DESCRIPTOR_TYPE_COUNT struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
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
    uint32_t i;
    for (i = 0; i<queueRecordCount; i++) {
        printf("%*s    %spRequestedQueues[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pRequestedQueues)[i]);
    }
    printf("%*s    %sextensionCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.extensionCount));
    for (i = 0; i<extensionCount; i++) {
        printf("%*s    %sppEnabledExtensionNames = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.ppEnabledExtensionNames)[0]);
    }
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
    uint32_t i;
    for (i = 0; i<queueRecordCount; i++) {
            xgl_device_queue_create_info_struct_wrapper class0(&(m_struct.pRequestedQueues[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
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


// xgl_dynamic_cb_state_create_info_struct_wrapper class definition
xgl_dynamic_cb_state_create_info_struct_wrapper::xgl_dynamic_cb_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_dynamic_cb_state_create_info_struct_wrapper::xgl_dynamic_cb_state_create_info_struct_wrapper(XGL_DYNAMIC_CB_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_dynamic_cb_state_create_info_struct_wrapper::xgl_dynamic_cb_state_create_info_struct_wrapper(const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_dynamic_cb_state_create_info_struct_wrapper::~xgl_dynamic_cb_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_dynamic_cb_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DYNAMIC_CB_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_dynamic_cb_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    uint32_t i;
    for (i = 0; i<4; i++) {
        printf("%*s    %sblendConst[%u] = %f\n", m_indent, "", &m_dummy_prefix, i, (m_struct.blendConst)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_dynamic_cb_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DYNAMIC_CB_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_dynamic_cb_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DYNAMIC_CB_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_dynamic_ds_state_create_info_struct_wrapper class definition
xgl_dynamic_ds_state_create_info_struct_wrapper::xgl_dynamic_ds_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_dynamic_ds_state_create_info_struct_wrapper::xgl_dynamic_ds_state_create_info_struct_wrapper(XGL_DYNAMIC_DS_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_dynamic_ds_state_create_info_struct_wrapper::xgl_dynamic_ds_state_create_info_struct_wrapper(const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_dynamic_ds_state_create_info_struct_wrapper::~xgl_dynamic_ds_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_dynamic_ds_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DYNAMIC_DS_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_dynamic_ds_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sminDepth = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.minDepth));
    printf("%*s    %smaxDepth = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDepth));
    printf("%*s    %sstencilReadMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stencilReadMask));
    printf("%*s    %sstencilWriteMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stencilWriteMask));
    printf("%*s    %sstencilFrontRef = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stencilFrontRef));
    printf("%*s    %sstencilBackRef = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stencilBackRef));
}

// Output all struct elements, each on their own line
void xgl_dynamic_ds_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DYNAMIC_DS_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_dynamic_ds_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DYNAMIC_DS_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_dynamic_rs_state_create_info_struct_wrapper class definition
xgl_dynamic_rs_state_create_info_struct_wrapper::xgl_dynamic_rs_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_dynamic_rs_state_create_info_struct_wrapper::xgl_dynamic_rs_state_create_info_struct_wrapper(XGL_DYNAMIC_RS_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_dynamic_rs_state_create_info_struct_wrapper::xgl_dynamic_rs_state_create_info_struct_wrapper(const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_dynamic_rs_state_create_info_struct_wrapper::~xgl_dynamic_rs_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_dynamic_rs_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DYNAMIC_RS_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_dynamic_rs_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sdepthBias = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.depthBias));
    printf("%*s    %sdepthBiasClamp = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.depthBiasClamp));
    printf("%*s    %sslopeScaledDepthBias = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.slopeScaledDepthBias));
    printf("%*s    %spointSize = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.pointSize));
    printf("%*s    %spointFadeThreshold = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.pointFadeThreshold));
    printf("%*s    %slineWidth = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.lineWidth));
}

// Output all struct elements, each on their own line
void xgl_dynamic_rs_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DYNAMIC_RS_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_dynamic_rs_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DYNAMIC_RS_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_dynamic_vp_state_create_info_struct_wrapper class definition
xgl_dynamic_vp_state_create_info_struct_wrapper::xgl_dynamic_vp_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_dynamic_vp_state_create_info_struct_wrapper::xgl_dynamic_vp_state_create_info_struct_wrapper(XGL_DYNAMIC_VP_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_dynamic_vp_state_create_info_struct_wrapper::xgl_dynamic_vp_state_create_info_struct_wrapper(const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_dynamic_vp_state_create_info_struct_wrapper::~xgl_dynamic_vp_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_dynamic_vp_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_DYNAMIC_VP_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_dynamic_vp_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sviewportAndScissorCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.viewportAndScissorCount));
    uint32_t i;
    for (i = 0; i<viewportAndScissorCount; i++) {
        printf("%*s    %spViewports[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pViewports)[i]);
    }
    for (i = 0; i<viewportAndScissorCount; i++) {
        printf("%*s    %spScissors[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pScissors)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_dynamic_vp_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_DYNAMIC_VP_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_dynamic_vp_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_DYNAMIC_VP_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<viewportAndScissorCount; i++) {
            xgl_rect_struct_wrapper class0(&(m_struct.pScissors[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    for (i = 0; i<viewportAndScissorCount; i++) {
            xgl_viewport_struct_wrapper class1(&(m_struct.pViewports[i]));
            class1.set_indent(m_indent + 4);
            class1.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
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


// xgl_event_wait_info_struct_wrapper class definition
xgl_event_wait_info_struct_wrapper::xgl_event_wait_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_event_wait_info_struct_wrapper::xgl_event_wait_info_struct_wrapper(XGL_EVENT_WAIT_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_event_wait_info_struct_wrapper::xgl_event_wait_info_struct_wrapper(const XGL_EVENT_WAIT_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_event_wait_info_struct_wrapper::~xgl_event_wait_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_event_wait_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_EVENT_WAIT_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_event_wait_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %seventCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.eventCount));
    uint32_t i;
    for (i = 0; i<eventCount; i++) {
        printf("%*s    %spEvents[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pEvents)[i]);
    }
    printf("%*s    %swaitEvent = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_WAIT_EVENT(m_struct.waitEvent));
    printf("%*s    %smemBarrierCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.memBarrierCount));
    for (i = 0; i<memBarrierCount; i++) {
        printf("%*s    %sppMemBarriers[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.ppMemBarriers)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_event_wait_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_EVENT_WAIT_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_event_wait_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_EVENT_WAIT_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
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


// xgl_framebuffer_create_info_struct_wrapper class definition
xgl_framebuffer_create_info_struct_wrapper::xgl_framebuffer_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_framebuffer_create_info_struct_wrapper::xgl_framebuffer_create_info_struct_wrapper(XGL_FRAMEBUFFER_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_framebuffer_create_info_struct_wrapper::xgl_framebuffer_create_info_struct_wrapper(const XGL_FRAMEBUFFER_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_framebuffer_create_info_struct_wrapper::~xgl_framebuffer_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_framebuffer_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_FRAMEBUFFER_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_framebuffer_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %scolorAttachmentCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.colorAttachmentCount));
    uint32_t i;
    for (i = 0; i<colorAttachmentCount; i++) {
        printf("%*s    %spColorAttachments[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pColorAttachments)[i]);
    }
    printf("%*s    %spDepthStencilAttachment = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pDepthStencilAttachment));
    printf("%*s    %ssampleCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.sampleCount));
    printf("%*s    %swidth = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.width));
    printf("%*s    %sheight = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.height));
    printf("%*s    %slayers = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.layers));
}

// Output all struct elements, each on their own line
void xgl_framebuffer_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_FRAMEBUFFER_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_framebuffer_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_FRAMEBUFFER_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pDepthStencilAttachment) {
        xgl_depth_stencil_bind_info_struct_wrapper class0(m_struct.pDepthStencilAttachment);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    uint32_t i;
    for (i = 0; i<colorAttachmentCount; i++) {
            xgl_color_attachment_bind_info_struct_wrapper class1(&(m_struct.pColorAttachments[i]));
            class1.set_indent(m_indent + 4);
            class1.display_full_txt();
    }
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
    printf("%*s    %slastSetLayout = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.lastSetLayout));
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
    printf("%*s    %sformat = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_FORMAT(m_struct.format));
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
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_image_memory_barrier_struct_wrapper class definition
xgl_image_memory_barrier_struct_wrapper::xgl_image_memory_barrier_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_image_memory_barrier_struct_wrapper::xgl_image_memory_barrier_struct_wrapper(XGL_IMAGE_MEMORY_BARRIER* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_memory_barrier_struct_wrapper::xgl_image_memory_barrier_struct_wrapper(const XGL_IMAGE_MEMORY_BARRIER* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_memory_barrier_struct_wrapper::~xgl_image_memory_barrier_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_image_memory_barrier_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_IMAGE_MEMORY_BARRIER = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_image_memory_barrier_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %soutputMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.outputMask));
    printf("%*s    %sinputMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.inputMask));
    printf("%*s    %soldLayout = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_LAYOUT(m_struct.oldLayout));
    printf("%*s    %snewLayout = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_LAYOUT(m_struct.newLayout));
    printf("%*s    %simage = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.image));
    printf("%*s    %ssubresourceRange = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.subresourceRange));
}

// Output all struct elements, each on their own line
void xgl_image_memory_barrier_struct_wrapper::display_txt()
{
    printf("%*sXGL_IMAGE_MEMORY_BARRIER struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_image_memory_barrier_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_IMAGE_MEMORY_BARRIER struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.subresourceRange) {
        xgl_image_subresource_range_struct_wrapper class0(&m_struct.subresourceRange);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_image_memory_bind_info_struct_wrapper class definition
xgl_image_memory_bind_info_struct_wrapper::xgl_image_memory_bind_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_image_memory_bind_info_struct_wrapper::xgl_image_memory_bind_info_struct_wrapper(XGL_IMAGE_MEMORY_BIND_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_memory_bind_info_struct_wrapper::xgl_image_memory_bind_info_struct_wrapper(const XGL_IMAGE_MEMORY_BIND_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_memory_bind_info_struct_wrapper::~xgl_image_memory_bind_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_image_memory_bind_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_IMAGE_MEMORY_BIND_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_image_memory_bind_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssubresource = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.subresource));
    printf("%*s    %soffset = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.offset));
    printf("%*s    %sextent = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.extent));
}

// Output all struct elements, each on their own line
void xgl_image_memory_bind_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_IMAGE_MEMORY_BIND_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_image_memory_bind_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_IMAGE_MEMORY_BIND_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (&m_struct.extent) {
        xgl_extent3d_struct_wrapper class0(&m_struct.extent);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
    if (&m_struct.offset) {
        xgl_offset3d_struct_wrapper class1(&m_struct.offset);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (&m_struct.subresource) {
        xgl_image_subresource_struct_wrapper class2(&m_struct.subresource);
        class2.set_indent(m_indent + 4);
        class2.display_full_txt();
    }
}


// xgl_image_memory_requirements_struct_wrapper class definition
xgl_image_memory_requirements_struct_wrapper::xgl_image_memory_requirements_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_image_memory_requirements_struct_wrapper::xgl_image_memory_requirements_struct_wrapper(XGL_IMAGE_MEMORY_REQUIREMENTS* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_memory_requirements_struct_wrapper::xgl_image_memory_requirements_struct_wrapper(const XGL_IMAGE_MEMORY_REQUIREMENTS* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_image_memory_requirements_struct_wrapper::~xgl_image_memory_requirements_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_image_memory_requirements_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_IMAGE_MEMORY_REQUIREMENTS = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_image_memory_requirements_struct_wrapper::display_struct_members()
{
    printf("%*s    %susage = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.usage));
    printf("%*s    %sformatClass = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_FORMAT_CLASS(m_struct.formatClass));
    printf("%*s    %ssamples = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.samples));
}

// Output all struct elements, each on their own line
void xgl_image_memory_requirements_struct_wrapper::display_txt()
{
    printf("%*sXGL_IMAGE_MEMORY_REQUIREMENTS struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_image_memory_requirements_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_IMAGE_MEMORY_REQUIREMENTS struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
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
    printf("%*s    %slayout = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_LAYOUT(m_struct.layout));
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
    printf("%*s    %sformat = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_FORMAT(m_struct.format));
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
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_layer_create_info_struct_wrapper class definition
xgl_layer_create_info_struct_wrapper::xgl_layer_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_layer_create_info_struct_wrapper::xgl_layer_create_info_struct_wrapper(XGL_LAYER_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_layer_create_info_struct_wrapper::xgl_layer_create_info_struct_wrapper(const XGL_LAYER_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_layer_create_info_struct_wrapper::~xgl_layer_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_layer_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_LAYER_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_layer_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %slayerCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.layerCount));
    uint32_t i;
    for (i = 0; i<layerCount; i++) {
        printf("%*s    %sppActiveLayerNames = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.ppActiveLayerNames)[0]);
    }
}

// Output all struct elements, each on their own line
void xgl_layer_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_LAYER_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_layer_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_LAYER_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
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
    printf("%*s    %sbufferSize = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.bufferSize));
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


// xgl_memory_alloc_buffer_info_struct_wrapper class definition
xgl_memory_alloc_buffer_info_struct_wrapper::xgl_memory_alloc_buffer_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_memory_alloc_buffer_info_struct_wrapper::xgl_memory_alloc_buffer_info_struct_wrapper(XGL_MEMORY_ALLOC_BUFFER_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_alloc_buffer_info_struct_wrapper::xgl_memory_alloc_buffer_info_struct_wrapper(const XGL_MEMORY_ALLOC_BUFFER_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_alloc_buffer_info_struct_wrapper::~xgl_memory_alloc_buffer_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_memory_alloc_buffer_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_MEMORY_ALLOC_BUFFER_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_memory_alloc_buffer_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %susage = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.usage));
}

// Output all struct elements, each on their own line
void xgl_memory_alloc_buffer_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_MEMORY_ALLOC_BUFFER_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_memory_alloc_buffer_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_MEMORY_ALLOC_BUFFER_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_memory_alloc_image_info_struct_wrapper class definition
xgl_memory_alloc_image_info_struct_wrapper::xgl_memory_alloc_image_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_memory_alloc_image_info_struct_wrapper::xgl_memory_alloc_image_info_struct_wrapper(XGL_MEMORY_ALLOC_IMAGE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_alloc_image_info_struct_wrapper::xgl_memory_alloc_image_info_struct_wrapper(const XGL_MEMORY_ALLOC_IMAGE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_alloc_image_info_struct_wrapper::~xgl_memory_alloc_image_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_memory_alloc_image_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_MEMORY_ALLOC_IMAGE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_memory_alloc_image_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %susage = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.usage));
    printf("%*s    %sformatClass = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_IMAGE_FORMAT_CLASS(m_struct.formatClass));
    printf("%*s    %ssamples = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.samples));
}

// Output all struct elements, each on their own line
void xgl_memory_alloc_image_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_MEMORY_ALLOC_IMAGE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_memory_alloc_image_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_MEMORY_ALLOC_IMAGE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
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
    printf("%*s    %sallocationSize = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.allocationSize));
    printf("%*s    %smemProps = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.memProps));
    printf("%*s    %smemType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_MEMORY_TYPE(m_struct.memType));
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


// xgl_memory_barrier_struct_wrapper class definition
xgl_memory_barrier_struct_wrapper::xgl_memory_barrier_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_memory_barrier_struct_wrapper::xgl_memory_barrier_struct_wrapper(XGL_MEMORY_BARRIER* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_barrier_struct_wrapper::xgl_memory_barrier_struct_wrapper(const XGL_MEMORY_BARRIER* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_memory_barrier_struct_wrapper::~xgl_memory_barrier_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_memory_barrier_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_MEMORY_BARRIER = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_memory_barrier_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %soutputMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.outputMask));
    printf("%*s    %sinputMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.inputMask));
}

// Output all struct elements, each on their own line
void xgl_memory_barrier_struct_wrapper::display_txt()
{
    printf("%*sXGL_MEMORY_BARRIER struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_memory_barrier_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_MEMORY_BARRIER struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
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
    printf("%*s    %ssize = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.size));
    printf("%*s    %salignment = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.alignment));
    printf("%*s    %sgranularity = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.granularity));
    printf("%*s    %smemProps = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.memProps));
    printf("%*s    %smemType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_MEMORY_TYPE(m_struct.memType));
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
    printf("%*s    %ssupportsMigration = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.supportsMigration) ? "TRUE" : "FALSE");
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
    printf("%*s    %smaxInlineMemoryUpdateSize = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.maxInlineMemoryUpdateSize));
    printf("%*s    %smaxBoundDescriptorSets = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxBoundDescriptorSets));
    printf("%*s    %smaxThreadGroupSize = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxThreadGroupSize));
    printf("%*s    %stimestampFrequency = %lu\n", m_indent, "", &m_dummy_prefix, (m_struct.timestampFrequency));
    printf("%*s    %smultiColorAttachmentClears = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.multiColorAttachmentClears) ? "TRUE" : "FALSE");
    printf("%*s    %smaxDescriptorSets = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxDescriptorSets));
    printf("%*s    %smaxViewports = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxViewports));
    printf("%*s    %smaxColorAttachments = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.maxColorAttachments));
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


// xgl_pipeline_barrier_struct_wrapper class definition
xgl_pipeline_barrier_struct_wrapper::xgl_pipeline_barrier_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_barrier_struct_wrapper::xgl_pipeline_barrier_struct_wrapper(XGL_PIPELINE_BARRIER* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_barrier_struct_wrapper::xgl_pipeline_barrier_struct_wrapper(const XGL_PIPELINE_BARRIER* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_barrier_struct_wrapper::~xgl_pipeline_barrier_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_barrier_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_BARRIER = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_barrier_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %seventCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.eventCount));
    uint32_t i;
    for (i = 0; i<eventCount; i++) {
        printf("%*s    %spEvents[%u] = %s\n", m_indent, "", &m_dummy_prefix, i, string_XGL_SET_EVENT(*m_struct.pEvents)[i]);
    }
    printf("%*s    %swaitEvent = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_WAIT_EVENT(m_struct.waitEvent));
    printf("%*s    %smemBarrierCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.memBarrierCount));
    for (i = 0; i<memBarrierCount; i++) {
        printf("%*s    %sppMemBarriers[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.ppMemBarriers)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_pipeline_barrier_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_BARRIER struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_barrier_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_BARRIER struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
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
    printf("%*s    %sformat = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_FORMAT(m_struct.format));
    printf("%*s    %ssrcBlendColor = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_BLEND(m_struct.srcBlendColor));
    printf("%*s    %sdestBlendColor = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_BLEND(m_struct.destBlendColor));
    printf("%*s    %sblendFuncColor = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_BLEND_FUNC(m_struct.blendFuncColor));
    printf("%*s    %ssrcBlendAlpha = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_BLEND(m_struct.srcBlendAlpha));
    printf("%*s    %sdestBlendAlpha = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_BLEND(m_struct.destBlendAlpha));
    printf("%*s    %sblendFuncAlpha = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_BLEND_FUNC(m_struct.blendFuncAlpha));
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
}


// xgl_pipeline_cb_state_create_info_struct_wrapper class definition
xgl_pipeline_cb_state_create_info_struct_wrapper::xgl_pipeline_cb_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_cb_state_create_info_struct_wrapper::xgl_pipeline_cb_state_create_info_struct_wrapper(XGL_PIPELINE_CB_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_cb_state_create_info_struct_wrapper::xgl_pipeline_cb_state_create_info_struct_wrapper(const XGL_PIPELINE_CB_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_cb_state_create_info_struct_wrapper::~xgl_pipeline_cb_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_cb_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_CB_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_cb_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %salphaToCoverageEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.alphaToCoverageEnable) ? "TRUE" : "FALSE");
    printf("%*s    %slogicOpEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.logicOpEnable) ? "TRUE" : "FALSE");
    printf("%*s    %slogicOp = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_LOGIC_OP(m_struct.logicOp));
    printf("%*s    %sattachmentCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.attachmentCount));
    uint32_t i;
    for (i = 0; i<attachmentCount; i++) {
        printf("%*s    %spAttachments[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pAttachments)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_pipeline_cb_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_CB_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_cb_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_CB_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<attachmentCount; i++) {
            xgl_pipeline_cb_attachment_state_struct_wrapper class0(&(m_struct.pAttachments[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_pipeline_ds_state_create_info_struct_wrapper class definition
xgl_pipeline_ds_state_create_info_struct_wrapper::xgl_pipeline_ds_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_ds_state_create_info_struct_wrapper::xgl_pipeline_ds_state_create_info_struct_wrapper(XGL_PIPELINE_DS_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_ds_state_create_info_struct_wrapper::xgl_pipeline_ds_state_create_info_struct_wrapper(const XGL_PIPELINE_DS_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_ds_state_create_info_struct_wrapper::~xgl_pipeline_ds_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_ds_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_DS_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_ds_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sformat = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_FORMAT(m_struct.format));
    printf("%*s    %sdepthTestEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthTestEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sdepthWriteEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthWriteEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sdepthFunc = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_COMPARE_FUNC(m_struct.depthFunc));
    printf("%*s    %sdepthBoundsEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.depthBoundsEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sstencilTestEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.stencilTestEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sfront = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.front));
    printf("%*s    %sback = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.back));
}

// Output all struct elements, each on their own line
void xgl_pipeline_ds_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_DS_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_ds_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_DS_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
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


// xgl_pipeline_ms_state_create_info_struct_wrapper class definition
xgl_pipeline_ms_state_create_info_struct_wrapper::xgl_pipeline_ms_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_ms_state_create_info_struct_wrapper::xgl_pipeline_ms_state_create_info_struct_wrapper(XGL_PIPELINE_MS_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_ms_state_create_info_struct_wrapper::xgl_pipeline_ms_state_create_info_struct_wrapper(const XGL_PIPELINE_MS_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_ms_state_create_info_struct_wrapper::~xgl_pipeline_ms_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_ms_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_MS_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_ms_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %ssamples = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.samples));
    printf("%*s    %smultisampleEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.multisampleEnable) ? "TRUE" : "FALSE");
    printf("%*s    %ssampleShadingEnable = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.sampleShadingEnable) ? "TRUE" : "FALSE");
    printf("%*s    %sminSampleShading = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.minSampleShading));
    printf("%*s    %ssampleMask = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.sampleMask));
}

// Output all struct elements, each on their own line
void xgl_pipeline_ms_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_MS_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_ms_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_MS_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
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
    printf("%*s    %sprogramPointSize = %s\n", m_indent, "", &m_dummy_prefix, (m_struct.programPointSize) ? "TRUE" : "FALSE");
    printf("%*s    %spointOrigin = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_COORDINATE_ORIGIN(m_struct.pointOrigin));
    printf("%*s    %sprovokingVertex = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_PROVOKING_VERTEX_CONVENTION(m_struct.provokingVertex));
    printf("%*s    %sfillMode = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_FILL_MODE(m_struct.fillMode));
    printf("%*s    %scullMode = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_CULL_MODE(m_struct.cullMode));
    printf("%*s    %sfrontFace = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_FACE_ORIENTATION(m_struct.frontFace));
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
    printf("%*s    %slinkConstBufferCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.linkConstBufferCount));
    uint32_t i;
    for (i = 0; i<linkConstBufferCount; i++) {
        printf("%*s    %spLinkConstBufferInfo[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pLinkConstBufferInfo)[i]);
    }
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
    uint32_t i;
    for (i = 0; i<linkConstBufferCount; i++) {
            xgl_link_const_buffer_struct_wrapper class0(&(m_struct.pLinkConstBufferInfo[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
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


// xgl_pipeline_vertex_input_create_info_struct_wrapper class definition
xgl_pipeline_vertex_input_create_info_struct_wrapper::xgl_pipeline_vertex_input_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_vertex_input_create_info_struct_wrapper::xgl_pipeline_vertex_input_create_info_struct_wrapper(XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_vertex_input_create_info_struct_wrapper::xgl_pipeline_vertex_input_create_info_struct_wrapper(const XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_vertex_input_create_info_struct_wrapper::~xgl_pipeline_vertex_input_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_vertex_input_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_VERTEX_INPUT_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_vertex_input_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sbindingCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.bindingCount));
    uint32_t i;
    for (i = 0; i<bindingCount; i++) {
        printf("%*s    %spVertexBindingDescriptions[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pVertexBindingDescriptions)[i]);
    }
    printf("%*s    %sattributeCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.attributeCount));
    for (i = 0; i<attributeCount; i++) {
        printf("%*s    %spVertexAttributeDescriptions[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pVertexAttributeDescriptions)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_pipeline_vertex_input_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_VERTEX_INPUT_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_vertex_input_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_VERTEX_INPUT_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<attributeCount; i++) {
            xgl_vertex_input_attribute_description_struct_wrapper class0(&(m_struct.pVertexAttributeDescriptions[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    for (i = 0; i<bindingCount; i++) {
            xgl_vertex_input_binding_description_struct_wrapper class1(&(m_struct.pVertexBindingDescriptions[i]));
            class1.set_indent(m_indent + 4);
            class1.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_pipeline_vp_state_create_info_struct_wrapper class definition
xgl_pipeline_vp_state_create_info_struct_wrapper::xgl_pipeline_vp_state_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_pipeline_vp_state_create_info_struct_wrapper::xgl_pipeline_vp_state_create_info_struct_wrapper(XGL_PIPELINE_VP_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_vp_state_create_info_struct_wrapper::xgl_pipeline_vp_state_create_info_struct_wrapper(const XGL_PIPELINE_VP_STATE_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_pipeline_vp_state_create_info_struct_wrapper::~xgl_pipeline_vp_state_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_pipeline_vp_state_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_PIPELINE_VP_STATE_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_pipeline_vp_state_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %snumViewports = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.numViewports));
    printf("%*s    %sclipOrigin = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_COORDINATE_ORIGIN(m_struct.clipOrigin));
    printf("%*s    %sdepthMode = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_DEPTH_MODE(m_struct.depthMode));
}

// Output all struct elements, each on their own line
void xgl_pipeline_vp_state_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_PIPELINE_VP_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_pipeline_vp_state_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_PIPELINE_VP_STATE_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
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


// xgl_render_pass_create_info_struct_wrapper class definition
xgl_render_pass_create_info_struct_wrapper::xgl_render_pass_create_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_render_pass_create_info_struct_wrapper::xgl_render_pass_create_info_struct_wrapper(XGL_RENDER_PASS_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_render_pass_create_info_struct_wrapper::xgl_render_pass_create_info_struct_wrapper(const XGL_RENDER_PASS_CREATE_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_render_pass_create_info_struct_wrapper::~xgl_render_pass_create_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_render_pass_create_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_RENDER_PASS_CREATE_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_render_pass_create_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %srenderArea = %p\n", m_indent, "", &m_dummy_prefix, (void*)&(m_struct.renderArea));
    printf("%*s    %sframebuffer = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.framebuffer));
    printf("%*s    %scolorAttachmentCount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.colorAttachmentCount));
    uint32_t i;
    for (i = 0; i<colorAttachmentCount; i++) {
        printf("%*s    %spColorLoadOps[%u] = %s\n", m_indent, "", &m_dummy_prefix, i, string_XGL_ATTACHMENT_LOAD_OP(*m_struct.pColorLoadOps)[i]);
    }
    for (i = 0; i<colorAttachmentCount; i++) {
        printf("%*s    %spColorStoreOps[%u] = %s\n", m_indent, "", &m_dummy_prefix, i, string_XGL_ATTACHMENT_STORE_OP(*m_struct.pColorStoreOps)[i]);
    }
    for (i = 0; i<colorAttachmentCount; i++) {
        printf("%*s    %spColorLoadClearValues[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pColorLoadClearValues)[i]);
    }
    printf("%*s    %sdepthLoadOp = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_ATTACHMENT_LOAD_OP(m_struct.depthLoadOp));
    printf("%*s    %sdepthLoadClearValue = %f\n", m_indent, "", &m_dummy_prefix, (m_struct.depthLoadClearValue));
    printf("%*s    %sdepthStoreOp = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_ATTACHMENT_STORE_OP(m_struct.depthStoreOp));
    printf("%*s    %sstencilLoadOp = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_ATTACHMENT_LOAD_OP(m_struct.stencilLoadOp));
    printf("%*s    %sstencilLoadClearValue = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.stencilLoadClearValue));
    printf("%*s    %sstencilStoreOp = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_ATTACHMENT_STORE_OP(m_struct.stencilStoreOp));
}

// Output all struct elements, each on their own line
void xgl_render_pass_create_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_RENDER_PASS_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_render_pass_create_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_RENDER_PASS_CREATE_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<colorAttachmentCount; i++) {
            xgl_clear_color_struct_wrapper class0(&(m_struct.pColorLoadClearValues[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (&m_struct.renderArea) {
        xgl_rect_struct_wrapper class1(&m_struct.renderArea);
        class1.set_indent(m_indent + 4);
        class1.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
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


// xgl_sampler_image_view_info_struct_wrapper class definition
xgl_sampler_image_view_info_struct_wrapper::xgl_sampler_image_view_info_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_sampler_image_view_info_struct_wrapper::xgl_sampler_image_view_info_struct_wrapper(XGL_SAMPLER_IMAGE_VIEW_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_sampler_image_view_info_struct_wrapper::xgl_sampler_image_view_info_struct_wrapper(const XGL_SAMPLER_IMAGE_VIEW_INFO* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_sampler_image_view_info_struct_wrapper::~xgl_sampler_image_view_info_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_sampler_image_view_info_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_SAMPLER_IMAGE_VIEW_INFO = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_sampler_image_view_info_struct_wrapper::display_struct_members()
{
    printf("%*s    %spSampler = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pSampler));
    printf("%*s    %spImageView = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.pImageView));
}

// Output all struct elements, each on their own line
void xgl_sampler_image_view_info_struct_wrapper::display_txt()
{
    printf("%*sXGL_SAMPLER_IMAGE_VIEW_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_sampler_image_view_info_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_SAMPLER_IMAGE_VIEW_INFO struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pImageView) {
        xgl_image_view_attach_info_struct_wrapper class0(m_struct.pImageView);
        class0.set_indent(m_indent + 4);
        class0.display_full_txt();
    }
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
    printf("%*s    %scodeSize = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.codeSize));
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
    printf("%*s    %soffset = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.offset));
    printf("%*s    %ssize = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.size));
    printf("%*s    %srowPitch = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.rowPitch));
    printf("%*s    %sdepthPitch = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.depthPitch));
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


// xgl_update_as_copy_struct_wrapper class definition
xgl_update_as_copy_struct_wrapper::xgl_update_as_copy_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_update_as_copy_struct_wrapper::xgl_update_as_copy_struct_wrapper(XGL_UPDATE_AS_COPY* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_update_as_copy_struct_wrapper::xgl_update_as_copy_struct_wrapper(const XGL_UPDATE_AS_COPY* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_update_as_copy_struct_wrapper::~xgl_update_as_copy_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_update_as_copy_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_UPDATE_AS_COPY = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_update_as_copy_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sdescriptorType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_DESCRIPTOR_TYPE(m_struct.descriptorType));
    printf("%*s    %sdescriptorSet = %p\n", m_indent, "", &m_dummy_prefix, (void*)(m_struct.descriptorSet));
    printf("%*s    %sdescriptorIndex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.descriptorIndex));
    printf("%*s    %scount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.count));
}

// Output all struct elements, each on their own line
void xgl_update_as_copy_struct_wrapper::display_txt()
{
    printf("%*sXGL_UPDATE_AS_COPY struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_update_as_copy_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_UPDATE_AS_COPY struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_update_buffers_struct_wrapper class definition
xgl_update_buffers_struct_wrapper::xgl_update_buffers_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_update_buffers_struct_wrapper::xgl_update_buffers_struct_wrapper(XGL_UPDATE_BUFFERS* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_update_buffers_struct_wrapper::xgl_update_buffers_struct_wrapper(const XGL_UPDATE_BUFFERS* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_update_buffers_struct_wrapper::~xgl_update_buffers_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_update_buffers_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_UPDATE_BUFFERS = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_update_buffers_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sdescriptorType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_DESCRIPTOR_TYPE(m_struct.descriptorType));
    printf("%*s    %sindex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.index));
    printf("%*s    %scount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.count));
    uint32_t i;
    for (i = 0; i<count; i++) {
        printf("%*s    %spBufferViews[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pBufferViews)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_update_buffers_struct_wrapper::display_txt()
{
    printf("%*sXGL_UPDATE_BUFFERS struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_update_buffers_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_UPDATE_BUFFERS struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<count; i++) {
            xgl_buffer_view_attach_info_struct_wrapper class0(&(m_struct.pBufferViews[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_update_images_struct_wrapper class definition
xgl_update_images_struct_wrapper::xgl_update_images_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_update_images_struct_wrapper::xgl_update_images_struct_wrapper(XGL_UPDATE_IMAGES* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_update_images_struct_wrapper::xgl_update_images_struct_wrapper(const XGL_UPDATE_IMAGES* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_update_images_struct_wrapper::~xgl_update_images_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_update_images_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_UPDATE_IMAGES = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_update_images_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sdescriptorType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_DESCRIPTOR_TYPE(m_struct.descriptorType));
    printf("%*s    %sindex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.index));
    printf("%*s    %scount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.count));
    uint32_t i;
    for (i = 0; i<count; i++) {
        printf("%*s    %spImageViews[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pImageViews)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_update_images_struct_wrapper::display_txt()
{
    printf("%*sXGL_UPDATE_IMAGES struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_update_images_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_UPDATE_IMAGES struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<count; i++) {
            xgl_image_view_attach_info_struct_wrapper class0(&(m_struct.pImageViews[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_update_samplers_struct_wrapper class definition
xgl_update_samplers_struct_wrapper::xgl_update_samplers_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_update_samplers_struct_wrapper::xgl_update_samplers_struct_wrapper(XGL_UPDATE_SAMPLERS* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_update_samplers_struct_wrapper::xgl_update_samplers_struct_wrapper(const XGL_UPDATE_SAMPLERS* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_update_samplers_struct_wrapper::~xgl_update_samplers_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_update_samplers_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_UPDATE_SAMPLERS = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_update_samplers_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sindex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.index));
    printf("%*s    %scount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.count));
    uint32_t i;
    for (i = 0; i<count; i++) {
        printf("%*s    %spSamplers[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (m_struct.pSamplers)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_update_samplers_struct_wrapper::display_txt()
{
    printf("%*sXGL_UPDATE_SAMPLERS struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_update_samplers_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_UPDATE_SAMPLERS struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_update_sampler_textures_struct_wrapper class definition
xgl_update_sampler_textures_struct_wrapper::xgl_update_sampler_textures_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_update_sampler_textures_struct_wrapper::xgl_update_sampler_textures_struct_wrapper(XGL_UPDATE_SAMPLER_TEXTURES* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_update_sampler_textures_struct_wrapper::xgl_update_sampler_textures_struct_wrapper(const XGL_UPDATE_SAMPLER_TEXTURES* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_update_sampler_textures_struct_wrapper::~xgl_update_sampler_textures_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_update_sampler_textures_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_UPDATE_SAMPLER_TEXTURES = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_update_sampler_textures_struct_wrapper::display_struct_members()
{
    printf("%*s    %ssType = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_STRUCTURE_TYPE(m_struct.sType));
    printf("%*s    %spNext = %p\n", m_indent, "", &m_dummy_prefix, (m_struct.pNext));
    printf("%*s    %sindex = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.index));
    printf("%*s    %scount = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.count));
    uint32_t i;
    for (i = 0; i<count; i++) {
        printf("%*s    %spSamplerImageViews[%u] = %p\n", m_indent, "", &m_dummy_prefix, i, (void*)(m_struct.pSamplerImageViews)[i]);
    }
}

// Output all struct elements, each on their own line
void xgl_update_sampler_textures_struct_wrapper::display_txt()
{
    printf("%*sXGL_UPDATE_SAMPLER_TEXTURES struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_update_sampler_textures_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_UPDATE_SAMPLER_TEXTURES struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
    uint32_t i;
    for (i = 0; i<count; i++) {
            xgl_sampler_image_view_info_struct_wrapper class0(&(m_struct.pSamplerImageViews[i]));
            class0.set_indent(m_indent + 4);
            class0.display_full_txt();
    }
    if (m_struct.pNext) {
        dynamic_display_full_txt(m_struct.pNext, m_indent);
    }
}


// xgl_vertex_input_attribute_description_struct_wrapper class definition
xgl_vertex_input_attribute_description_struct_wrapper::xgl_vertex_input_attribute_description_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_vertex_input_attribute_description_struct_wrapper::xgl_vertex_input_attribute_description_struct_wrapper(XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_vertex_input_attribute_description_struct_wrapper::xgl_vertex_input_attribute_description_struct_wrapper(const XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_vertex_input_attribute_description_struct_wrapper::~xgl_vertex_input_attribute_description_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_vertex_input_attribute_description_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_vertex_input_attribute_description_struct_wrapper::display_struct_members()
{
    printf("%*s    %sbinding = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.binding));
    printf("%*s    %sformat = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_FORMAT(m_struct.format));
    printf("%*s    %soffsetInBytes = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.offsetInBytes));
}

// Output all struct elements, each on their own line
void xgl_vertex_input_attribute_description_struct_wrapper::display_txt()
{
    printf("%*sXGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_vertex_input_attribute_description_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}


// xgl_vertex_input_binding_description_struct_wrapper class definition
xgl_vertex_input_binding_description_struct_wrapper::xgl_vertex_input_binding_description_struct_wrapper() : m_struct(), m_indent(0), m_dummy_prefix('\0'), m_origStructAddr(NULL) {}
xgl_vertex_input_binding_description_struct_wrapper::xgl_vertex_input_binding_description_struct_wrapper(XGL_VERTEX_INPUT_BINDING_DESCRIPTION* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_vertex_input_binding_description_struct_wrapper::xgl_vertex_input_binding_description_struct_wrapper(const XGL_VERTEX_INPUT_BINDING_DESCRIPTION* pInStruct) : m_indent(0), m_dummy_prefix('\0')
{
    m_struct = *pInStruct;
    m_origStructAddr = pInStruct;
}
xgl_vertex_input_binding_description_struct_wrapper::~xgl_vertex_input_binding_description_struct_wrapper() {}
// Output 'structname = struct_address' on a single line
void xgl_vertex_input_binding_description_struct_wrapper::display_single_txt()
{
    printf(" %*sXGL_VERTEX_INPUT_BINDING_DESCRIPTION = %p", m_indent, "", (void*)m_origStructAddr);
}

// Private helper function that displays the members of the wrapped struct
void xgl_vertex_input_binding_description_struct_wrapper::display_struct_members()
{
    printf("%*s    %sstrideInBytes = %u\n", m_indent, "", &m_dummy_prefix, (m_struct.strideInBytes));
    printf("%*s    %sstepRate = %s\n", m_indent, "", &m_dummy_prefix, string_XGL_VERTEX_INPUT_STEP_RATE(m_struct.stepRate));
}

// Output all struct elements, each on their own line
void xgl_vertex_input_binding_description_struct_wrapper::display_txt()
{
    printf("%*sXGL_VERTEX_INPUT_BINDING_DESCRIPTION struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
}

// Output all struct elements, and for any structs pointed to, print complete contents
void xgl_vertex_input_binding_description_struct_wrapper::display_full_txt()
{
    printf("%*sXGL_VERTEX_INPUT_BINDING_DESCRIPTION struct contents at %p:\n", m_indent, "", (void*)m_origStructAddr);
    this->display_struct_members();
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

//any footer info for class
