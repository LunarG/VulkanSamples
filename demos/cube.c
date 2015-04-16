#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <xcb/xcb.h>
#include <vulkan.h>
#include <vkDbg.h>
#include <vkWsiX11Ext.h>

#include "icd-spv.h"

#include "linmath.h"
#include <png.h>

#define DEMO_BUFFER_COUNT 2
#define DEMO_TEXTURE_COUNT 1

/*
 * structure to track all objects related to a texture.
 */
struct texture_object {
    VkSampler sampler;

    VkImage image;
    VkImageLayout imageLayout;

    uint32_t  num_mem;
    VkDeviceMemory *mem;
    VkImageView view;
    int32_t tex_width, tex_height;
};

static char *tex_files[] = {
    "lunarg-logo-256x256-solid.png"
};

struct vkcube_vs_uniform {
    // Must start with MVP
    float       mvp[4][4];
    float       position[12*3][4];
    float       color[12*3][4];
};

struct vktexcube_vs_uniform {
    // Must start with MVP
    float       mvp[4][4];
    float       position[12*3][4];
    float       attr[12*3][4];
};

//--------------------------------------------------------------------------------------
// Mesh and VertexFormat Data
//--------------------------------------------------------------------------------------
struct Vertex
{
    float     posX, posY, posZ, posW;    // Position data
    float     r, g, b, a;                // Color
};

struct VertexPosTex
{
    float     posX, posY, posZ, posW;    // Position data
    float     u, v, s, t;                // Texcoord
};

#define XYZ1(_x_, _y_, _z_)         (_x_), (_y_), (_z_), 1.f
#define UV(_u_, _v_)                (_u_), (_v_), 0.f, 1.f

static const float g_vertex_buffer_data[] = {
    -1.0f,-1.0f,-1.0f,  // Vertex 0
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,

    -1.0f, 1.0f, 1.0f,  // Vertex 1
    -1.0f, 1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // Vertex 2
     1.0f, 1.0f,-1.0f,
     1.0f,-1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // Vertex 3
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // Vertex 4
     1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,

    -1.0f,-1.0f,-1.0f,  // Vertex 5
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,

    -1.0f, 1.0f,-1.0f,  // Vertex 6
    -1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,

    -1.0f, 1.0f,-1.0f,  // Vertex 7
     1.0f, 1.0f, 1.0f,
     1.0f, 1.0f,-1.0f,

     1.0f, 1.0f,-1.0f,  // Vertex 8
     1.0f, 1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,

     1.0f,-1.0f, 1.0f,  // Vertex 9
     1.0f,-1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f, 1.0f, 1.0f,  // Vertex 10
    -1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,

    -1.0f,-1.0f, 1.0f,  // Vertex 11
     1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
};

static const float g_uv_buffer_data[] = {
    1.0f, 0.0f,  // Vertex 0
    0.0f, 0.0f,
    0.0f, 1.0f,

    0.0f, 1.0f,  // Vertex 1
    1.0f, 1.0f,
    1.0f, 0.0f,

//    0.0f, 1.0f,  // Vertex 2
//    1.0f, 0.0f,
//    0.0f, 0.0f,

//    0.0f, 1.0f,  // Vertex 3
//    1.0f, 0.0f,
//    1.0f, 1.0f,

    0.0f, 0.0f,  // Vertex 2
    1.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 0.0f,  // Vertex 3
    0.0f, 1.0f,
    1.0f, 1.0f,

    0.0f, 1.0f,  // Vertex 4
    0.0f, 0.0f,
    1.0f, 0.0f,

    0.0f, 1.0f,  // Vertex 5
    1.0f, 0.0f,
    1.0f, 1.0f,

    0.0f, 1.0f,  // Vertex 6
    1.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 1.0f,  // Vertex 7
    1.0f, 0.0f,
    0.0f, 0.0f,

    0.0f, 1.0f,  // Vertex 8
    1.0f, 1.0f,
    1.0f, 0.0f,

    1.0f, 0.0f,  // Vertex 9
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // Vertex 10
    1.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 0.0f,  // Vertex 11
    0.0f, 0.0f,
    0.0f, 1.0f,
};

void dumpMatrix(const char *note, mat4x4 MVP)
{
    int i;

    printf("%s: \n", note);
    for (i=0; i<4; i++) {
        printf("%f, %f, %f, %f\n", MVP[i][0], MVP[i][1], MVP[i][2], MVP[i][3]);
    }
    printf("\n");
    fflush(stdout);
}

void dumpVec4(const char *note, vec4 vector)
{
    printf("%s: \n", note);
        printf("%f, %f, %f, %f\n", vector[0], vector[1], vector[2], vector[3]);
    printf("\n");
    fflush(stdout);
}

struct demo {
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    bool use_staging_buffer;

    VkInstance inst;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue queue;
    uint32_t graphics_queue_node_index;
    VkPhysicalDeviceProperties *gpu_props;
    VkPhysicalDeviceQueueProperties *queue_props;

    VkFramebuffer framebuffer;
    int width, height;
    VkFormat format;

    struct {
        VkImage image;
        VkDeviceMemory mem;
        VkCmdBuffer cmd;

        VkColorAttachmentView view;
        VkFence fence;
    } buffers[DEMO_BUFFER_COUNT];

    struct {
        VkFormat format;

        VkImage image;
        uint32_t num_mem;
        VkDeviceMemory *mem;
        VkDepthStencilView view;
    } depth;

    struct texture_object textures[DEMO_TEXTURE_COUNT];

    struct {
        VkBuffer buf;
        uint32_t num_mem;
        VkDeviceMemory *mem;
        VkBufferView view;
        VkBufferViewAttachInfo attach;
    } uniform_data;

    VkCmdBuffer cmd;  // Buffer for initialization commands
    VkDescriptorSetLayoutChain desc_layout_chain;
    VkDescriptorSetLayout desc_layout;
    VkPipeline pipeline;

    VkDynamicVpState viewport;
    VkDynamicRsState raster;
    VkDynamicCbState color_blend;
    VkDynamicDsState depth_stencil;

    mat4x4 projection_matrix;
    mat4x4 view_matrix;
    mat4x4 model_matrix;

    float spin_angle;
    float spin_increment;
    bool pause;

    VkDescriptorPool desc_pool;
    VkDescriptorSet desc_set;

    xcb_window_t window;
    xcb_intern_atom_reply_t *atom_wm_delete_window;

    bool quit;
    uint32_t current_buffer;
};

static void demo_flush_init_cmd(struct demo *demo)
{
    VkResult err;

    if (demo->cmd == VK_NULL_HANDLE)
        return;

    err = vkEndCommandBuffer(demo->cmd);
    assert(!err);

    const VkCmdBuffer cmd_bufs[] = { demo->cmd };

    err = vkQueueSubmit(demo->queue, 1, cmd_bufs, VK_NULL_HANDLE);
    assert(!err);

    err = vkQueueWaitIdle(demo->queue);
    assert(!err);

    vkDestroyObject(demo->cmd);
    demo->cmd = VK_NULL_HANDLE;
}

static void demo_add_mem_refs(
        struct demo *demo,
        int num_refs, VkDeviceMemory *mem)
{
    vkQueueAddMemReferences(demo->queue, num_refs, mem);
}

static void demo_remove_mem_refs(
        struct demo *demo,
        int num_refs, VkDeviceMemory *mem)
{
    vkQueueRemoveMemReferences(demo->queue, num_refs, mem);
}

static void demo_set_image_layout(
        struct demo *demo,
        VkImage image,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout)
{
    VkResult err;

    if (demo->cmd == VK_NULL_HANDLE) {
        const VkCmdBufferCreateInfo cmd = {
            .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO,
            .pNext = NULL,
            .queueNodeIndex = demo->graphics_queue_node_index,
            .flags = 0,
        };

        err = vkCreateCommandBuffer(demo->device, &cmd, &demo->cmd);
        assert(!err);

        VkCmdBufferBeginInfo cmd_buf_info = {
            .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO,
            .pNext = NULL,
            .flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT |
                VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT,
        };
        err = vkBeginCommandBuffer(demo->cmd, &cmd_buf_info);
    }

    VkImageMemoryBarrier image_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .outputMask = 0,
        .inputMask = 0,
        .oldLayout = old_image_layout,
        .newLayout = new_image_layout,
        .image = image,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR, 0, 1, 0, 0 }
    };

    if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL) {
        /* Make sure anything that was copying from this image has completed */
        image_memory_barrier.inputMask = VK_MEMORY_INPUT_TRANSFER_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        /* Make sure any Copy or CPU writes to image are flushed */
        image_memory_barrier.outputMask = VK_MEMORY_OUTPUT_CPU_WRITE_BIT | VK_MEMORY_OUTPUT_TRANSFER_BIT;
    }

    VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

    VkPipeEvent set_events[] = { VK_PIPE_EVENT_TOP_OF_PIPE };

    vkCmdPipelineBarrier(demo->cmd, VK_WAIT_EVENT_TOP_OF_PIPE, 1, set_events, 1, (const void **)&pmemory_barrier);
}

static void demo_draw_build_cmd(struct demo *demo, VkCmdBuffer cmd_buf)
{
    const VkColorAttachmentBindInfo color_attachment = {
        .view = demo->buffers[demo->current_buffer].view,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkDepthStencilBindInfo depth_stencil = {
        .view = demo->depth.view,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkClearColor clear_color = {
        .color.floatColor = { 0.2f, 0.2f, 0.2f, 0.2f },
        .useRawValue = false,
    };
    const float clear_depth = 1.0f;
    VkImageSubresourceRange clear_range;
    VkCmdBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT |
            VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT,
    };
    VkResult err;
    VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    const VkFramebufferCreateInfo fb_info = {
         .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
         .pNext = NULL,
         .colorAttachmentCount = 1,
         .pColorAttachments = (VkColorAttachmentBindInfo*) &color_attachment,
         .pDepthStencilAttachment = (VkDepthStencilBindInfo*) &depth_stencil,
         .sampleCount = 1,
         .width  = demo->width,
         .height = demo->height,
         .layers = 1,
    };
    VkRenderPassCreateInfo rp_info;
    VkRenderPassBegin rp_begin;

    memset(&rp_info, 0 , sizeof(rp_info));
    err = vkCreateFramebuffer(demo->device, &fb_info, &rp_begin.framebuffer);
    assert(!err);
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.renderArea.extent.width = demo->width;
    rp_info.renderArea.extent.height = demo->height;
    rp_info.colorAttachmentCount = fb_info.colorAttachmentCount;
    rp_info.pColorFormats = &demo->format;
    rp_info.pColorLayouts = &color_attachment.layout;
    rp_info.pColorLoadOps = &load_op;
    rp_info.pColorStoreOps = &store_op;
    rp_info.pColorLoadClearValues = &clear_color;
    rp_info.depthStencilFormat = VK_FORMAT_D16_UNORM;
    rp_info.depthStencilLayout = depth_stencil.layout;
    rp_info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp_info.depthLoadClearValue = clear_depth;
    rp_info.depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    rp_info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp_info.stencilLoadClearValue = 0;
    rp_info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    err = vkCreateRenderPass(demo->device, &rp_info, &rp_begin.renderPass);
    assert(!err);

    err = vkBeginCommandBuffer(cmd_buf, &cmd_buf_info);
    assert(!err);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  demo->pipeline);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
            0, 1, &demo->desc_set, NULL);

    vkCmdBindDynamicStateObject(cmd_buf, VK_STATE_BIND_POINT_VIEWPORT, demo->viewport);
    vkCmdBindDynamicStateObject(cmd_buf, VK_STATE_BIND_POINT_RASTER, demo->raster);
    vkCmdBindDynamicStateObject(cmd_buf, VK_STATE_BIND_POINT_COLOR_BLEND,
                                     demo->color_blend);
    vkCmdBindDynamicStateObject(cmd_buf, VK_STATE_BIND_POINT_DEPTH_STENCIL,
                                     demo->depth_stencil);

    vkCmdBeginRenderPass(cmd_buf, &rp_begin);
    clear_range.aspect = VK_IMAGE_ASPECT_COLOR;
    clear_range.baseMipLevel = 0;
    clear_range.mipLevels = 1;
    clear_range.baseArraySlice = 0;
    clear_range.arraySize = 1;
    vkCmdClearColorImage(cmd_buf,
            demo->buffers[demo->current_buffer].image,
            VK_IMAGE_LAYOUT_CLEAR_OPTIMAL,
            clear_color, 1, &clear_range);

    clear_range.aspect = VK_IMAGE_ASPECT_DEPTH;
    vkCmdClearDepthStencil(cmd_buf, demo->depth.image,
            VK_IMAGE_LAYOUT_CLEAR_OPTIMAL,
            clear_depth, 0, 1, &clear_range);

    vkCmdDraw(cmd_buf, 0, 12 * 3, 0, 1);
    vkCmdEndRenderPass(cmd_buf, rp_begin.renderPass);

    err = vkEndCommandBuffer(cmd_buf);
    assert(!err);

    vkDestroyObject(rp_begin.renderPass);
    vkDestroyObject(rp_begin.framebuffer);
}


void demo_update_data_buffer(struct demo *demo)
{
    mat4x4 MVP, Model, VP;
    int matrixSize = sizeof(MVP);
    uint8_t *pData;
    VkResult err;

    mat4x4_mul(VP, demo->projection_matrix, demo->view_matrix);

    // Rotate 22.5 degrees around the Y axis
    mat4x4_dup(Model, demo->model_matrix);
    mat4x4_rotate(demo->model_matrix, Model, 0.0f, 1.0f, 0.0f, (float)degreesToRadians(demo->spin_angle));
    mat4x4_mul(MVP, VP, demo->model_matrix);

    assert(demo->uniform_data.num_mem == 1);
    err = vkMapMemory(demo->uniform_data.mem[0], 0, (void **) &pData);
    assert(!err);

    memcpy(pData, (const void*) &MVP[0][0], matrixSize);

    err = vkUnmapMemory(demo->uniform_data.mem[0]);
    assert(!err);
}

static void demo_draw(struct demo *demo)
{
    const VK_WSI_X11_PRESENT_INFO present = {
        .destWindow = demo->window,
        .srcImage = demo->buffers[demo->current_buffer].image,
        .async = true,
        .flip = false,
    };
    VkFence fence = demo->buffers[demo->current_buffer].fence;
    VkResult err;

    err = vkWaitForFences(demo->device, 1, &fence, VK_TRUE, ~((uint64_t) 0));
    assert(err == VK_SUCCESS || err == VK_ERROR_UNAVAILABLE);

    err = vkQueueSubmit(demo->queue, 1, &demo->buffers[demo->current_buffer].cmd,
            VK_NULL_HANDLE);
    assert(!err);

    err = vkWsiX11QueuePresent(demo->queue, &present, fence);
    assert(!err);

    demo->current_buffer = (demo->current_buffer + 1) % DEMO_BUFFER_COUNT;
}

static void demo_prepare_buffers(struct demo *demo)
{
    const VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO presentable_image = {
        .format = demo->format,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .extent = {
            .width = demo->width,
            .height = demo->height,
        },
        .flags = 0,
    };
    const VkFenceCreateInfo fence = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };
    VkResult err;
    uint32_t i;

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        VkColorAttachmentViewCreateInfo color_attachment_view = {
            .sType = VK_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
            .pNext = NULL,
            .format = demo->format,
            .mipLevel = 0,
            .baseArraySlice = 0,
            .arraySize = 1,
        };

        err = vkWsiX11CreatePresentableImage(demo->device, &presentable_image,
                &demo->buffers[i].image, &demo->buffers[i].mem);
        assert(!err);

        demo_add_mem_refs(demo, 1, &demo->buffers[i].mem);

        demo_set_image_layout(demo, demo->buffers[i].image,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        color_attachment_view.image = demo->buffers[i].image;

        err = vkCreateColorAttachmentView(demo->device,
                &color_attachment_view, &demo->buffers[i].view);
        assert(!err);

        err = vkCreateFence(demo->device,
                &fence, &demo->buffers[i].fence);
        assert(!err);
    }
}

static void demo_prepare_depth(struct demo *demo)
{
    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    const VkImageCreateInfo image = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depth_format,
        .extent = { demo->width, demo->height, 1 },
        .mipLevels = 1,
        .arraySize = 1,
        .samples = 1,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_BIT,
        .flags = 0,
    };
    VkMemoryAllocInfo mem_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .memProps = VK_MEMORY_PROPERTY_DEVICE_ONLY,
        .memPriority = VK_MEMORY_PRIORITY_NORMAL,
    };
    VkDepthStencilViewCreateInfo view = {
        .sType = VK_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO,
        .pNext = NULL,
        .image = VK_NULL_HANDLE,
        .mipLevel = 0,
        .baseArraySlice = 0,
        .arraySize = 1,
        .flags = 0,
    };
    VkMemoryRequirements *mem_reqs;
    size_t mem_reqs_size = sizeof(VkMemoryRequirements);
    VkResult err;
    uint32_t num_allocations = 0;
    size_t num_alloc_size = sizeof(num_allocations);

    demo->depth.format = depth_format;

    /* create image */
    err = vkCreateImage(demo->device, &image,
            &demo->depth.image);
    assert(!err);

    err = vkGetObjectInfo(demo->depth.image, VK_OBJECT_INFO_TYPE_MEMORY_ALLOCATION_COUNT, &num_alloc_size, &num_allocations);
    assert(!err && num_alloc_size == sizeof(num_allocations));
    mem_reqs = malloc(num_allocations * sizeof(VkMemoryRequirements));
    demo->depth.mem = malloc(num_allocations * sizeof(VkDeviceMemory));
    demo->depth.num_mem = num_allocations;
    err = vkGetObjectInfo(demo->depth.image,
                    VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
                    &mem_reqs_size, mem_reqs);
    assert(!err && mem_reqs_size == num_allocations * sizeof(VkMemoryRequirements));
    for (uint32_t i = 0; i < num_allocations; i ++) {
        mem_alloc.allocationSize = mem_reqs[i].size;

        /* allocate memory */
        err = vkAllocMemory(demo->device, &mem_alloc,
                    &(demo->depth.mem[i]));
        assert(!err);

        /* bind memory */
        err = vkQueueBindObjectMemory(demo->queue, demo->depth.image, i,
                demo->depth.mem[i], 0);
        assert(!err);
    }

    demo_set_image_layout(demo, demo->depth.image,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    demo_add_mem_refs(demo, demo->depth.num_mem, demo->depth.mem);

    /* create image view */
    view.image = demo->depth.image;
    err = vkCreateDepthStencilView(demo->device, &view,
            &demo->depth.view);
    assert(!err);
}

/** loadTexture
 * 	loads a png file into an memory object, using cstdio , libpng.
 *
 *    	\param demo : Needed to access VK calls
 * 	\param filename : the png file to be loaded
 * 	\param width : width of png, to be updated as a side effect of this function
 * 	\param height : height of png, to be updated as a side effect of this function
 *
 * 	\return bool : an opengl texture id.  true if successful?,
 * 					should be validated by the client of this function.
 *
 * Source: http://en.wikibooks.org/wiki/OpenGL_Programming/Intermediate/Textures
 * Modified to copy image to memory
 *
 */
bool loadTexture(const char *filename, uint8_t *rgba_data,
                 VkSubresourceLayout *layout,
                 int32_t *width, int32_t *height)
{
  //header for testing if it is a png
  png_byte header[8];
  int is_png, bit_depth, color_type,rowbytes;
  png_uint_32 i, twidth, theight;
  png_structp  png_ptr;
  png_infop info_ptr, end_info;
  png_byte *image_data;
  png_bytep *row_pointers;

  //open file as binary
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    return false;
  }

  //read the header
  fread(header, 1, 8, fp);

  //test if png
  is_png = !png_sig_cmp(header, 0, 8);
  if (!is_png) {
    fclose(fp);
    return false;
  }

  //create png struct
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
      NULL, NULL);
  if (!png_ptr) {
    fclose(fp);
    return (false);
  }

  //create png info struct
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
    fclose(fp);
    return (false);
  }

  //create png info struct
  end_info = png_create_info_struct(png_ptr);
  if (!end_info) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
    fclose(fp);
    return (false);
  }

  //png error stuff, not sure libpng man suggests this.
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);
    return (false);
  }

  //init png reading
  png_init_io(png_ptr, fp);

  //let libpng know you already read the first 8 bytes
  png_set_sig_bytes(png_ptr, 8);

  // read all the info up to the image data
  png_read_info(png_ptr, info_ptr);

  // get info about png
  png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type,
      NULL, NULL, NULL);

  //update width and height based on png info
  *width = twidth;
  *height = theight;

  // Require that incoming texture be 8bits per color component
  // and 4 components (RGBA).
  if (png_get_bit_depth(png_ptr, info_ptr) != 8 ||
      png_get_channels(png_ptr, info_ptr) != 4) {
      return false;
  }

  if (rgba_data == NULL) {
      // If data pointer is null, we just want the width & height
      // clean up memory and close stuff
      png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
      fclose(fp);

      return true;
  }

  // Update the png info struct.
  png_read_update_info(png_ptr, info_ptr);

  // Row size in bytes.
  rowbytes = png_get_rowbytes(png_ptr, info_ptr);

  // Allocate the image_data as a big block, to be given to opengl
  image_data = (png_byte *)malloc(rowbytes * theight * sizeof(png_byte));
  if (!image_data) {
    //clean up memory and close stuff
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);
    return false;
  }

  // row_pointers is for pointing to image_data for reading the png with libpng
  row_pointers = (png_bytep *)malloc(theight * sizeof(png_bytep));
  if (!row_pointers) {
    //clean up memory and close stuff
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    // delete[] image_data;
    fclose(fp);
    return false;
  }
  // set the individual row_pointers to point at the correct offsets of image_data
  for (i = 0; i < theight; ++i)
    row_pointers[theight - 1 - i] = rgba_data + i * layout->rowPitch;

  // read the png into image_data through row_pointers
  png_read_image(png_ptr, row_pointers);

  // clean up memory and close stuff
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  free(row_pointers);
  free(image_data);
  fclose(fp);

  return true;
}

static void demo_prepare_texture_image(struct demo *demo,
                                       const char *filename,
                                       struct texture_object *tex_obj,
                                       VkImageTiling tiling,
                                       VkFlags mem_props)
{
    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    int32_t tex_width;
    int32_t tex_height;
    VkResult err;

    err = loadTexture(filename, NULL, NULL, &tex_width, &tex_height);
    assert(err);

    tex_obj->tex_width = tex_width;
    tex_obj->tex_height = tex_height;

    const VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = tex_format,
        .extent = { tex_width, tex_height, 1 },
        .mipLevels = 1,
        .arraySize = 1,
        .samples = 1,
        .tiling = tiling,
        .usage = VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT,
        .flags = 0,
    };
    VkMemoryAllocInfo mem_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .memProps = mem_props,
        .memPriority = VK_MEMORY_PRIORITY_NORMAL,
    };

    VkMemoryRequirements *mem_reqs;
    size_t mem_reqs_size = sizeof(VkMemoryRequirements);
    uint32_t num_allocations = 0;
    size_t num_alloc_size = sizeof(num_allocations);

    err = vkCreateImage(demo->device, &image_create_info,
            &tex_obj->image);
    assert(!err);

    err = vkGetObjectInfo(tex_obj->image,
                VK_OBJECT_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                &num_alloc_size, &num_allocations);
    assert(!err && num_alloc_size == sizeof(num_allocations));
    mem_reqs = malloc(num_allocations * sizeof(VkMemoryRequirements));
    tex_obj->mem = malloc(num_allocations * sizeof(VkDeviceMemory));
    err = vkGetObjectInfo(tex_obj->image,
                VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
                &mem_reqs_size, mem_reqs);
    assert(!err && mem_reqs_size == num_allocations * sizeof(VkMemoryRequirements));
    mem_alloc.memProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    for (uint32_t j = 0; j < num_allocations; j ++) {
        mem_alloc.allocationSize = mem_reqs[j].size;

        /* allocate memory */
        err = vkAllocMemory(demo->device, &mem_alloc,
                    &(tex_obj->mem[j]));
        assert(!err);

        /* bind memory */
        err = vkQueueBindObjectMemory(demo->queue, tex_obj->image, j, tex_obj->mem[j], 0);
        assert(!err);
    }
    free(mem_reqs);
    mem_reqs = NULL;

    tex_obj->num_mem = num_allocations;

    if (mem_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        const VkImageSubresource subres = {
            .aspect = VK_IMAGE_ASPECT_COLOR,
            .mipLevel = 0,
            .arraySlice = 0,
        };
        VkSubresourceLayout layout;
        size_t layout_size = sizeof(VkSubresourceLayout);
        void *data;

        err = vkGetImageSubresourceInfo(tex_obj->image, &subres,
                                         VK_SUBRESOURCE_INFO_TYPE_LAYOUT,
                                         &layout_size, &layout);
        assert(!err && layout_size == sizeof(layout));
        /* Linear texture must be within a single memory object */
        assert(num_allocations == 1);

        err = vkMapMemory(tex_obj->mem[0], 0, &data);
        assert(!err);

        if (!loadTexture(filename, data, &layout, &tex_width, &tex_height)) {
            fprintf(stderr, "Error loading texture: %s\n", filename);
        }

        err = vkUnmapMemory(tex_obj->mem[0]);
        assert(!err);
    }

    tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    demo_set_image_layout(demo, tex_obj->image,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           tex_obj->imageLayout);
    /* setting the image layout does not reference the actual memory so no need to add a mem ref */
}

static void demo_destroy_texture_image(struct demo *demo, struct texture_object *tex_objs)
{
    /* clean up staging resources */
    for (uint32_t j = 0; j < tex_objs->num_mem; j ++) {
        vkQueueBindObjectMemory(demo->queue, tex_objs->image, j, VK_NULL_HANDLE, 0);
        vkFreeMemory(tex_objs->mem[j]);
    }

    free(tex_objs->mem);
    vkDestroyObject(tex_objs->image);
}

static void demo_prepare_textures(struct demo *demo)
{
    const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties props;
    size_t size = sizeof(props);
    VkResult err;
    uint32_t i;

    err = vkGetFormatInfo(demo->device, tex_format,
                           VK_FORMAT_INFO_TYPE_PROPERTIES,
                           &size, &props);
    assert(!err);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {

        if (props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT && !demo->use_staging_buffer) {
            /* Device can texture using linear textures */
            demo_prepare_texture_image(demo, tex_files[i], &demo->textures[i],
                                       VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        } else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
            /* Must use staging buffer to copy linear texture to optimized */
            struct texture_object staging_texture;

            memset(&staging_texture, 0, sizeof(staging_texture));
            demo_prepare_texture_image(demo, tex_files[i], &staging_texture,
                                       VK_IMAGE_TILING_LINEAR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

            demo_prepare_texture_image(demo, tex_files[i], &demo->textures[i],
                                       VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_ONLY);

            demo_set_image_layout(demo, staging_texture.image,
                                   staging_texture.imageLayout,
                                   VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL);

            demo_set_image_layout(demo, demo->textures[i].image,
                                   demo->textures[i].imageLayout,
                                   VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL);

            VkImageCopy copy_region = {
                .srcSubresource = { VK_IMAGE_ASPECT_COLOR, 0, 0 },
                .srcOffset = { 0, 0, 0 },
                .destSubresource = { VK_IMAGE_ASPECT_COLOR, 0, 0 },
                .destOffset = { 0, 0, 0 },
                .extent = { staging_texture.tex_width, staging_texture.tex_height, 1 },
            };
            vkCmdCopyImage(demo->cmd,
                            staging_texture.image, VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL,
                            demo->textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL,
                            1, &copy_region);

            demo_add_mem_refs(demo, staging_texture.num_mem, staging_texture.mem);
            demo_add_mem_refs(demo, demo->textures[i].num_mem, demo->textures[i].mem);

            demo_set_image_layout(demo, demo->textures[i].image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL,
                                   demo->textures[i].imageLayout);

            demo_flush_init_cmd(demo);

            demo_destroy_texture_image(demo, &staging_texture);
            demo_remove_mem_refs(demo, staging_texture.num_mem, staging_texture.mem);
        } else {
            /* Can't support VK_FORMAT_B8G8R8A8_UNORM !? */
            assert(!"No support for tB8G8R8A8_UNORM as texture image format");
        }

        const VkSamplerCreateInfo sampler = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .magFilter = VK_TEX_FILTER_NEAREST,
            .minFilter = VK_TEX_FILTER_NEAREST,
            .mipMode = VK_TEX_MIPMAP_MODE_BASE,
            .addressU = VK_TEX_ADDRESS_CLAMP,
            .addressV = VK_TEX_ADDRESS_CLAMP,
            .addressW = VK_TEX_ADDRESS_CLAMP,
            .mipLodBias = 0.0f,
            .maxAnisotropy = 1,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_OPAQUE_WHITE,
        };

        VkImageViewCreateInfo view = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .image = VK_NULL_HANDLE,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = tex_format,
            .channels = { VK_CHANNEL_SWIZZLE_R,
                          VK_CHANNEL_SWIZZLE_G,
                          VK_CHANNEL_SWIZZLE_B,
                          VK_CHANNEL_SWIZZLE_A, },
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR, 0, 1, 0, 1 },
            .minLod = 0.0f,
        };

        /* create sampler */
        err = vkCreateSampler(demo->device, &sampler,
                &demo->textures[i].sampler);
        assert(!err);

        /* create image view */
        view.image = demo->textures[i].image;
        err = vkCreateImageView(demo->device, &view,
                &demo->textures[i].view);
        assert(!err);
    }
}

void demo_prepare_cube_data_buffer(struct demo *demo)
{
    VkBufferCreateInfo buf_info;
    VkBufferViewCreateInfo view_info;
    VkMemoryAllocInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .memProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memPriority = VK_MEMORY_PRIORITY_NORMAL,
    };
    VkMemoryRequirements *mem_reqs;
    size_t mem_reqs_size = sizeof(VkMemoryRequirements);
    uint32_t num_allocations = 0;
    size_t num_alloc_size = sizeof(num_allocations);
    uint8_t *pData;
    int i;
    mat4x4 MVP, VP;
    VkResult err;
    struct vktexcube_vs_uniform data;

    mat4x4_mul(VP, demo->projection_matrix, demo->view_matrix);
    mat4x4_mul(MVP, VP, demo->model_matrix);
    memcpy(data.mvp, MVP, sizeof(MVP));
//    dumpMatrix("MVP", MVP);

    for (i=0; i<12*3; i++) {
        data.position[i][0] = g_vertex_buffer_data[i*3];
        data.position[i][1] = g_vertex_buffer_data[i*3+1];
        data.position[i][2] = g_vertex_buffer_data[i*3+2];
        data.position[i][3] = 1.0f;
        data.attr[i][0] = g_uv_buffer_data[2*i];
        data.attr[i][1] = g_uv_buffer_data[2*i + 1];
        data.attr[i][2] = 0;
        data.attr[i][3] = 0;
    }

    memset(&buf_info, 0, sizeof(buf_info));
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.size = sizeof(data);
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    err = vkCreateBuffer(demo->device, &buf_info, &demo->uniform_data.buf);
    assert(!err);

    err = vkGetObjectInfo(demo->uniform_data.buf,
                           VK_OBJECT_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                           &num_alloc_size, &num_allocations);
    assert(!err && num_alloc_size == sizeof(num_allocations));
    mem_reqs = malloc(num_allocations * sizeof(VkMemoryRequirements));
    demo->uniform_data.mem = malloc(num_allocations * sizeof(VkDeviceMemory));
    demo->uniform_data.num_mem = num_allocations;
    err = vkGetObjectInfo(demo->uniform_data.buf,
            VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
            &mem_reqs_size, mem_reqs);
    assert(!err && mem_reqs_size == num_allocations * sizeof(*mem_reqs));
    for (uint32_t i = 0; i < num_allocations; i ++) {
        alloc_info.allocationSize = mem_reqs[i].size;

        err = vkAllocMemory(demo->device, &alloc_info, &(demo->uniform_data.mem[i]));
        assert(!err);

        err = vkMapMemory(demo->uniform_data.mem[i], 0, (void **) &pData);
        assert(!err);

        memcpy(pData, &data, (size_t)alloc_info.allocationSize);

        err = vkUnmapMemory(demo->uniform_data.mem[i]);
        assert(!err);

        err = vkQueueBindObjectMemory(demo->queue, demo->uniform_data.buf, i,
                    demo->uniform_data.mem[i], 0);
        assert(!err);
    }
    demo_add_mem_refs(demo, demo->uniform_data.num_mem, demo->uniform_data.mem);

    memset(&view_info, 0, sizeof(view_info));
    view_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    view_info.buffer = demo->uniform_data.buf;
    view_info.viewType = VK_BUFFER_VIEW_TYPE_RAW;
    view_info.offset = 0;
    view_info.range = sizeof(data);

    err = vkCreateBufferView(demo->device, &view_info, &demo->uniform_data.view);
    assert(!err);

    demo->uniform_data.attach.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO;
    demo->uniform_data.attach.view = demo->uniform_data.view;
}

static void demo_prepare_descriptor_layout(struct demo *demo)
{
    const VkDescriptorSetLayoutBinding layout_bindings[2] = {
        [0] = {
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .count = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = NULL,
        },
        [1] = {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .count = DEMO_TEXTURE_COUNT,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = NULL,
        },
    };
    const VkDescriptorSetLayoutCreateInfo descriptor_layout = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 2,
        .pBinding = layout_bindings,
    };
    VkResult err;

    err = vkCreateDescriptorSetLayout(demo->device,
            &descriptor_layout, &demo->desc_layout);
    assert(!err);

    err = vkCreateDescriptorSetLayoutChain(demo->device,
            1, &demo->desc_layout, &demo->desc_layout_chain);
    assert(!err);
}

static VkShader demo_prepare_shader(struct demo *demo,
                                      VkShaderStage stage,
                                      const void *code,
                                      size_t size)
{
    VkShaderCreateInfo createInfo;
    VkShader shader;
    VkResult err;


    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;

#ifdef EXTERNAL_SPV
    createInfo.codeSize = size;
    createInfo.pCode = code;
    createInfo.flags = 0;

    err = vkCreateShader(demo->device, &createInfo, &shader);
    if (err) {
        free((void *) createInfo.pCode);
    }
#else
    // Create fake SPV structure to feed GLSL
    // to the driver "under the covers"
    createInfo.codeSize = 3 * sizeof(uint32_t) + size + 1;
    createInfo.pCode = malloc(createInfo.codeSize);
    createInfo.flags = 0;

    /* try version 0 first: VkShaderStage followed by GLSL */
    ((uint32_t *) createInfo.pCode)[0] = ICD_SPV_MAGIC;
    ((uint32_t *) createInfo.pCode)[1] = 0;
    ((uint32_t *) createInfo.pCode)[2] = stage;
    memcpy(((uint32_t *) createInfo.pCode + 3), code, size + 1);

    err = vkCreateShader(demo->device, &createInfo, &shader);
    if (err) {
        free((void *) createInfo.pCode);
        return NULL;
    }
#endif

    return shader;
}

char *demo_read_spv(const char *filename, size_t *psize)
{
    long int size;
    void *shader_code;

    FILE *fp = fopen(filename, "rb");
    if (!fp) return NULL;

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);

    shader_code = malloc(size);
    fread(shader_code, size, 1, fp);

    *psize = size;

    return shader_code;
}

static VkShader demo_prepare_vs(struct demo *demo)
{
#ifdef EXTERNAL_SPV
    void *vertShaderCode;
    size_t size;

    vertShaderCode = demo_read_spv("cube-vert.spv", &size);

    return demo_prepare_shader(demo, VK_SHADER_STAGE_VERTEX,
                               vertShaderCode, size);
#else
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "\n"
            "layout(binding = 0) uniform buf {\n"
            "        mat4 MVP;\n"
            "        vec4 position[12*3];\n"
            "        vec4 attr[12*3];\n"
            "} ubuf;\n"
            "\n"
            "layout (location = 0) out vec4 texcoord;\n"
            "\n"
            "void main() \n"
            "{\n"
            "   texcoord = ubuf.attr[gl_VertexID];\n"
            "   gl_Position = ubuf.MVP * ubuf.position[gl_VertexID];\n"
            "}\n";

    return demo_prepare_shader(demo, VK_SHADER_STAGE_VERTEX,
                               (const void *) vertShaderText,
                               strlen(vertShaderText));
#endif
}

static VkShader demo_prepare_fs(struct demo *demo)
{
#ifdef EXTERNAL_SPV
    void *fragShaderCode;
    size_t size;

    fragShaderCode = demo_read_spv("cube-frag.spv", &size);

    return demo_prepare_shader(demo, VK_SHADER_STAGE_FRAGMENT,
                               fragShaderCode, size);
#else
    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (binding = 1) uniform sampler2D tex;\n"
            "\n"
            "layout (location = 0) in vec4 texcoord;\n"
            "void main() {\n"
            "   gl_FragColor = texture(tex, texcoord.xy);\n"
            "}\n";

    return demo_prepare_shader(demo, VK_SHADER_STAGE_FRAGMENT,
                               (const void *) fragShaderText,
                               strlen(fragShaderText));
#endif
}

static void demo_prepare_pipeline(struct demo *demo)
{
    VkGraphicsPipelineCreateInfo pipeline;
    VkPipelineIaStateCreateInfo ia;
    VkPipelineRsStateCreateInfo rs;
    VkPipelineCbStateCreateInfo cb;
    VkPipelineDsStateCreateInfo ds;
    VkPipelineShaderStageCreateInfo vs;
    VkPipelineShaderStageCreateInfo fs;
    VkPipelineVpStateCreateInfo vp;
    VkPipelineMsStateCreateInfo ms;
    VkResult err;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.pSetLayoutChain = demo->desc_layout_chain;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO;
    rs.fillMode = VK_FILL_MODE_SOLID;
    rs.cullMode = VK_CULL_MODE_BACK;
    rs.frontFace = VK_FRONT_FACE_CCW;

    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO;
    VkPipelineCbAttachmentState att_state[1];
    memset(att_state, 0, sizeof(att_state));
    att_state[0].format = demo->format;
    att_state[0].channelWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;

    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.clipOrigin = VK_COORDINATE_ORIGIN_LOWER_LEFT;

    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO;
    ds.format = demo->depth.format;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_EQUAL;
    ds.depthBoundsEnable = VK_FALSE;
    ds.back.stencilFailOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilPassOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilCompareOp = VK_COMPARE_OP_ALWAYS;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = ds.back;

    memset(&vs, 0, sizeof(vs));
    vs.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs.shader.stage = VK_SHADER_STAGE_VERTEX;
    vs.shader.shader = demo_prepare_vs(demo);
    assert(vs.shader.shader != NULL);

    memset(&fs, 0, sizeof(fs));
    fs.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fs.shader.stage = VK_SHADER_STAGE_FRAGMENT;
    fs.shader.shader = demo_prepare_fs(demo);
    assert(fs.shader.shader != NULL);

    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO;
    ms.sampleMask = 1;
    ms.multisampleEnable = VK_FALSE;
    ms.samples = 1;

    pipeline.pNext = (const void *) &ia;
    ia.pNext = (const void *) &rs;
    rs.pNext = (const void *) &cb;
    cb.pNext = (const void *) &ms;
    ms.pNext = (const void *) &vp;
    vp.pNext = (const void *) &ds;
    ds.pNext = (const void *) &vs;
    vs.pNext = (const void *) &fs;

    err = vkCreateGraphicsPipeline(demo->device, &pipeline, &demo->pipeline);
    assert(!err);

    vkDestroyObject(vs.shader.shader);
    vkDestroyObject(fs.shader.shader);
}

static void demo_prepare_dynamic_states(struct demo *demo)
{
    VkDynamicVpStateCreateInfo viewport_create;
    VkDynamicRsStateCreateInfo raster;
    VkDynamicCbStateCreateInfo color_blend;
    VkDynamicDsStateCreateInfo depth_stencil;
    VkResult err;

    memset(&viewport_create, 0, sizeof(viewport_create));
    viewport_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO;
    viewport_create.viewportAndScissorCount = 1;
    VkViewport viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.height = (float) demo->height;
    viewport.width = (float) demo->width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    viewport_create.pViewports = &viewport;
    VkRect scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = demo->width;
    scissor.extent.height = demo->height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    viewport_create.pScissors = &scissor;

    memset(&raster, 0, sizeof(raster));
    raster.sType = VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO;
    raster.pointSize = 1.0;
    raster.lineWidth = 1.0;

    memset(&color_blend, 0, sizeof(color_blend));
    color_blend.sType = VK_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO;
    color_blend.blendConst[0] = 1.0f;
    color_blend.blendConst[1] = 1.0f;
    color_blend.blendConst[2] = 1.0f;
    color_blend.blendConst[3] = 1.0f;

    memset(&depth_stencil, 0, sizeof(depth_stencil));
    depth_stencil.sType = VK_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO;
    depth_stencil.minDepth = 0.0f;
    depth_stencil.maxDepth = 1.0f;
    depth_stencil.stencilBackRef = 0;
    depth_stencil.stencilFrontRef = 0;
    depth_stencil.stencilReadMask = 0xff;
    depth_stencil.stencilWriteMask = 0xff;

    err = vkCreateDynamicViewportState(demo->device, &viewport_create, &demo->viewport);
    assert(!err);

    err = vkCreateDynamicRasterState(demo->device, &raster, &demo->raster);
    assert(!err);

    err = vkCreateDynamicColorBlendState(demo->device,
            &color_blend, &demo->color_blend);
    assert(!err);

    err = vkCreateDynamicDepthStencilState(demo->device,
            &depth_stencil, &demo->depth_stencil);
    assert(!err);
}

static void demo_prepare_descriptor_pool(struct demo *demo)
{
    const VkDescriptorTypeCount type_counts[2] = {
        [0] = {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .count = 1,
        },
        [1] = {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .count = DEMO_TEXTURE_COUNT,
        },
    };
    const VkDescriptorPoolCreateInfo descriptor_pool = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .count = 2,
        .pTypeCount = type_counts,
    };
    VkResult err;

    err = vkCreateDescriptorPool(demo->device,
            VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1,
            &descriptor_pool, &demo->desc_pool);
    assert(!err);
}

static void demo_prepare_descriptor_set(struct demo *demo)
{
    VkImageViewAttachInfo view_info[DEMO_TEXTURE_COUNT];
    VkSamplerImageViewInfo combined_info[DEMO_TEXTURE_COUNT];
    VkUpdateSamplerTextures update_fs;
    VkUpdateBuffers update_vs;
    const void *update_array[2] = { &update_vs, &update_fs };
    VkResult err;
    uint32_t count;
    uint32_t i;

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        view_info[i].sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO;
        view_info[i].pNext = NULL;
        view_info[i].view = demo->textures[i].view,
        view_info[i].layout = VK_IMAGE_LAYOUT_GENERAL;

        combined_info[i].sampler = demo->textures[i].sampler;
        combined_info[i].pImageView = &view_info[i];
    }

    memset(&update_vs, 0, sizeof(update_vs));
    update_vs.sType = VK_STRUCTURE_TYPE_UPDATE_BUFFERS;
    update_vs.pNext = &update_fs;
    update_vs.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    update_vs.count = 1;
    update_vs.pBufferViews = &demo->uniform_data.attach;

    memset(&update_fs, 0, sizeof(update_fs));
    update_fs.sType = VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES;
    update_fs.binding = 1;
    update_fs.count = DEMO_TEXTURE_COUNT;
    update_fs.pSamplerImageViews = combined_info;

    err = vkAllocDescriptorSets(demo->desc_pool,
            VK_DESCRIPTOR_SET_USAGE_STATIC,
            1, &demo->desc_layout,
            &demo->desc_set, &count);
    assert(!err && count == 1);

    vkBeginDescriptorPoolUpdate(demo->device,
            VK_DESCRIPTOR_UPDATE_MODE_FASTEST);

    vkClearDescriptorSets(demo->desc_pool, 1, &demo->desc_set);
    vkUpdateDescriptors(demo->desc_set, 2, update_array);

    vkEndDescriptorPoolUpdate(demo->device, demo->buffers[demo->current_buffer].cmd);
}

static void demo_prepare(struct demo *demo)
{
    const VkCmdBufferCreateInfo cmd = {
        .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .queueNodeIndex = demo->graphics_queue_node_index,
        .flags = 0,
    };
    VkResult err;

    demo_prepare_buffers(demo);
    demo_prepare_depth(demo);
    demo_prepare_textures(demo);
    demo_prepare_cube_data_buffer(demo);

    demo_prepare_descriptor_layout(demo);
    demo_prepare_pipeline(demo);
    demo_prepare_dynamic_states(demo);

    for (int i = 0; i < DEMO_BUFFER_COUNT; i++) {
        err = vkCreateCommandBuffer(demo->device, &cmd, &demo->buffers[i].cmd);
        assert(!err);
    }

    demo_prepare_descriptor_pool(demo);
    demo_prepare_descriptor_set(demo);


    for (int i = 0; i < DEMO_BUFFER_COUNT; i++) {
        demo->current_buffer = i;
        demo_draw_build_cmd(demo, demo->buffers[i].cmd);
    }

    /*
     * Prepare functions above may generate pipeline commands
     * that need to be flushed before beginning the render loop.
     */
    demo_flush_init_cmd(demo);

    demo->current_buffer = 0;
}

static void demo_handle_event(struct demo *demo,
                              const xcb_generic_event_t *event)
{
    uint8_t event_code = event->response_type & 0x7f;
    switch (event_code) {
    case XCB_EXPOSE:
        // TODO: Resize window
        break;
    case XCB_CLIENT_MESSAGE:
        if((*(xcb_client_message_event_t*)event).data.data32[0] ==
           (*demo->atom_wm_delete_window).atom) {
            demo->quit = true;
        }
        break;
    case XCB_KEY_RELEASE:
        {
            const xcb_key_release_event_t *key =
                (const xcb_key_release_event_t *) event;

            switch (key->detail) {
            case 0x9:           // Escape
                demo->quit = true;
                break;
            case 0x71:          // left arrow key
                demo->spin_angle += demo->spin_increment;
                break;
            case 0x72:          // right arrow key
                demo->spin_angle -= demo->spin_increment;
                break;
            case 0x41:
                demo->pause = !demo->pause;
                break;
            }
        }
        break;
    default:
        break;
    }
}

static void demo_run(struct demo *demo)
{
    xcb_flush(demo->connection);

    while (!demo->quit) {
        xcb_generic_event_t *event;

        if (demo->pause) {
            event = xcb_wait_for_event(demo->connection);
        } else {
            event = xcb_poll_for_event(demo->connection);
        }
        if (event) {
            demo_handle_event(demo, event);
            free(event);
        }

        // Wait for work to finish before updating MVP.
        vkDeviceWaitIdle(demo->device);
        demo_update_data_buffer(demo);

        demo_draw(demo);

        // Wait for work to finish before updating MVP.
        vkDeviceWaitIdle(demo->device);
    }
}

static void demo_create_window(struct demo *demo)
{
    uint32_t value_mask, value_list[32];

    demo->window = xcb_generate_id(demo->connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = demo->screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE |
                    XCB_EVENT_MASK_EXPOSURE;

    xcb_create_window(demo->connection,
            XCB_COPY_FROM_PARENT,
            demo->window, demo->screen->root,
            0, 0, demo->width, demo->height, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            demo->screen->root_visual,
            value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(demo->connection, 1, 12,
                                                      "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(demo->connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(demo->connection, 0, 16, "WM_DELETE_WINDOW");
    demo->atom_wm_delete_window = xcb_intern_atom_reply(demo->connection, cookie2, 0);

    xcb_change_property(demo->connection, XCB_PROP_MODE_REPLACE,
                        demo->window, (*reply).atom, 4, 32, 1,
                        &(*demo->atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(demo->connection, demo->window);
}

static void demo_init_vk(struct demo *demo)
{
    // TODO : Should query count w/ GetGlobalExtensionInfo, then enable via CreateInstance
    const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pAppName = "cube",
        .appVersion = 0,
        .pEngineName = "cube",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION,
    };
    const VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pAppInfo = &app,
        .pAllocCb = NULL,
        .extensionCount = 0,
        .ppEnabledExtensionNames = NULL,
    };
    const VK_WSI_X11_CONNECTION_INFO connection = {
        .pConnection = demo->connection,
        .root = demo->screen->root,
        .provider = 0,
    };
    const VkDeviceQueueCreateInfo queue = {
        .queueNodeIndex = 0,
        .queueCount = 1,
    };
    
    const char *ext_names[] = {
        "VK_WSI_X11",
    };

    const VkDeviceCreateInfo device = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueRecordCount = 1,
        .pRequestedQueues = &queue,
        .extensionCount = 1, // TODO : Should query count w/ GetGlobalExtensionInfo
        .ppEnabledExtensionNames = ext_names,
        .flags = VK_DEVICE_CREATE_VALIDATION_BIT,
    };
    VkResult err;
    uint32_t gpu_count;
    uint32_t i;
    size_t data_size;
    uint32_t queue_count;

    err = vkCreateInstance(&inst_info, &demo->inst);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        printf("Cannot find a compatible Vulkan installable client driver "
               "(ICD).\nExiting ...\n");
        fflush(stdout);
        exit(1);
    } else {
        assert(!err);
    }

    gpu_count = 1;
    err = vkEnumeratePhysicalDevices(demo->inst, &gpu_count, &demo->gpu);
    assert(!err && gpu_count == 1);

    err = vkWsiX11AssociateConnection(demo->gpu, &connection);
    assert(!err);

    err = vkCreateDevice(demo->gpu, &device, &demo->device);
    assert(!err);

    err = vkGetPhysicalDeviceInfo(demo->gpu, VK_PHYSICAL_DEVICE_INFO_TYPE_PROPERTIES,
                        &data_size, NULL);
    assert(!err);

    demo->gpu_props = (VkPhysicalDeviceProperties *) malloc(data_size);
    err = vkGetPhysicalDeviceInfo(demo->gpu, VK_PHYSICAL_DEVICE_INFO_TYPE_PROPERTIES,
                        &data_size, demo->gpu_props);
    assert(!err);

    err = vkGetPhysicalDeviceInfo(demo->gpu, VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES,
                        &data_size, NULL);
    assert(!err);

    demo->queue_props = (VkPhysicalDeviceQueueProperties *) malloc(data_size);
    err = vkGetPhysicalDeviceInfo(demo->gpu, VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES,
                        &data_size, demo->queue_props);
    assert(!err);
	queue_count = (uint32_t)(data_size / sizeof(VkPhysicalDeviceQueueProperties));
    assert(queue_count >= 1);

    // Graphics queue and MemMgr queue can be separate.
    // TODO: Add support for separate queues, including synchronization,
    //       and appropriate tracking for QueueSubmit and QueueBindObjectMemory
    for (i = 0; i < queue_count; i++) {
        if ((demo->queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            (demo->queue_props[i].queueFlags & VK_QUEUE_MEMMGR_BIT)      )
            break;
    }
    assert(i < queue_count);
    demo->graphics_queue_node_index = i;

    err = vkGetDeviceQueue(demo->device, demo->graphics_queue_node_index,
            0, &demo->queue);
    assert(!err);
}

static void demo_init_connection(struct demo *demo)
{
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    demo->connection = xcb_connect(NULL, &scr);
    if (demo->connection == NULL) {
        printf("Cannot find a compatible Vulkan installable client driver "
               "(ICD).\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

    setup = xcb_get_setup(demo->connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);

    demo->screen = iter.data;
}

static void demo_init(struct demo *demo, int argc, char **argv)
{
    vec3 eye = {0.0f, 3.0f, 5.0f};
    vec3 origin = {0, 0, 0};
    vec3 up = {0.0f, -1.0f, 0.0};

    memset(demo, 0, sizeof(*demo));

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--use_staging", strlen("--use_staging")) == 0)
            demo->use_staging_buffer = true;
    }

    demo_init_connection(demo);
    demo_init_vk(demo);

    demo->width = 500;
    demo->height = 500;
    demo->format = VK_FORMAT_B8G8R8A8_UNORM;

    demo->spin_angle = 0.01f;
    demo->spin_increment = 0.01f;
    demo->pause = false;

    mat4x4_perspective(demo->projection_matrix, (float)degreesToRadians(45.0f), 1.0f, 0.1f, 100.0f);
    mat4x4_look_at(demo->view_matrix, eye, origin, up);
    mat4x4_identity(demo->model_matrix);
}

static void demo_cleanup(struct demo *demo)
{
    uint32_t i, j;

    vkDestroyObject(demo->desc_set);
    vkDestroyObject(demo->desc_pool);

    vkDestroyObject(demo->viewport);
    vkDestroyObject(demo->raster);
    vkDestroyObject(demo->color_blend);
    vkDestroyObject(demo->depth_stencil);

    vkDestroyObject(demo->pipeline);
    vkDestroyObject(demo->desc_layout_chain);
    vkDestroyObject(demo->desc_layout);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        vkDestroyObject(demo->textures[i].view);
        vkQueueBindObjectMemory(demo->queue, demo->textures[i].image, 0, VK_NULL_HANDLE, 0);
        vkDestroyObject(demo->textures[i].image);
        demo_remove_mem_refs(demo, demo->textures[i].num_mem, demo->textures[i].mem);
        for (j = 0; j < demo->textures[i].num_mem; j++)
            vkFreeMemory(demo->textures[i].mem[j]);
        vkDestroyObject(demo->textures[i].sampler);
    }

    vkDestroyObject(demo->depth.view);
    vkQueueBindObjectMemory(demo->queue, demo->depth.image, 0, VK_NULL_HANDLE, 0);
    vkDestroyObject(demo->depth.image);
    demo_remove_mem_refs(demo, demo->depth.num_mem, demo->depth.mem);
    for (j = 0; j < demo->depth.num_mem; j++) {
        vkFreeMemory(demo->depth.mem[j]);
    }

    vkDestroyObject(demo->uniform_data.view);
    vkQueueBindObjectMemory(demo->queue, demo->uniform_data.buf, 0, VK_NULL_HANDLE, 0);
    vkDestroyObject(demo->uniform_data.buf);
    demo_remove_mem_refs(demo, demo->uniform_data.num_mem, demo->uniform_data.mem);
    for (j = 0; j < demo->uniform_data.num_mem; j++)
        vkFreeMemory(demo->uniform_data.mem[j]);

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        vkDestroyObject(demo->buffers[i].fence);
        vkDestroyObject(demo->buffers[i].view);
        vkDestroyObject(demo->buffers[i].image);
        vkDestroyObject(demo->buffers[i].cmd);
        demo_remove_mem_refs(demo, 1, &demo->buffers[i].mem);
    }

    vkDestroyDevice(demo->device);
    vkDestroyInstance(demo->inst);

    xcb_destroy_window(demo->connection, demo->window);
    xcb_disconnect(demo->connection);
}

int main(int argc, char **argv)
{
    struct demo demo;

    demo_init(&demo, argc, argv);

    demo_prepare(&demo);
    demo_create_window(&demo);
    demo_run(&demo);

    demo_cleanup(&demo);

    return 0;
}
