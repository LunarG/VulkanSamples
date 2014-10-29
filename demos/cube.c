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
#include <png.h>

#define DEMO_BUFFER_COUNT 2
#define DEMO_TEXTURE_COUNT 1

static char *tex_files[] = {
    "demos/lunarg-logo-256x256-solid.png"
};

// HACK
bool do_tri = false;

struct xgltriangle_vs_uniform {
    // Must start with MVP
    XGL_FLOAT   mvp[4][4];
    XGL_FLOAT   position[3][4];
    XGL_FLOAT   color[3][4];
};

struct xgltextriangle_vs_uniform {
    // Must start with MVP
    XGL_FLOAT   mvp[4][4];
    XGL_FLOAT   position[3][4];
    XGL_FLOAT   attr[3][4];
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
    0.0f, 1.0f,  // Vertex 0
    1.0f, 1.0f,
    1.0f, 0.0f,

    1.0f, 0.0f,  // Vertex 1
    0.0f, 0.0f,
    0.0f, 1.0f,

    0.0f, 1.0f,  // Vertex 2
    1.0f, 0.0f,
    0.0f, 0.0f,

    0.0f, 1.0f,  // Vertex 3
    1.0f, 0.0f,
    1.0f, 1.0f,

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

    0.0f, 0.0f,  // Vertex 8
    0.0f, 1.0f,
    1.0f, 1.0f,

    1.0f, 1.0f,  // Vertex 9
    1.0f, 0.0f,
    0.0f, 0.0f,

    0.0f, 0.0f,  // Vertex 10
    1.0f, 0.0f,
    0.0f, 1.0f,

    0.0f, 1.0f,  // Vertex 11
    1.0f, 1.0f,
    1.0f, 0.0f,
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
    } buffers[DEMO_BUFFER_COUNT];

    struct {
        XGL_FORMAT format;

        XGL_IMAGE image;
        XGL_GPU_MEMORY mem;
        XGL_DEPTH_STENCIL_VIEW view;
    } depth;

    struct {
        XGL_SAMPLER sampler;

        char *filename;
        XGL_IMAGE image;
        XGL_GPU_MEMORY mem;
        XGL_IMAGE_VIEW view;
    } textures[DEMO_TEXTURE_COUNT];

    struct {
        XGL_GPU_MEMORY mem;
        XGL_MEMORY_VIEW_ATTACH_INFO view;

        XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO vi;
        XGL_VERTEX_INPUT_BINDING_DESCRIPTION vi_bindings[1];
        XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION vi_attrs[2];
    } vertices;

    struct {
        XGL_GPU_MEMORY mem;
        XGL_MEMORY_VIEW_ATTACH_INFO view;
    } uniform_data;

    XGL_DESCRIPTOR_SET dset;

    XGL_PIPELINE pipeline;

    XGL_VIEWPORT_STATE_OBJECT viewport;
    XGL_RASTER_STATE_OBJECT raster;
    XGL_MSAA_STATE_OBJECT msaa;
    XGL_COLOR_BLEND_STATE_OBJECT color_blend;
    XGL_DEPTH_STENCIL_STATE_OBJECT depth_stencil;

    mat4x4 projection_matrix;
    mat4x4 view_matrix;
    mat4x4 model_matrix;

    XGL_CMD_BUFFER cmd;

    xcb_window_t window;

    bool quit;
    XGL_UINT current_buffer;
};

static void demo_draw_build_cmd(struct demo *demo)
{
    const XGL_COLOR_ATTACHMENT_BIND_INFO color_attachment = {
        .view = demo->buffers[demo->current_buffer].view,
        .colorAttachmentState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL,
    };
    const XGL_DEPTH_STENCIL_BIND_INFO depth_stencil = {
        .view = demo->depth.view,
        .depthState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL,
        .stencilState = XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL,
    };
    const XGL_FLOAT clear_color[4] = { 0.2f, 0.2f, 0.2f, 0.2f };
    const XGL_FLOAT clear_depth = 1.0f;
    XGL_IMAGE_SUBRESOURCE_RANGE clear_range;
    XGL_RESULT err;

    err = xglBeginCommandBuffer(demo->cmd,
            XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT |
            XGL_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT);
    assert(!err);

    xglCmdBindPipeline(demo->cmd, XGL_PIPELINE_BIND_POINT_GRAPHICS,
                                  demo->pipeline);
    xglCmdBindDescriptorSet(demo->cmd, XGL_PIPELINE_BIND_POINT_GRAPHICS,
            0, demo->dset, 0);

    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_VIEWPORT, demo->viewport);
    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_RASTER, demo->raster);
    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_MSAA, demo->msaa);
    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_COLOR_BLEND,
                                     demo->color_blend);
    xglCmdBindStateObject(demo->cmd, XGL_STATE_BIND_DEPTH_STENCIL,
                                     demo->depth_stencil);

    xglCmdBindAttachments(demo->cmd, 1, &color_attachment, &depth_stencil);

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

    if (do_tri) {
        xglCmdDraw(demo->cmd, 0, 3, 0, 1);
    } else {
        xglCmdDraw(demo->cmd, 0, 12 * 3, 0, 1);
    }

    err = xglEndCommandBuffer(demo->cmd);
    assert(!err);
}


void demo_update_data_buffer(struct demo *demo, bool forward)
{
    mat4x4 MVP, Model, VP;
    int matrixSize = sizeof(MVP);
    XGL_UINT8 *pData;
    XGL_RESULT err;

    mat4x4_mul(VP, demo->projection_matrix, demo->view_matrix);

    // Rotate 22.5 degrees around the Y axis
    mat4x4_dup(Model, demo->model_matrix);
    if (forward) {
        mat4x4_rotate(demo->model_matrix, Model, 0.0f, 1.0f, 0.0f, degreesToRadians(22.5f));
    } else {
        mat4x4_rotate(demo->model_matrix, Model, 0.0f, 1.0f, 0.0f, degreesToRadians(-22.5f));
    }
    mat4x4_mul(MVP, VP, demo->model_matrix);

    err = xglMapMemory(demo->uniform_data.mem, 0, (XGL_VOID **) &pData);
    assert(!err);

    memcpy(pData, (const void*) &MVP[0][0], matrixSize);

    err = xglUnmapMemory(demo->uniform_data.mem);
    assert(!err);
}

static void demo_draw(struct demo *demo)
{
    const XGL_WSI_X11_PRESENT_INFO present = {
        .destWindow = demo->window,
        .srcImage = demo->buffers[demo->current_buffer].image,
    };
    XGL_RESULT err;

    demo_draw_build_cmd(demo);

    err = xglQueueSubmit(demo->queue, 1, &demo->cmd,
            0, NULL, XGL_NULL_HANDLE);
    assert(!err);

    err = xglWsiX11QueuePresent(demo->queue, &present, XGL_NULL_HANDLE);
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
    }
}

static void demo_prepare_depth(struct demo *demo)
{
    const XGL_FORMAT depth_format = { XGL_CH_FMT_R16, XGL_NUM_FMT_DS };
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
    XGL_MEMORY_ALLOC_INFO mem_alloc = {
        .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .alignment = 0,
        .flags = 0,
        .heapCount = 0,
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
    XGL_MEMORY_REQUIREMENTS mem_reqs;
    XGL_SIZE mem_reqs_size;
    XGL_RESULT err;

    demo->depth.format = depth_format;

    /* create image */
    err = xglCreateImage(demo->device, &image,
            &demo->depth.image);
    assert(!err);

    err = xglGetObjectInfo(demo->depth.image,
            XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
            &mem_reqs_size, &mem_reqs);
    assert(!err && mem_reqs_size == sizeof(mem_reqs));

    mem_alloc.allocationSize = mem_reqs.size;
    mem_alloc.alignment = mem_reqs.alignment;
    mem_alloc.heapCount = mem_reqs.heapCount;
    memcpy(mem_alloc.heaps, mem_reqs.heaps,
            sizeof(mem_reqs.heaps[0]) * mem_reqs.heapCount);

    /* allocate memory */
    err = xglAllocMemory(demo->device, &mem_alloc,
            &demo->depth.mem);
    assert(!err);

    /* bind memory */
    err = xglBindObjectMemory(demo->depth.image,
            demo->depth.mem, 0);
    assert(!err);

    /* create image view */
    view.image = demo->depth.image;
    err = xglCreateDepthStencilView(demo->device, &view,
            &demo->depth.view);
    assert(!err);
}

#if 1
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
#endif

static void demo_prepare_textures(struct demo *demo)
{
    const XGL_FORMAT tex_format = { XGL_CH_FMT_R8G8B8A8, XGL_NUM_FMT_UNORM };
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
        XGL_MEMORY_ALLOC_INFO mem_alloc = {
            .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
            .pNext = NULL,
            .allocationSize = 0,
            .alignment = 0,
            .flags = 0,
            .heapCount = 0,
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
        XGL_MEMORY_REQUIREMENTS mem_reqs;
        XGL_SIZE mem_reqs_size;

        /* create sampler */
        err = xglCreateSampler(demo->device, &sampler,
                &demo->textures[i].sampler);
        assert(!err);

        /* create image */
        err = xglCreateImage(demo->device, &image,
                &demo->textures[i].image);
        assert(!err);

        err = xglGetObjectInfo(demo->textures[i].image,
                XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                &mem_reqs_size, &mem_reqs);
        assert(!err && mem_reqs_size == sizeof(mem_reqs));

        mem_alloc.allocationSize = mem_reqs.size;
        mem_alloc.alignment = mem_reqs.alignment;
        mem_alloc.heapCount = mem_reqs.heapCount;
        memcpy(mem_alloc.heaps, mem_reqs.heaps,
                sizeof(mem_reqs.heaps[0]) * mem_reqs.heapCount);

        /* allocate memory */
        err = xglAllocMemory(demo->device, &mem_alloc,
                &demo->textures[i].mem);
        assert(!err);

        /* bind memory */
        err = xglBindObjectMemory(demo->textures[i].image,
                demo->textures[i].mem, 0);
        assert(!err);

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
        XGL_SIZE layout_size;
        XGL_VOID *data;

        err = xglGetImageSubresourceInfo(demo->textures[i].image, &subres,
                XGL_INFO_TYPE_SUBRESOURCE_LAYOUT, &layout_size, &layout);
        assert(!err && layout_size == sizeof(layout));

        err = xglMapMemory(demo->textures[i].mem, 0, &data);
        assert(!err);

        loadTexture(tex_files[i], data, &layout, &tex_width, &tex_height);

        err = xglUnmapMemory(demo->textures[i].mem);
        assert(!err);
    }
}

void demo_prepare_cube_data_buffer(struct demo *demo)
{
    XGL_MEMORY_ALLOC_INFO alloc_info;
    XGL_UINT8 *pData;
    int i;
    mat4x4 MVP, VP;
    vec3 eye = {0.0f, 3.0f, 10.0f};
    vec3 origin = {0, 0, 0};
    vec3 up = {0.0f, 1.0f, 0.0};
    XGL_RESULT err;
    struct xgltexcube_vs_uniform data;

    mat4x4_perspective(demo->projection_matrix, degreesToRadians(45.0f), 1.0f, 0.1f, 100.0f);
    mat4x4_look_at(demo->view_matrix, eye, origin, up);
//    mat4x4_identity(demo->projection_matrix);
//    mat4x4_identity(demo->view_matrix);
    mat4x4_mul(VP, demo->projection_matrix, demo->view_matrix);
//    mat4x4_translate(demo->model_matrix, 0.25, 0, 0);
    mat4x4_identity(demo->model_matrix);
    mat4x4_mul(MVP, VP, demo->model_matrix);
    memcpy(data.mvp, MVP, sizeof(MVP));
    dumpMatrix("MVP", MVP);

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

    memset(&alloc_info, 0, sizeof(alloc_info));
    alloc_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.allocationSize = sizeof(data);
    alloc_info.alignment = 0;
    alloc_info.heapCount = 1;
    alloc_info.heaps[0] = 0; // TODO: Use known existing heap

    alloc_info.flags = XGL_MEMORY_HEAP_CPU_VISIBLE_BIT;
    alloc_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

    err = xglAllocMemory(demo->device, &alloc_info, &demo->uniform_data.mem);
    assert(!err);

    err = xglMapMemory(demo->uniform_data.mem, 0, (XGL_VOID **) &pData);
    assert(!err);

    memcpy(pData, &data, alloc_info.allocationSize);

    err = xglUnmapMemory(demo->uniform_data.mem);
    assert(!err);

    // set up the memory view for the constant buffer
    demo->uniform_data.view.sType = XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO;
    demo->uniform_data.view.stride = 16;
    demo->uniform_data.view.range  = alloc_info.allocationSize;
    demo->uniform_data.view.offset = 0;
    demo->uniform_data.view.mem    = demo->uniform_data.mem;
    demo->uniform_data.view.format.channelFormat = XGL_CH_FMT_R32G32B32A32;
    demo->uniform_data.view.format.numericFormat = XGL_NUM_FMT_FLOAT;
}

#if 0
// this function will create the vertex buffer and fill it with the mesh data
void demo_prepare_mesh( struct demo *demo )
{
    XGL_UINT32 m_numVertices;
    XGL_GPU_SIZE vbStride;
    XGL_RESULT err = XGL_SUCCESS;

    m_numVertices = sizeof(g_vb_solid_face_colors_Data)/sizeof(g_vb_solid_face_colors_Data[0]);
    vbStride = sizeof(g_vb_solid_face_colors_Data[0]);

    XGL_MEMORY_ALLOC_INFO alloc_info;
    XGL_UINT8 *pData;

    memset(&alloc_info, 0, sizeof(alloc_info));
    alloc_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
//    alloc_info.pNext = NULL;
    alloc_info.allocationSize = m_numVertices * vbStride;
    alloc_info.alignment = 0;
    alloc_info.heapCount = 1;
    alloc_info.heaps[0] = 0; // TODO: Use known existing heap

    alloc_info.flags = XGL_MEMORY_HEAP_CPU_VISIBLE_BIT;
    alloc_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

    err = xglAllocMemory(demo->device, &alloc_info, &demo->vertices.mem);
    assert(!err);

    err = xglMapMemory(demo->vertices.mem, 0, (XGL_VOID **) &pData);
    assert(!err);

    memcpy(pData, g_vb_solid_face_colors_Data, alloc_info.allocationSize);

    err = xglUnmapMemory(demo->vertices.mem);
    assert(!err);

    // set up the memory view for the vertex buffer
    demo->vertices.view.sType = XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO;
    demo->vertices.view.pNext = NULL;
    demo->vertices.view.mem    = demo->vertices.mem;
    demo->vertices.view.offset = 0;
    demo->vertices.view.range  = sizeof(g_vb_solid_face_colors_Data);
    demo->vertices.view.stride = vbStride;
    demo->vertices.view.format.channelFormat = XGL_CH_FMT_UNDEFINED;
    demo->vertices.view.format.numericFormat = XGL_NUM_FMT_UNDEFINED;
    demo->vertices.view.state = XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY;

    demo->vertices.vi_bindings[0].strideInBytes = vbStride;  // strideInBytes;  Distance between vertices in bytes (0 = no advancement)
    demo->vertices.vi_bindings[0].stepRate = XGL_VERTEX_INPUT_STEP_RATE_VERTEX; // stepRate;

    // this is the current description of g_vbData
    demo->vertices.vi_attrs[0].binding = 0;                       // index into vertexBindingDescriptions
    demo->vertices.vi_attrs[0].format.channelFormat = XGL_CH_FMT_R32G32B32A32;            // format of source data
    demo->vertices.vi_attrs[0].format.numericFormat = XGL_NUM_FMT_FLOAT;
    demo->vertices.vi_attrs[0].offsetInBytes = 0;                 // Offset of first element in bytes from base of vertex

    demo->vertices.vi_attrs[1].binding = 0;                       // index into vertexBindingDescriptions
    demo->vertices.vi_attrs[1].format.channelFormat = XGL_CH_FMT_R32G32B32A32;            // format of source data
    demo->vertices.vi_attrs[1].format.numericFormat = XGL_NUM_FMT_FLOAT;
    demo->vertices.vi_attrs[1].offsetInBytes = sizeof(float) * 4;                 // Offset of first element in bytes from base of vertex

    demo->vertices.vi.sType = XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO;
    demo->vertices.vi.pNext = NULL;
    demo->vertices.vi.bindingCount = 1;
    demo->vertices.vi.pVertexBindingDescriptions = demo->vertices.vi_bindings;
    demo->vertices.vi.attributeCount = 2;
    demo->vertices.vi.pVertexAttributeDescriptions = demo->vertices.vi_attrs;
}
#endif

#if 0
static void demo_prepare_vertices(struct demo *demo)
{
    const float vb[3][5] = {
        /*      position             texcoord */
        { -1.0f, -1.0f, -0.6f,      0.0f, 0.0f },
        {  1.0f, -1.0f, -0.5f,      1.0f, 0.0f },
        {  0.0f,  1.0f,  1.0f,      0.5f, 1.0f },
    };

    const XGL_MEMORY_ALLOC_INFO mem_alloc = {
        .sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = NULL,
        .allocationSize = sizeof(vb),
        .alignment = 0,
        .flags = 0,
        .heapCount = 1,
        .heaps[0] = 0,
        .memPriority = XGL_MEMORY_PRIORITY_NORMAL,
    };
    XGL_RESULT err;
    void *data;

    memset(&demo->vertices, 0, sizeof(demo->vertices));

    err = xglAllocMemory(demo->device, &mem_alloc, &demo->vertices.mem);
    XGL_RESULT err;    assert(!err);

    err = xglMapMemory(demo->vertices.mem, 0, &data);
    assert(!err);

    memcpy(data, vb, sizeof(vb));

    err = xglUnmapMemory(demo->vertices.mem);
    assert(!err);

    demo->vertices.view.sType = XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO;
    demo->vertices.view.pNext = NULL;
    demo->vertices.view.mem = demo->vertices.mem;
    demo->vertices.view.offset = 0;
    demo->vertices.view.range = sizeof(vb);
    demo->vertices.view.stride = sizeof(vb[0]);
    demo->vertices.view.format.channelFormat = XGL_CH_FMT_UNDEFINED;
    demo->vertices.view.format.numericFormat = XGL_NUM_FMT_UNDEFINED;
    demo->vertices.view.state = XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY;

    demo->vertices.vi.sType = XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO;
    demo->vertices.vi.pNext = NULL;
    demo->vertices.vi.bindingCount = 1;
    demo->vertices.vi.pVertexBindingDescriptions = demo->vertices.vi_bindings;
    demo->vertices.vi.attributeCount = 2;
    demo->vertices.vi.pVertexAttributeDescriptions = demo->vertices.vi_attrs;

    demo->vertices.vi_bindings[0].strideInBytes = sizeof(vb[0]);
    demo->vertices.vi_bindings[0].stepRate = XGL_VERTEX_INPUT_STEP_RATE_VERTEX;

    demo->vertices.vi_attrs[0].binding = 0;
    demo->vertices.vi_attrs[0].format.channelFormat = XGL_CH_FMT_R32G32B32;
    demo->vertices.vi_attrs[0].format.numericFormat = XGL_NUM_FMT_FLOAT;
    demo->vertices.vi_attrs[0].offsetInBytes = 0;

    demo->vertices.vi_attrs[1].binding = 0;
    demo->vertices.vi_attrs[1].format.channelFormat = XGL_CH_FMT_R32G32;
    demo->vertices.vi_attrs[1].format.numericFormat = XGL_NUM_FMT_FLOAT;
    demo->vertices.vi_attrs[1].offsetInBytes = sizeof(float) * 3;
}
#endif

void demo_prepare_tri_data_buffer(struct demo *demo)
{
    XGL_MEMORY_ALLOC_INFO alloc_info;
    XGL_UINT8 *pData;
    int i;
    mat4x4 MVP, VP;
    vec3 eye = {0.0f, 3.0f, 10.0f};
    vec3 origin = {0, 0, 0};
    vec3 up = {0.0f, 1.0f, 0.0};
    struct xgltextriangle_vs_uniform data;
    const struct VertexPosTex tri_data[] =
    {
        /*      position                texcoord */
//        { XYZ1(-1.0f, -1.0f, -0.6f),    UV(0.0f, 0.0f) },
//        { XYZ1(1.0f, -1.0f, -0.5f),     UV(1.0f, 0.0f) },
//        { XYZ1(0.0f,  1.0f,  1.0f),     UV(0.5f, 1.0f) },
        { XYZ1(-1.0f, -1.0f, 0.0f),    UV(0.0f, 0.0f) },
        { XYZ1(1.0f, -1.0f, 0.0f),     UV(1.0f, 0.0f) },
        { XYZ1(-1.0f,  1.0f,  0.0f),     UV(0.0f, 1.0f) },
    };
    XGL_RESULT err;

//    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
//    dumpMatrix("Projection", Projection);

    // Camera matrix
   /* glm::mat4 View       = glm::lookAt(
                               glm::vec3(0,3,10), // Camera is at (0,3,10), in World Space
                               glm::vec3(0,0,0), // and looks at the origin
                               glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                               );
                               */
    mat4x4_perspective(demo->projection_matrix, degreesToRadians(45.0f), 1.0f, 0.1f, 100.0f);
    dumpMatrix("Projection", demo->projection_matrix);
    mat4x4_look_at(demo->view_matrix, eye, origin, up);
    dumpMatrix("View", demo->view_matrix);
//    mat4x4_identity(demo->projection_matrix);
//    mat4x4_identity(demo->view_matrix);
    mat4x4_mul(VP, demo->projection_matrix, demo->view_matrix);
    mat4x4_identity(demo->model_matrix);
    mat4x4_mul(MVP, VP, demo->model_matrix);
    mat4x4_identity(MVP);
//    mat4x4_translate(MVP, 0.25, 0, 0);
    dumpMatrix("MVP", MVP);
    memcpy(data.mvp, MVP, sizeof(MVP));

    for (i=0; i<3; i++) {
        data.position[i][0] = tri_data[i].posX;
        data.position[i][1] = tri_data[i].posY;
        data.position[i][2] = tri_data[i].posZ;
        data.position[i][3] = tri_data[i].posW;
        data.attr[i][0] = tri_data[i].u;
        data.attr[i][1] = tri_data[i].v;
        data.attr[i][2] = tri_data[i].s;
        data.attr[i][3] = tri_data[i].t;
    }

    memset(&alloc_info, 0, sizeof(alloc_info));
    alloc_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.allocationSize = sizeof(data);
    alloc_info.alignment = 0;
    alloc_info.heapCount = 1;
    alloc_info.heaps[0] = 0; // TODO: Use known existing heap

    alloc_info.flags = XGL_MEMORY_HEAP_CPU_VISIBLE_BIT;
    alloc_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

    err = xglAllocMemory(demo->device, &alloc_info, &demo->uniform_data.mem);
    assert(!err);

    err = xglMapMemory(demo->uniform_data.mem, 0, (XGL_VOID **) &pData);
    assert(!err);

    memcpy(pData, &data, alloc_info.allocationSize);

    err = xglUnmapMemory(demo->uniform_data.mem);
    assert(!err);

    // set up the memory view for the constant buffer
    demo->uniform_data.view.sType = XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO;
    demo->uniform_data.view.stride = 16;
    demo->uniform_data.view.range  = alloc_info.allocationSize;
    demo->uniform_data.view.offset = 0;
    demo->uniform_data.view.mem    = demo->uniform_data.mem;
    demo->uniform_data.view.format.channelFormat = XGL_CH_FMT_R32G32B32A32;
    demo->uniform_data.view.format.numericFormat = XGL_NUM_FMT_FLOAT;
}

static void demo_prepare_descriptor_set(struct demo *demo)
{
    const XGL_DESCRIPTOR_SET_CREATE_INFO descriptor_set = {
        .sType = XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO,
        .pNext = NULL,
        .slots = DEMO_TEXTURE_COUNT * 2 + 2,
    };
    XGL_RESULT err;

    err = xglCreateDescriptorSet(demo->device, &descriptor_set, &demo->dset);
    assert(!err);

    xglBeginDescriptorSetUpdate(demo->dset);
    xglClearDescriptorSetSlots(demo->dset, 0, DEMO_TEXTURE_COUNT * 2 + 2);

//    xglAttachMemoryViewDescriptors(demo->dset, 0, 1, &demo->vertices.view);

    xglAttachMemoryViewDescriptors(demo->dset, 0, 1, &demo->uniform_data.view);

    XGL_IMAGE_VIEW_ATTACH_INFO image_view;

    image_view.sType = XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO;
    image_view.pNext = NULL;
    image_view.view = demo->textures[0].view;
    image_view.state = XGL_IMAGE_STATE_GRAPHICS_SHADER_READ_ONLY;

    xglAttachSamplerDescriptors(demo->dset, 1, 1, &demo->textures[0].sampler);
    xglAttachImageViewDescriptors(demo->dset, 2, 1, &image_view);

    xglEndDescriptorSetUpdate(demo->dset);
}

#define USE_BIL

static XGL_SHADER demo_prepare_shader(struct demo *demo,
                                      XGL_PIPELINE_SHADER_STAGE stage,
                                      const void *code,
                                      XGL_SIZE size)
{
    XGL_SHADER_CREATE_INFO createInfo;
    XGL_SHADER shader;
    XGL_RESULT err;

#ifdef USE_BIL
    createInfo.sType = XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;

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

static XGL_SHADER demo_prepare_vs(struct demo *demo)
{
    if (do_tri) {
        static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "\n"
            "layout(binding = 0) uniform buf {\n"
            "        mat4 MVP;\n"
            "        vec4 position[3];\n"
            "        vec4 attr[3];\n"
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
    } else {
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
    }
}

static XGL_SHADER demo_prepare_fs(struct demo *demo)
{
    if (do_tri) {
#if 1
        static const char *fragShaderText =
                "#version 140\n"
                "#extension GL_ARB_separate_shader_objects : enable\n"
                "#extension GL_ARB_shading_language_420pack : enable\n"
                "uniform sampler2D tex;\n"
//                "in vec2 texcoord;\n"
                "layout (location = 0) in vec4 texcoord;\n"
                "void main() {\n"
                "   gl_FragColor = texture(tex, texcoord.xy);\n"
//                "   gl_FragColor = texture(tex, vec2(0, 0));\n"
//                "   gl_FragColor = texcoord;"
//                "   gl_FragColor = vec4(0, 1, 0, 1);"
                "}\n";
#endif

        return demo_prepare_shader(demo, XGL_SHADER_STAGE_FRAGMENT,
                                   (const void *) fragShaderText,
                                   strlen(fragShaderText));
    } else {
        static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "uniform sampler2D tex;\n"
            "\n"
            "layout (location = 0) in vec4 texcoord;\n"
            "void main() {\n"
            "   gl_FragColor = texture(tex, texcoord.xy);\n"
            "}\n";

        return demo_prepare_shader(demo, XGL_SHADER_STAGE_FRAGMENT,
                                   (const void *) fragShaderText,
                                   strlen(fragShaderText));
    }
}

static void demo_prepare_pipeline(struct demo *demo)
{
    XGL_GRAPHICS_PIPELINE_CREATE_INFO pipeline;
//    XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO vi;
    XGL_PIPELINE_IA_STATE_CREATE_INFO ia;
    XGL_PIPELINE_RS_STATE_CREATE_INFO rs;
    XGL_PIPELINE_CB_STATE cb;
    XGL_PIPELINE_DB_STATE_CREATE_INFO db;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO vs;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO fs;
    XGL_DESCRIPTOR_SLOT_INFO vs_slots[3];
    XGL_DESCRIPTOR_SLOT_INFO fs_slots[3];
    XGL_RESULT err;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    memset(&ia, 0, sizeof(ia));
    ia.sType = XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO;
    ia.topology = XGL_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO;

    memset(&cb, 0, sizeof(cb));
    cb.sType = XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO;
    cb.attachment[0].format = demo->format;
    cb.attachment[0].channelWriteMask = 0xf;

    memset(&db, 0, sizeof(db));
    db.sType = XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO;
    db.format = demo->depth.format;

    memset(&vs_slots, 0, sizeof(vs_slots));
    vs_slots[0].slotObjectType = XGL_SLOT_SHADER_RESOURCE;
    vs_slots[0].shaderEntityIndex = 0;

    memset(&fs_slots, 0, sizeof(fs_slots));
    fs_slots[1].slotObjectType = XGL_SLOT_SHADER_SAMPLER;
    fs_slots[1].shaderEntityIndex = 0;
    fs_slots[2].slotObjectType = XGL_SLOT_SHADER_RESOURCE;
    fs_slots[2].shaderEntityIndex = 0;

    memset(&vs, 0, sizeof(vs));
    vs.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs.shader.stage = XGL_SHADER_STAGE_VERTEX;
    vs.shader.shader = demo_prepare_vs(demo);
    assert(vs.shader.shader != NULL);
    vs.shader.descriptorSetMapping[0].descriptorCount = 3;
    vs.shader.descriptorSetMapping[0].pDescriptorInfo = vs_slots;

    memset(&fs, 0, sizeof(fs));
    fs.sType = XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fs.shader.stage = XGL_SHADER_STAGE_FRAGMENT;
    fs.shader.shader = demo_prepare_fs(demo);
    assert(fs.shader.shader != NULL);
    fs.shader.descriptorSetMapping[0].descriptorCount = 3;
    fs.shader.descriptorSetMapping[0].pDescriptorInfo = fs_slots;

    pipeline.pNext = (const XGL_VOID *) &ia;
    ia.pNext = (const XGL_VOID *) &rs;
    rs.pNext = (const XGL_VOID *) &cb;
    cb.pNext = (const XGL_VOID *) &db;
    db.pNext = (const XGL_VOID *) &vs;
    vs.pNext = (const XGL_VOID *) &fs;

    err = xglCreateGraphicsPipeline(demo->device, &pipeline, &demo->pipeline);
    assert(!err);

    xglDestroyObject(vs.shader.shader);
    xglDestroyObject(fs.shader.shader);
}

static void demo_prepare_dynamic_states(struct demo *demo)
{
    XGL_VIEWPORT_STATE_CREATE_INFO viewport;
    XGL_RASTER_STATE_CREATE_INFO raster;
    XGL_MSAA_STATE_CREATE_INFO msaa;
    XGL_COLOR_BLEND_STATE_CREATE_INFO color_blend;
    XGL_DEPTH_STENCIL_STATE_CREATE_INFO depth_stencil;
    XGL_RESULT err;

    memset(&viewport, 0, sizeof(viewport));
    viewport.viewportCount = 1;
    viewport.scissorEnable = XGL_FALSE;
    viewport.viewports[0].width = (XGL_FLOAT) demo->width;
    viewport.viewports[0].height = (XGL_FLOAT) demo->height;
    viewport.viewports[0].minDepth = (XGL_FLOAT) 0.0f;
    viewport.viewports[0].maxDepth = (XGL_FLOAT) 1.0f;

    memset(&raster, 0, sizeof(raster));
    raster.sType = XGL_STRUCTURE_TYPE_RASTER_STATE_CREATE_INFO;
    raster.fillMode = XGL_FILL_SOLID;
    raster.cullMode = XGL_CULL_NONE;
    raster.frontFace = XGL_FRONT_FACE_CCW;

    memset(&msaa, 0, sizeof(msaa));
    msaa.sType = XGL_STRUCTURE_TYPE_MSAA_STATE_CREATE_INFO;
    msaa.samples = 1;
    msaa.sampleMask = 0x1;

    memset(&color_blend, 0, sizeof(color_blend));
    color_blend.sType = XGL_STRUCTURE_TYPE_COLOR_BLEND_STATE_CREATE_INFO;

    memset(&depth_stencil, 0, sizeof(depth_stencil));
    depth_stencil.sType = XGL_STRUCTURE_TYPE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = XGL_TRUE;
    depth_stencil.depthWriteEnable = XGL_TRUE;
    depth_stencil.depthFunc = XGL_COMPARE_LESS_EQUAL;
    depth_stencil.depthBoundsEnable = XGL_FALSE;

    err = xglCreateViewportState(demo->device, &viewport, &demo->viewport);
    assert(!err);

    err = xglCreateRasterState(demo->device, &raster, &demo->raster);
    assert(!err);

    err = xglCreateMsaaState(demo->device, &msaa, &demo->msaa);
    assert(!err);

    err = xglCreateColorBlendState(demo->device,
            &color_blend, &demo->color_blend);
    assert(!err);

    err = xglCreateDepthStencilState(demo->device,
            &depth_stencil, &demo->depth_stencil);
    assert(!err);
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
    if (do_tri) {
//         demo_prepare_vertices(demo);
        demo_prepare_tri_data_buffer(demo);
    } else {
        demo_prepare_cube_data_buffer(demo);
    }
    demo_prepare_descriptor_set(demo);

    demo_prepare_pipeline(demo);
    demo_prepare_dynamic_states(demo);

    err = xglCreateCommandBuffer(demo->device, &cmd, &demo->cmd);
    assert(!err);
}

static void demo_handle_event(struct demo *demo,
                              const xcb_generic_event_t *event)
{
    switch (event->response_type & 0x7f) {
    case XCB_EXPOSE:
        demo_draw(demo);
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
                // Wait for work to finish before updating MVP.
                xglDeviceWaitIdle(demo->device);

                demo_update_data_buffer(demo, false);

                demo_draw(demo);
                break;
            case 0x72:          // right arrow key
                // Wait for work to finish before updating MVP.
                xglDeviceWaitIdle(demo->device);

                demo_update_data_buffer(demo, true);

                demo_draw(demo);
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

        event = xcb_wait_for_event(demo->connection);
        if (event) {
            demo_handle_event(demo, event);
            free(event);
        }
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

    xcb_map_window(demo->connection, demo->window);
}

static void demo_init_xgl(struct demo *demo)
{
    const XGL_APPLICATION_INFO app = {
        .sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pAppName = (const XGL_CHAR *) "cube",
        .appVersion = 0,
        .pEngineName = (const XGL_CHAR *) "cube",
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
        (const XGL_CHAR *) "XGL_WSI_X11",
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
    memset(demo, 0, sizeof(*demo));

    demo_init_connection(demo);
    demo_init_xgl(demo);

    demo->width = 1024;
    demo->height = 1024;
    demo->format.channelFormat = XGL_CH_FMT_B8G8R8A8;
    demo->format.numericFormat = XGL_NUM_FMT_UNORM;
}

static void demo_cleanup(struct demo *demo)
{
    XGL_UINT i;

    xglDestroyObject(demo->cmd);

    xglDestroyObject(demo->viewport);
    xglDestroyObject(demo->raster);
    xglDestroyObject(demo->msaa);
    xglDestroyObject(demo->color_blend);
    xglDestroyObject(demo->depth_stencil);

    xglDestroyObject(demo->pipeline);

    xglDestroyObject(demo->dset);

//    xglFreeMemory(demo->vertices.mem);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        xglDestroyObject(demo->textures[i].view);
        xglDestroyObject(demo->textures[i].image);
        xglFreeMemory(demo->textures[i].mem);
        xglDestroyObject(demo->textures[i].sampler);
    }

    xglDestroyObject(demo->depth.view);
    xglDestroyObject(demo->depth.image);
    xglFreeMemory(demo->depth.mem);

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
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
