#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <xcb/xcb.h>
#include <xgl.h>
#include <xglDbg.h>
#include <xglWsiX11Ext.h>

#include "icd-bil.h"

#include "linmath.h"
#include <unistd.h>
#include <png.h>

#define DEMO_BUFFER_COUNT 2
#define DEMO_TEXTURE_COUNT 1

/*
 * When not defined, code will use built-in GLSL compiler
 * which may not be supported on all drivers
 */
#define EXTERNAL_BIL

static char *tex_files[] = {
    "lunarg-logo-256x256-solid.png"
};

struct xglcube_vs_uniform {
    // Must start with MVP
    XGL_FLOAT   mvp[4][4];
    XGL_FLOAT   position[12*3][4];
    XGL_FLOAT   color[12*3][4];
};

struct xgltexcube_vs_uniform {
    // Must start with MVP
    XGL_FLOAT   mvp[4][4];
    XGL_FLOAT   position[12*3][4];
    XGL_FLOAT   attr[12*3][4];
};

//--------------------------------------------------------------------------------------
// Mesh and VertexFormat Data
//--------------------------------------------------------------------------------------
struct Vertex
{
    XGL_FLOAT posX, posY, posZ, posW;    // Position data
    XGL_FLOAT r, g, b, a;                // Color
};

struct VertexPosTex
{
    XGL_FLOAT posX, posY, posZ, posW;    // Position data
    XGL_FLOAT u, v, s, t;                // Texcoord
};

#define XYZ1(_x_, _y_, _z_)         (_x_), (_y_), (_z_), 1.f
#define UV(_u_, _v_)                (_u_), (_v_), 0.f, 1.f

static const XGL_FLOAT g_vertex_buffer_data[] = {
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
     1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // Vertex 4
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // Vertex 5
    -1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,

    -1.0f, 1.0f,-1.0f,  // Vertex 6
    -1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,

    -1.0f, 1.0f,-1.0f,  // Vertex 7
     1.0f, 1.0f,-1.0f,
     1.0f, 1.0f, 1.0f,

     1.0f, 1.0f,-1.0f,  // Vertex 8
     1.0f, 1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,

     1.0f,-1.0f, 1.0f,  // Vertex 9
     1.0f,-1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f, 1.0f, 1.0f,  // Vertex 10
     1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,

    -1.0f,-1.0f, 1.0f,  // Vertex 11
     1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
};

static const XGL_FLOAT g_uv_buffer_data[] = {
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
    1.0f, 1.0f,
    0.0f, 1.0f,

    0.0f, 1.0f,  // Vertex 4
    1.0f, 0.0f,
    0.0f, 0.0f,

    0.0f, 1.0f,  // Vertex 5
    1.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 1.0f,  // Vertex 6
    1.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 1.0f,  // Vertex 7
    0.0f, 0.0f,
    1.0f, 0.0f,

    0.0f, 1.0f,  // Vertex 8
    1.0f, 1.0f,
    1.0f, 0.0f,

    1.0f, 0.0f,  // Vertex 9
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // Vertex 10
    0.0f, 1.0f,
    1.0f, 0.0f,

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

    XGL_PHYSICAL_GPU gpu;
    XGL_DEVICE device;
    XGL_QUEUE queue;

    int width, height;
    XGL_FORMAT format;

    struct {
        XGL_IMAGE image;
        XGL_GPU_MEMORY mem;

        XGL_COLOR_ATTACHMENT_VIEW view;
        XGL_FENCE fence;
    } buffers[DEMO_BUFFER_COUNT];

    struct {
        XGL_FORMAT format;

        XGL_IMAGE image;
        XGL_UINT num_mem;
        XGL_GPU_MEMORY *mem;
        XGL_DEPTH_STENCIL_VIEW view;
    } depth;

    struct {
        XGL_SAMPLER sampler;

        char *filename;
        XGL_IMAGE image;
        XGL_UINT num_mem;
        XGL_GPU_MEMORY *mem;
        XGL_IMAGE_VIEW view;
    } textures[DEMO_TEXTURE_COUNT];

    struct {
        XGL_BUFFER buf;
        XGL_UINT num_mem;
        XGL_GPU_MEMORY *mem;
        XGL_BUFFER_VIEW view;
        XGL_BUFFER_VIEW_ATTACH_INFO attach;
    } uniform_data;

    XGL_DESCRIPTOR_SET_LAYOUT desc_layout_vs;
    XGL_DESCRIPTOR_SET_LAYOUT desc_layout_fs;
    XGL_DESCRIPTOR_SET_LAYOUT *desc_layout_last;
    XGL_PIPELINE pipeline;

    XGL_DYNAMIC_VP_STATE_OBJECT viewport;
    XGL_DYNAMIC_RS_STATE_OBJECT raster;
    XGL_DYNAMIC_CB_STATE_OBJECT color_blend;
    XGL_DYNAMIC_DS_STATE_OBJECT depth_stencil;

    mat4x4 projection_matrix;
    mat4x4 view_matrix;
    mat4x4 model_matrix;

    XGL_FLOAT spin_angle;
    XGL_FLOAT spin_increment;
    bool pause;

    XGL_CMD_BUFFER cmd;

    XGL_DESCRIPTOR_REGION desc_region;
    XGL_DESCRIPTOR_SET desc_set;

    xcb_window_t window;
    xcb_intern_atom_reply_t *atom_wm_delete_window;

    bool quit;
    XGL_UINT current_buffer;
};

static void demo_draw_build_cmd(struct demo *demo)
{
    const XGL_COLOR_ATTACHMENT_BIND_INFO color_attachment = {
        .view = demo->buffers[demo->current_buffer].view,
        .layout = XGL_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const XGL_DEPTH_STENCIL_BIND_INFO depth_stencil = {
        .view = demo->depth.view,
        .layout = XGL_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const XGL_FLOAT clear_color[4] = { 0.2f, 0.2f, 0.2f, 0.2f };
    const XGL_FLOAT clear_depth = 1.0f;
    XGL_IMAGE_SUBRESOURCE_RANGE clear_range;
    XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO graphics_cmd_buf_info = {
        .sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO,
        .pNext = NULL,
        .operation = XGL_RENDER_PASS_OPERATION_BEGIN_AND_END,
    };
    XGL_CMD_BUFFER_BEGIN_INFO cmd_buf_info = {
        .sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO,
        .pNext = &graphics_cmd_buf_info,
        .flags = XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT |
            XGL_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT,
    };
    XGL_RESULT err;
    XGL_ATTACHMENT_LOAD_OP load_op = XGL_ATTACHMENT_LOAD_OP_DONT_CARE;
    XGL_ATTACHMENT_STORE_OP store_op = XGL_ATTACHMENT_STORE_OP_DONT_CARE;
    const XGL_FRAMEBUFFER_CREATE_INFO fb_info = {
         .sType = XGL_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
         .pNext = NULL,
         .colorAttachmentCount = 1,
         .pColorAttachments = (XGL_COLOR_ATTACHMENT_BIND_INFO*) &color_attachment,
         .pDepthStencilAttachment = (XGL_DEPTH_STENCIL_BIND_INFO*) &depth_stencil,
         .sampleCount = 1,
    };
    XGL_RENDER_PASS_CREATE_INFO rp_info;

    memset(&rp_info, 0 , sizeof(rp_info));
    err = xglCreateFramebuffer(demo->device, &fb_info, &(rp_info.framebuffer));
    assert(!err);
    rp_info.sType = XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.renderArea.extent.width = demo->width;
    rp_info.renderArea.extent.height = demo->height;
    rp_info.pColorLoadOps = &load_op;
    rp_info.pColorStoreOps = &store_op;
    rp_info.depthLoadOp = XGL_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp_info.depthStoreOp = XGL_ATTACHMENT_STORE_OP_DONT_CARE;
    rp_info.stencilLoadOp = XGL_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp_info.stencilStoreOp = XGL_ATTACHMENT_STORE_OP_DONT_CARE;
    err = xglCreateRenderPass(demo->device, &rp_info, &(graphics_cmd_buf_info.renderPass));
    assert(!err);

    err = xglBeginCommandBuffer(demo->cmd, &cmd_buf_info);
    assert(!err);

    xglCmdBindPipeline(demo->cmd, XGL_PIPELINE_BIND_POINT_GRAPHICS,
                                  demo->pipeline);
    xglCmdBindDescriptorSet(demo->cmd, XGL_PIPELINE_BIND_POINT_GRAPHICS,
            demo->desc_set, NULL);

    xglCmdBindDynamicStateObject(demo->cmd, XGL_STATE_BIND_VIEWPORT, demo->viewport);
    xglCmdBindDynamicStateObject(demo->cmd, XGL_STATE_BIND_RASTER, demo->raster);
    xglCmdBindDynamicStateObject(demo->cmd, XGL_STATE_BIND_COLOR_BLEND,
                                     demo->color_blend);
    xglCmdBindDynamicStateObject(demo->cmd, XGL_STATE_BIND_DEPTH_STENCIL,
                                     demo->depth_stencil);

    clear_range.aspect = XGL_IMAGE_ASPECT_COLOR;
    clear_range.baseMipLevel = 0;
    clear_range.mipLevels = 1;
    clear_range.baseArraySlice = 0;
    clear_range.arraySize = 1;
    xglCmdClearColorImage(demo->cmd,
            demo->buffers[demo->current_buffer].image,
            clear_color, 1, &clear_range);

    clear_range.aspect = XGL_IMAGE_ASPECT_DEPTH;
    xglCmdClearDepthStencil(demo->cmd, demo->depth.image,
            clear_depth, 0, 1, &clear_range);

    xglCmdDraw(demo->cmd, 0, 12 * 3, 0, 1);

    err = xglEndCommandBuffer(demo->cmd);
    assert(!err);
}


void demo_update_data_buffer(struct demo *demo)
{
    mat4x4 MVP, Model, VP;
    int matrixSize = sizeof(MVP);
    XGL_UINT8 *pData;
    XGL_RESULT err;

    mat4x4_mul(VP, demo->projection_matrix, demo->view_matrix);

    // Rotate 22.5 degrees around the Y axis
    mat4x4_dup(Model, demo->model_matrix);
    mat4x4_rotate(demo->model_matrix, Model, 0.0f, 1.0f, 0.0f, degreesToRadians(demo->spin_angle));
    mat4x4_mul(MVP, VP, demo->model_matrix);

    assert(demo->uniform_data.num_mem == 1);
    err = xglMapMemory(demo->uniform_data.mem[0], 0, (XGL_VOID **) &pData);
    assert(!err);

    memcpy(pData, (const void*) &MVP[0][0], matrixSize);

    err = xglUnmapMemory(demo->uniform_data.mem[0]);
    assert(!err);
}

static void demo_draw(struct demo *demo)
{
    const XGL_WSI_X11_PRESENT_INFO present = {
        .destWindow = demo->window,
        .srcImage = demo->buffers[demo->current_buffer].image,
        .async = true,
        .flip = false,
    };
    XGL_FENCE fence = demo->buffers[demo->current_buffer].fence;
    XGL_RESULT err;

    demo_draw_build_cmd(demo);

    err = xglWaitForFences(demo->device, 1, &fence, XGL_TRUE, ~((XGL_UINT64) 0));
    assert(err == XGL_SUCCESS || err == XGL_ERROR_UNAVAILABLE);

    err = xglQueueSubmit(demo->queue, 1, &demo->cmd,
            0, NULL, XGL_NULL_HANDLE);
    assert(!err);

    err = xglWsiX11QueuePresent(demo->queue, &present, fence);
    assert(!err);

    demo->current_buffer = (demo->current_buffer + 1) % DEMO_BUFFER_COUNT;
}

static void demo_prepare_buffers(struct demo *demo)
{
    const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO presentable_image = {
        .format = demo->format,
        .usage = XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .extent = {
            .width = demo->width,
            .height = demo->height,
        },
        .flags = 0,
    };
    const XGL_FENCE_CREATE_INFO fence = {
        .sType = XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };
    XGL_RESULT err;
    XGL_UINT i;

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO color_attachment_view = {
            .sType = XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
            .pNext = NULL,
            .format = demo->format,
            .mipLevel = 0,
            .baseArraySlice = 0,
            .arraySize = 1,
        };

        err = xglWsiX11CreatePresentableImage(demo->device, &presentable_image,
                &demo->buffers[i].image, &demo->buffers[i].mem);
        assert(!err);

        color_attachment_view.image = demo->buffers[i].image;

        err = xglCreateColorAttachmentView(demo->device,
                &color_attachment_view, &demo->buffers[i].view);
        assert(!err);

        err = xglCreateFence(demo->device,
                &fence, &demo->buffers[i].fence);
        assert(!err);
    }
}

static void demo_prepare_depth(struct demo *demo)
{
    const XGL_FORMAT depth_format = XGL_FMT_D16_UNORM;
    const XGL_IMAGE_CREATE_INFO image = {
        .sType = XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = XGL_IMAGE_2D,
        .format = depth_format,
        .extent = { demo->width, demo->height, 1 },
        .mipLevels = 1,
        .arraySize = 1,
        .samples = 1,
        .tiling = XGL_OPTIMAL_TILING,
        .usage = XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT,
        .flags = 0,
    };
    XGL_MEMORY_ALLOC_IMAGE_INFO img_alloc = {
        .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO,
        .pNext = NULL,
    };
    XGL_MEMORY_ALLOC_INFO mem_alloc = {
        .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = &img_alloc,
        .allocationSize = 0,
        .memProps = XGL_MEMORY_PROPERTY_GPU_ONLY,
        .memType = XGL_MEMORY_TYPE_IMAGE,
        .memPriority = XGL_MEMORY_PRIORITY_NORMAL,
    };
    XGL_DEPTH_STENCIL_VIEW_CREATE_INFO view = {
        .sType = XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO,
        .pNext = NULL,
        .image = XGL_NULL_HANDLE,
        .mipLevel = 0,
        .baseArraySlice = 0,
        .arraySize = 1,
        .flags = 0,
    };
    XGL_MEMORY_REQUIREMENTS *mem_reqs;
    XGL_SIZE mem_reqs_size = sizeof(XGL_MEMORY_REQUIREMENTS);
    XGL_IMAGE_MEMORY_REQUIREMENTS img_reqs;
    XGL_SIZE img_reqs_size = sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS);
    XGL_RESULT err;
    XGL_UINT num_allocations = 0;
    XGL_SIZE num_alloc_size = sizeof(num_allocations);

    demo->depth.format = depth_format;

    /* create image */
    err = xglCreateImage(demo->device, &image,
            &demo->depth.image);
    assert(!err);


    err = xglGetObjectInfo(demo->depth.image, XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT, &num_alloc_size, &num_allocations);
    assert(!err && num_alloc_size == sizeof(num_allocations));
    mem_reqs = malloc(num_allocations * sizeof(XGL_MEMORY_REQUIREMENTS));
    demo->depth.mem = malloc(num_allocations * sizeof(XGL_GPU_MEMORY));
    demo->depth.num_mem = num_allocations;
    err = xglGetObjectInfo(demo->depth.image,
                    XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                    &mem_reqs_size, mem_reqs);
    assert(!err && mem_reqs_size == num_allocations * sizeof(XGL_MEMORY_REQUIREMENTS));
    err = xglGetObjectInfo(demo->depth.image,
                    XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS,
                    &img_reqs_size, &img_reqs);
    assert(!err && img_reqs_size == sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS));
    img_alloc.usage = img_reqs.usage;
    img_alloc.formatClass = img_reqs.formatClass;
    img_alloc.samples = img_reqs.samples;
    for (XGL_UINT i = 0; i < num_allocations; i ++) {
        mem_alloc.allocationSize = mem_reqs[i].size;

        /* allocate memory */
        err = xglAllocMemory(demo->device, &mem_alloc,
                    &(demo->depth.mem[i]));
        assert(!err);

        /* bind memory */
        err = xglBindObjectMemory(demo->depth.image, i,
                demo->depth.mem[i], 0);
        assert(!err);
    }

    /* create image view */
    view.image = demo->depth.image;
    err = xglCreateDepthStencilView(demo->device, &view,
            &demo->depth.view);
    assert(!err);
}

/** loadTexture
 * 	loads a png file into an memory object, using cstdio , libpng.
 *
 *    	\param demo : Needed to access XGL calls
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
bool loadTexture(char *filename, XGL_UINT8 *rgba_data,
                 XGL_SUBRESOURCE_LAYOUT *layout,
                 XGL_INT *width, XGL_INT *height)
{
  //header for testing if it is a png
  png_byte header[8];
  int i, is_png, bit_depth, color_type,rowbytes;
  png_uint_32 twidth, theight;
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
    row_pointers[theight - 1 - i] = rgba_data + i * rowbytes;

  // read the png into image_data through row_pointers
  png_read_image(png_ptr, row_pointers);

  // clean up memory and close stuff
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  free(row_pointers);
  free(image_data);
  fclose(fp);

  return true;
}

static void demo_prepare_textures(struct demo *demo)
{
    const XGL_FORMAT tex_format = XGL_FMT_R8G8B8A8_UNORM;
    XGL_INT tex_width;
    XGL_INT tex_height;
    XGL_RESULT err;
    XGL_UINT i;

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        const XGL_SAMPLER_CREATE_INFO sampler = {
            .sType = XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .magFilter = XGL_TEX_FILTER_NEAREST,
            .minFilter = XGL_TEX_FILTER_NEAREST,
            .mipMode = XGL_TEX_MIPMAP_BASE,
            .addressU = XGL_TEX_ADDRESS_CLAMP,
            .addressV = XGL_TEX_ADDRESS_CLAMP,
            .addressW = XGL_TEX_ADDRESS_CLAMP,
            .mipLodBias = 0.0f,
            .maxAnisotropy = 0,
            .compareFunc = XGL_COMPARE_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColorType = XGL_BORDER_COLOR_OPAQUE_WHITE,
        };

        assert(loadTexture(tex_files[i], NULL, NULL, &tex_width, &tex_height));

        const XGL_IMAGE_CREATE_INFO image = {
            .sType = XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = NULL,
            .imageType = XGL_IMAGE_2D,
            .format = tex_format,
            .extent = { tex_width, tex_height, 1 },
            .mipLevels = 1,
            .arraySize = 1,
            .samples = 1,
            .tiling = XGL_LINEAR_TILING,
            .usage = XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT,
            .flags = 0,
        };
        XGL_MEMORY_ALLOC_IMAGE_INFO img_alloc = {
            .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO,
            .pNext = NULL,
        };
        XGL_MEMORY_ALLOC_INFO mem_alloc = {
            .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
            .pNext = &img_alloc,
            .allocationSize = 0,
            .memProps = XGL_MEMORY_PROPERTY_GPU_ONLY,
            .memType = XGL_MEMORY_TYPE_IMAGE,
            .memPriority = XGL_MEMORY_PRIORITY_NORMAL,
        };
        XGL_IMAGE_VIEW_CREATE_INFO view = {
            .sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .image = XGL_NULL_HANDLE,
            .viewType = XGL_IMAGE_VIEW_2D,
            .format = image.format,
            .channels = { XGL_CHANNEL_SWIZZLE_R,
                          XGL_CHANNEL_SWIZZLE_G,
                          XGL_CHANNEL_SWIZZLE_B,
                          XGL_CHANNEL_SWIZZLE_A, },
            .subresourceRange = { XGL_IMAGE_ASPECT_COLOR, 0, 1, 0, 1 },
            .minLod = 0.0f,
        };

        XGL_MEMORY_REQUIREMENTS *mem_reqs;
        XGL_SIZE mem_reqs_size = sizeof(XGL_MEMORY_REQUIREMENTS);
        XGL_IMAGE_MEMORY_REQUIREMENTS img_reqs;
        XGL_SIZE img_reqs_size = sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS);
        XGL_UINT num_allocations = 0;
        XGL_SIZE num_alloc_size = sizeof(num_allocations);

        /* create sampler */
        err = xglCreateSampler(demo->device, &sampler,
                &demo->textures[i].sampler);
        assert(!err);

        /* create image */
        err = xglCreateImage(demo->device, &image,
                &demo->textures[i].image);
        assert(!err);

        err = xglGetObjectInfo(demo->textures[i].image,
                    XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                    &num_alloc_size, &num_allocations);
        assert(!err && num_alloc_size == sizeof(num_allocations));
        mem_reqs = malloc(num_allocations * sizeof(XGL_MEMORY_REQUIREMENTS));
        demo->textures[i].mem = malloc(num_allocations * sizeof(XGL_GPU_MEMORY));
        demo->textures[i].num_mem = num_allocations;
        err = xglGetObjectInfo(demo->textures[i].image,
                    XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                    &mem_reqs_size, mem_reqs);
        assert(!err && mem_reqs_size == num_allocations * sizeof(XGL_MEMORY_REQUIREMENTS));
        err = xglGetObjectInfo(demo->textures[i].image,
                        XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS,
                        &img_reqs_size, &img_reqs);
        assert(!err && img_reqs_size == sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS));
        img_alloc.usage = img_reqs.usage;
        img_alloc.formatClass = img_reqs.formatClass;
        img_alloc.samples = img_reqs.samples;
        for (XGL_UINT j = 0; j < num_allocations; j ++) {
            mem_alloc.allocationSize = mem_reqs[j].size;

            /* allocate memory */
            err = xglAllocMemory(demo->device, &mem_alloc,
                        &(demo->textures[i].mem[j]));
            assert(!err);

            /* bind memory */
            err = xglBindObjectMemory(demo->textures[i].image, j,
                    demo->textures[i].mem[j], 0);
            assert(!err);
        }

        /* create image view */
        view.image = demo->textures[i].image;
        err = xglCreateImageView(demo->device, &view,
                &demo->textures[i].view);
        assert(!err);
    }

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        const XGL_IMAGE_SUBRESOURCE subres = {
            .aspect = XGL_IMAGE_ASPECT_COLOR,
            .mipLevel = 0,
            .arraySlice = 0,
        };
        XGL_SUBRESOURCE_LAYOUT layout;
        XGL_SIZE layout_size = sizeof(layout);
        XGL_VOID *data;

        err = xglGetImageSubresourceInfo(demo->textures[i].image, &subres,
                XGL_INFO_TYPE_SUBRESOURCE_LAYOUT, &layout_size, &layout);
        assert(!err && layout_size == sizeof(layout));
        assert(demo->textures[i].num_mem == 1);

        err = xglMapMemory(demo->textures[i].mem[0], 0, &data);
        assert(!err);

        loadTexture(tex_files[i], data, &layout, &tex_width, &tex_height);

        err = xglUnmapMemory(demo->textures[i].mem[0]);
        assert(!err);
    }
}

void demo_prepare_cube_data_buffer(struct demo *demo)
{
    XGL_BUFFER_CREATE_INFO buf_info;
    XGL_BUFFER_VIEW_CREATE_INFO view_info;
    XGL_MEMORY_ALLOC_BUFFER_INFO buf_alloc = {
        .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO,
        .pNext = NULL,
    };
    XGL_MEMORY_ALLOC_INFO alloc_info = {
        .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = &buf_alloc,
        .allocationSize = 0,
        .memProps = XGL_MEMORY_PROPERTY_CPU_VISIBLE_BIT,
        .memType = XGL_MEMORY_TYPE_BUFFER,
        .memPriority = XGL_MEMORY_PRIORITY_NORMAL,
    };
    XGL_MEMORY_REQUIREMENTS *mem_reqs;
    XGL_SIZE mem_reqs_size = sizeof(XGL_MEMORY_REQUIREMENTS);
    XGL_BUFFER_MEMORY_REQUIREMENTS buf_reqs;
    XGL_SIZE buf_reqs_size = sizeof(XGL_BUFFER_MEMORY_REQUIREMENTS);
    XGL_UINT num_allocations = 0;
    XGL_SIZE num_alloc_size = sizeof(num_allocations);
    XGL_UINT8 *pData;
    int i;
    mat4x4 MVP, VP;
    XGL_RESULT err;
    struct xgltexcube_vs_uniform data;

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
    buf_info.sType = XGL_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.size = sizeof(data);
    buf_info.usage = XGL_BUFFER_USAGE_UNIFORM_READ_BIT;
    err = xglCreateBuffer(demo->device, &buf_info, &demo->uniform_data.buf);
    assert(!err);

    err = xglGetObjectInfo(demo->uniform_data.buf,
                           XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                           &num_alloc_size, &num_allocations);
    assert(!err && num_alloc_size == sizeof(num_allocations));
    mem_reqs = malloc(num_allocations * sizeof(XGL_MEMORY_REQUIREMENTS));
    demo->uniform_data.mem = malloc(num_allocations * sizeof(XGL_GPU_MEMORY));
    demo->uniform_data.num_mem = num_allocations;
    err = xglGetObjectInfo(demo->uniform_data.buf,
            XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
            &mem_reqs_size, mem_reqs);
    assert(!err && mem_reqs_size == num_allocations * sizeof(*mem_reqs));
    err = xglGetObjectInfo(demo->uniform_data.buf,
                    XGL_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS,
                    &buf_reqs_size, &buf_reqs);
    assert(!err && buf_reqs_size == sizeof(XGL_BUFFER_MEMORY_REQUIREMENTS));
    buf_alloc.usage = buf_reqs.usage;
    for (XGL_UINT i = 0; i < num_allocations; i ++) {
        alloc_info.allocationSize = mem_reqs[i].size;

        err = xglAllocMemory(demo->device, &alloc_info, &(demo->uniform_data.mem[i]));
        assert(!err);

        err = xglMapMemory(demo->uniform_data.mem[i], 0, (XGL_VOID **) &pData);
        assert(!err);

        memcpy(pData, &data, alloc_info.allocationSize);

        err = xglUnmapMemory(demo->uniform_data.mem[i]);
        assert(!err);

        err = xglBindObjectMemory(demo->uniform_data.buf, i,
                    demo->uniform_data.mem[i], 0);
        assert(!err);
    }

    memset(&view_info, 0, sizeof(view_info));
    view_info.sType = XGL_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    view_info.buffer = demo->uniform_data.buf;
    view_info.viewType = XGL_BUFFER_VIEW_RAW;
    view_info.offset = 0;
    view_info.range = sizeof(data);

    err = xglCreateBufferView(demo->device, &view_info, &demo->uniform_data.view);
    assert(!err);

    demo->uniform_data.attach.sType = XGL_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO;
    demo->uniform_data.attach.view = demo->uniform_data.view;
}

static void demo_prepare_descriptor_layout(struct demo *demo)
{
    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO descriptor_layout_vs = {
        .sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .descriptorType = XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count = 1,
        .stageFlags = XGL_SHADER_STAGE_FLAGS_VERTEX_BIT,
        .immutableSampler = XGL_NULL_HANDLE,
    };
    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO descriptor_layout_fs = {
        .sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .descriptorType = XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE,
        .count = DEMO_TEXTURE_COUNT,
        .stageFlags = XGL_SHADER_STAGE_FLAGS_FRAGMENT_BIT,
        .immutableSampler = XGL_NULL_HANDLE,
    };
    const uint32_t bind_point = 0;
    XGL_RESULT err;

    err = xglCreateDescriptorSetLayout(demo->device,
            XGL_SHADER_STAGE_FLAGS_VERTEX_BIT, &bind_point,
            XGL_NULL_HANDLE, &descriptor_layout_vs,
            &demo->desc_layout_vs);
    assert(!err);

    err = xglCreateDescriptorSetLayout(demo->device,
            XGL_SHADER_STAGE_FLAGS_FRAGMENT_BIT, &bind_point,
            demo->desc_layout_vs, &descriptor_layout_fs,
            &demo->desc_layout_fs);
    assert(!err);

    demo->desc_layout_last = &demo->desc_layout_fs;
}

static XGL_SHADER demo_prepare_shader(struct demo *demo,
                                      XGL_PIPELINE_SHADER_STAGE stage,
                                      const void *code,
                                      XGL_SIZE size)
{
    XGL_SHADER_CREATE_INFO createInfo;
    XGL_SHADER shader;
    XGL_RESULT err;


    createInfo.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;

#ifdef EXTERNAL_BIL
    createInfo.codeSize = size;
    createInfo.pCode = code;
    createInfo.flags = 0;

    err = xglCreateShader(demo->device, &createInfo, &shader);
    if (err) {
        free((void *) createInfo.pCode);
    }
#else
    // Create fake BIL structure to feed GLSL
    // to the driver "under the covers"
    createInfo.codeSize = 3 * sizeof(uint32_t) + size + 1;
    createInfo.pCode = malloc(createInfo.codeSize);
    createInfo.flags = 0;

    /* try version 0 first: XGL_PIPELINE_SHADER_STAGE followed by GLSL */
    ((uint32_t *) createInfo.pCode)[0] = ICD_BIL_MAGIC;
    ((uint32_t *) createInfo.pCode)[1] = 0;
    ((uint32_t *) createInfo.pCode)[2] = stage;
    memcpy(((uint32_t *) createInfo.pCode + 3), code, size + 1);

    err = xglCreateShader(demo->device, &createInfo, &shader);
    if (err) {
        free((void *) createInfo.pCode);
        return NULL;
    }
#endif

    return shader;
}

char *demo_read_bil(const char *filename, XGL_SIZE *psize)
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

static XGL_SHADER demo_prepare_vs(struct demo *demo)
{
#ifdef EXTERNAL_BIL
    void *vertShaderCode;
    XGL_SIZE size;

    vertShaderCode = demo_read_bil("cube-vert.bil", &size);

    return demo_prepare_shader(demo, XGL_SHADER_STAGE_VERTEX,
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

    return demo_prepare_shader(demo, XGL_SHADER_STAGE_VERTEX,
                               (const void *) vertShaderText,
                               strlen(vertShaderText));
#endif
}

static XGL_SHADER demo_prepare_fs(struct demo *demo)
{
#ifdef EXTERNAL_BIL
    void *fragShaderCode;
    XGL_SIZE size;

    fragShaderCode = demo_read_bil("cube-frag.bil", &size);

    return demo_prepare_shader(demo, XGL_SHADER_STAGE_FRAGMENT,
                               fragShaderCode, size);
#else
    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (binding = 0) uniform sampler2D tex;\n"
            "\n"
            "layout (location = 0) in vec4 texcoord;\n"
            "void main() {\n"
            "   gl_FragColor = texture(tex, texcoord.xy);\n"
            "}\n";

    return demo_prepare_shader(demo, XGL_SHADER_STAGE_FRAGMENT,
                               (const void *) fragShaderText,
                               strlen(fragShaderText));
#endif
}

static void demo_prepare_pipeline(struct demo *demo)
{
    XGL_GRAPHICS_PIPELINE_CREATE_INFO pipeline;
    XGL_PIPELINE_IA_STATE_CREATE_INFO ia;
    XGL_PIPELINE_RS_STATE_CREATE_INFO rs;
    XGL_PIPELINE_CB_STATE_CREATE_INFO cb;
    XGL_PIPELINE_DS_STATE_CREATE_INFO ds;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO fs;
    XGL_PIPELINE_VP_STATE_CREATE_INFO vp;
    XGL_PIPELINE_MS_STATE_CREATE_INFO ms;
    XGL_RESULT err;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.lastSetLayout = *demo->desc_layout_last;

    memset(&ia, 0, sizeof(ia));
    ia.sType = XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO;
    ia.topology = XGL_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO;
    rs.fillMode = XGL_FILL_SOLID;
    rs.cullMode = XGL_CULL_NONE;
    rs.frontFace = XGL_FRONT_FACE_CCW;

    memset(&cb, 0, sizeof(cb));
    cb.sType = XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO;
    XGL_PIPELINE_CB_ATTACHMENT_STATE att_state[1];
    memset(att_state, 0, sizeof(att_state));
    att_state[0].format = demo->format;
    att_state[0].channelWriteMask = 0xf;
    att_state[0].blendEnable = XGL_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;

    memset(&vp, 0, sizeof(vp));
    vp.sType = XGL_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO;
    vp.scissorEnable = XGL_FALSE;

    memset(&ds, 0, sizeof(ds));
    ds.sType = XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO;
    ds.format = demo->depth.format;
    ds.depthTestEnable = XGL_TRUE;
    ds.depthWriteEnable = XGL_TRUE;
    ds.depthFunc = XGL_COMPARE_LESS_EQUAL;
    ds.depthBoundsEnable = XGL_FALSE;
    ds.back.stencilFailOp = XGL_STENCIL_OP_KEEP;
    ds.back.stencilPassOp = XGL_STENCIL_OP_KEEP;
    ds.back.stencilFunc = XGL_COMPARE_ALWAYS;
    ds.stencilTestEnable = XGL_FALSE;
    ds.front = ds.back;

    memset(&vs, 0, sizeof(vs));
    vs.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs.shader.stage = XGL_SHADER_STAGE_VERTEX;
    vs.shader.shader = demo_prepare_vs(demo);
    assert(vs.shader.shader != NULL);

    memset(&fs, 0, sizeof(fs));
    fs.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fs.shader.stage = XGL_SHADER_STAGE_FRAGMENT;
    fs.shader.shader = demo_prepare_fs(demo);
    assert(fs.shader.shader != NULL);

    memset(&ms, 0, sizeof(ms));
    ms.sType = XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO;
    ms.sampleMask = 1;
    ms.multisampleEnable = XGL_FALSE;
    ms.samples = 1;

    pipeline.pNext = (const XGL_VOID *) &ia;
    ia.pNext = (const XGL_VOID *) &rs;
    rs.pNext = (const XGL_VOID *) &cb;
    cb.pNext = (const XGL_VOID *) &ms;
    ms.pNext = (const XGL_VOID *) &vp;
    vp.pNext = (const XGL_VOID *) &ds;
    ds.pNext = (const XGL_VOID *) &vs;
    vs.pNext = (const XGL_VOID *) &fs;

    err = xglCreateGraphicsPipeline(demo->device, &pipeline, &demo->pipeline);
    assert(!err);

    xglDestroyObject(vs.shader.shader);
    xglDestroyObject(fs.shader.shader);
}

static void demo_prepare_dynamic_states(struct demo *demo)
{
    XGL_DYNAMIC_VP_STATE_CREATE_INFO viewport_create;
    XGL_DYNAMIC_RS_STATE_CREATE_INFO raster;
    XGL_DYNAMIC_CB_STATE_CREATE_INFO color_blend;
    XGL_DYNAMIC_DS_STATE_CREATE_INFO depth_stencil;
    XGL_RESULT err;

    memset(&viewport_create, 0, sizeof(viewport_create));
    viewport_create.sType = XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO;
    viewport_create.viewportCount = 1;
    XGL_VIEWPORT viewport;
    viewport.height = (XGL_FLOAT) demo->height;
    viewport.width = (XGL_FLOAT) demo->width;
    viewport.minDepth = (XGL_FLOAT) 0.0f;
    viewport.maxDepth = (XGL_FLOAT) 1.0f;
    viewport_create.pViewports = &viewport;

    memset(&raster, 0, sizeof(raster));
    raster.sType = XGL_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO;

    memset(&color_blend, 0, sizeof(color_blend));
    color_blend.sType = XGL_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO;

    memset(&depth_stencil, 0, sizeof(depth_stencil));
    depth_stencil.sType = XGL_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO;
    depth_stencil.stencilBackRef = 0;
    depth_stencil.stencilFrontRef = 0;
    depth_stencil.stencilReadMask = 0xff;
    depth_stencil.stencilWriteMask = 0xff;

    err = xglCreateDynamicViewportState(demo->device, &viewport_create, &demo->viewport);
    assert(!err);

    err = xglCreateDynamicRasterState(demo->device, &raster, &demo->raster);
    assert(!err);

    err = xglCreateDynamicColorBlendState(demo->device,
            &color_blend, &demo->color_blend);
    assert(!err);

    err = xglCreateDynamicDepthStencilState(demo->device,
            &depth_stencil, &demo->depth_stencil);
    assert(!err);
}

static void demo_prepare_descriptor_region(struct demo *demo)
{
    const XGL_DESCRIPTOR_TYPE_COUNT type_counts[2] = {
        [0] = {
            .type = XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .count = 1,
        },
        [1] = {
            .type = XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE,
            .count = DEMO_TEXTURE_COUNT,
        },
    };
    const XGL_DESCRIPTOR_REGION_CREATE_INFO descriptor_region = {
        .sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_REGION_CREATE_INFO,
        .pNext = NULL,
        .count = 2,
        .pTypeCount = type_counts,
    };
    XGL_RESULT err;

    err = xglCreateDescriptorRegion(demo->device,
            XGL_DESCRIPTOR_REGION_USAGE_ONE_SHOT, 1,
            &descriptor_region, &demo->desc_region);
    assert(!err);
}

static void demo_prepare_descriptor_set(struct demo *demo)
{
    const XGL_BUFFER_VIEW_ATTACH_INFO *view_info_vs =
        &demo->uniform_data.attach;
    XGL_IMAGE_VIEW_ATTACH_INFO view_info[DEMO_TEXTURE_COUNT];
    XGL_SAMPLER_IMAGE_VIEW_INFO combined_info[DEMO_TEXTURE_COUNT];
    XGL_UPDATE_SAMPLER_TEXTURES update_fs;
    XGL_UPDATE_BUFFERS update_vs;
    XGL_RESULT err;
    uint32_t count;
    XGL_UINT i;

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        view_info[i].sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO;
        view_info[i].pNext = NULL;
        view_info[i].view = demo->textures[i].view,
        view_info[i].layout = XGL_IMAGE_LAYOUT_GENERAL;

        combined_info[i].pSampler = demo->textures[i].sampler;
        combined_info[i].pImageView = &view_info[i];
    }

    memset(&update_vs, 0, sizeof(update_vs));
    update_vs.sType = XGL_STRUCTURE_TYPE_UPDATE_BUFFERS;
    update_vs.pNext = &update_fs;
    update_vs.descriptorType = XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    update_vs.count = 1;
    update_vs.pBufferViews = &view_info_vs;

    memset(&update_fs, 0, sizeof(update_fs));
    update_fs.sType = XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES;
    update_fs.index = 1;
    update_fs.count = DEMO_TEXTURE_COUNT;
    update_fs.pSamplerImageViews = combined_info;

    err = xglAllocDescriptorSets(demo->desc_region,
            XGL_DESCRIPTOR_SET_USAGE_STATIC,
            1, demo->desc_layout_last,
            &demo->desc_set, &count);
    assert(!err && count == 1);

    xglBeginDescriptorRegionUpdate(demo->device,
            XGL_DESCRIPTOR_UPDATE_MODE_FASTEST);

    xglClearDescriptorSets(demo->desc_region, 1, &demo->desc_set);
    xglUpdateDescriptors(demo->desc_set, &update_vs);

    xglEndDescriptorRegionUpdate(demo->device, demo->cmd);
}

static void demo_prepare(struct demo *demo)
{
    const XGL_CMD_BUFFER_CREATE_INFO cmd = {
        .sType = XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .queueType = XGL_QUEUE_TYPE_GRAPHICS,
        .flags = 0,
    };
    XGL_RESULT err;

    demo_prepare_buffers(demo);
    demo_prepare_depth(demo);
    demo_prepare_textures(demo);
    demo_prepare_cube_data_buffer(demo);

    demo_prepare_descriptor_layout(demo);
    demo_prepare_pipeline(demo);
    demo_prepare_dynamic_states(demo);

    err = xglCreateCommandBuffer(demo->device, &cmd, &demo->cmd);
    assert(!err);

    demo_prepare_descriptor_region(demo);
    demo_prepare_descriptor_set(demo);
}

static void demo_handle_event(struct demo *demo,
                              const xcb_generic_event_t *event)
{
    u_int8_t event_code = event->response_type & 0x7f;
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
        xglDeviceWaitIdle(demo->device);
        demo_update_data_buffer(demo);

        demo_draw(demo);

        // Wait for work to finish before updating MVP.
        xglDeviceWaitIdle(demo->device);
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

static void demo_init_xgl(struct demo *demo)
{
    const XGL_APPLICATION_INFO app = {
        .sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pAppName = "cube",
        .appVersion = 0,
        .pEngineName = "cube",
        .engineVersion = 0,
        .apiVersion = XGL_MAKE_VERSION(0, 22, 0),
    };
    const XGL_WSI_X11_CONNECTION_INFO connection = {
        .pConnection = demo->connection,
        .root = demo->screen->root,
        .provider = 0,
    };
    const XGL_DEVICE_QUEUE_CREATE_INFO queue = {
        .queueNodeIndex = 0,
        .queueCount = 1,
    };
    const XGL_CHAR *ext_names[] = {
        "XGL_WSI_X11",
    };
    const XGL_DEVICE_CREATE_INFO device = {
        .sType = XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueRecordCount = 1,
        .pRequestedQueues = &queue,
        .extensionCount = 1,
        .ppEnabledExtensionNames = ext_names,
        .maxValidationLevel = XGL_VALIDATION_LEVEL_END_RANGE,
        .flags = XGL_DEVICE_CREATE_VALIDATION_BIT,
    };
    XGL_RESULT err;
    XGL_UINT gpu_count;
    XGL_UINT i;

    err = xglInitAndEnumerateGpus(&app, NULL, 1, &gpu_count, &demo->gpu);
    assert(!err && gpu_count == 1);

    for (i = 0; i < device.extensionCount; i++) {
        err = xglGetExtensionSupport(demo->gpu, ext_names[i]);
        assert(!err);
    }

    err = xglWsiX11AssociateConnection(demo->gpu, &connection);
    assert(!err);

    err = xglCreateDevice(demo->gpu, &device, &demo->device);
    assert(!err);

    err = xglGetDeviceQueue(demo->device, XGL_QUEUE_TYPE_GRAPHICS,
            0, &demo->queue);
    assert(!err);
}

static void demo_init_connection(struct demo *demo)
{
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    demo->connection = xcb_connect(NULL, &scr);

    setup = xcb_get_setup(demo->connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);

    demo->screen = iter.data;
}

static void demo_init(struct demo *demo)
{
    vec3 eye = {0.0f, 3.0f, 5.0f};
    vec3 origin = {0, 0, 0};
    vec3 up = {0.0f, -1.0f, 0.0};

    memset(demo, 0, sizeof(*demo));

    demo_init_connection(demo);
    demo_init_xgl(demo);

    demo->width = 500;
    demo->height = 500;
    demo->format = XGL_FMT_B8G8R8A8_UNORM;

    demo->spin_angle = 0.01f;
    demo->spin_increment = 0.01f;
    demo->pause = false;

    mat4x4_perspective(demo->projection_matrix, degreesToRadians(45.0f), 1.0f, 0.1f, 100.0f);
    mat4x4_look_at(demo->view_matrix, eye, origin, up);
    mat4x4_identity(demo->model_matrix);
}

static void demo_cleanup(struct demo *demo)
{
    XGL_UINT i, j;

    xglDestroyObject(demo->desc_set);
    xglDestroyObject(demo->desc_region);

    xglDestroyObject(demo->cmd);

    xglDestroyObject(demo->viewport);
    xglDestroyObject(demo->raster);
    xglDestroyObject(demo->color_blend);
    xglDestroyObject(demo->depth_stencil);

    xglDestroyObject(demo->pipeline);
    xglDestroyObject(demo->desc_layout_fs);
    xglDestroyObject(demo->desc_layout_vs);

//    xglFreeMemory(demo->vertices.mem);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        xglDestroyObject(demo->textures[i].view);
        xglDestroyObject(demo->textures[i].image);
        for (j = 0; j < demo->textures[i].num_mem; j++)
            xglFreeMemory(demo->textures[i].mem[j]);
        xglDestroyObject(demo->textures[i].sampler);
    }

    xglDestroyObject(demo->depth.view);
    xglDestroyObject(demo->depth.image);
    for (j = 0; j < demo->depth.num_mem; j++)
        xglFreeMemory(demo->depth.mem[j]);
    xglDestroyObject(demo->uniform_data.buf);
    for (j = 0; j < demo->uniform_data.num_mem; j++)
        xglFreeMemory(demo->uniform_data.mem[j]);

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        xglDestroyObject(demo->buffers[i].fence);
        xglDestroyObject(demo->buffers[i].view);
        xglDestroyObject(demo->buffers[i].image);
    }

    xglDestroyDevice(demo->device);

    xcb_destroy_window(demo->connection, demo->window);
    xcb_disconnect(demo->connection);
}

int main(void)
{
    struct demo demo;

    demo_init(&demo);

    demo_prepare(&demo);
    demo_create_window(&demo);
    demo_run(&demo);

    demo_cleanup(&demo);

    return 0;
}
