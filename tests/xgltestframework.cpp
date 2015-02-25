//  XGL tests
//
//  Copyright (C) 2014 LunarG, Inc.
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.

#include "xgltestframework.h"
#include "xglrenderframework.h"
#include "GL/freeglut_std.h"
//#include "ShaderLang.h"
#include "GlslangToBil.h"
#include <limits.h>
#include <math.h>
#include <wand/MagickWand.h>

// Command-line options
enum TOptions {
    EOptionNone               = 0x000,
    EOptionIntermediate       = 0x001,
    EOptionSuppressInfolog    = 0x002,
    EOptionMemoryLeakMode     = 0x004,
    EOptionRelaxedErrors      = 0x008,
    EOptionGiveWarnings       = 0x010,
    EOptionLinkProgram        = 0x020,
    EOptionMultiThreaded      = 0x040,
    EOptionDumpConfig         = 0x080,
    EOptionDumpReflection     = 0x100,
    EOptionSuppressWarnings   = 0x200,
    EOptionDumpVersions       = 0x400,
    EOptionBil                = 0x800,
    EOptionDefaultDesktop     = 0x1000,
};

#ifndef _WIN32

#include <errno.h>

int fopen_s(
   FILE** pFile,
   const char* filename,
   const char* mode
)
{
   if (!pFile || !filename || !mode) {
      return EINVAL;
   }

   FILE* f = fopen(filename, mode);
   if (! f) {
      if (errno != 0) {
         return errno;
      } else {
         return ENOENT;
      }
   }
   *pFile = f;

   return 0;
}

#endif

// Set up environment for GLSL compiler
// Must be done once per process
void TestEnvironment::SetUp()
{
    // Initialize GLSL to BIL compiler utility
    glslang::InitializeProcess();

    xgl_testing::set_error_callback(test_error_callback);
}

void TestEnvironment::TearDown()
{
    glslang::FinalizeProcess();
}

XglTestFramework::XglTestFramework() :
    m_compile_options( 0 ),
    m_num_shader_strings( 0 )
{

}

XglTestFramework::~XglTestFramework()
{

}

// Define all the static elements
bool XglTestFramework::m_show_images = false;
bool XglTestFramework::m_save_images = false;
bool XglTestFramework::m_compare_images = false;
bool XglTestFramework::m_use_bil = true;
int XglTestFramework::m_width = 0;
int XglTestFramework::m_height = 0;
std::list<XglTestImageRecord> XglTestFramework::m_images;
std::list<XglTestImageRecord>::iterator XglTestFramework::m_display_image;
int m_display_image_idx = 0;

void XglTestFramework::InitArgs(int *argc, char *argv[])
{
    int i, n;

    for (i=0, n=0; i< *argc; i++) {
        if (strncmp("--show-images", argv[i], 13) == 0) {
            m_show_images = true;
            continue;
        }
        if (strncmp("--save-images", argv[i], 13) == 0) {
            m_save_images = true;
            continue;
        }

        if (strncmp("--use-BIL", argv[i], 13) == 0) {
            m_use_bil = true;
            continue;
        }

        if (strncmp("--no-BIL", argv[i], 13) == 0) {
            m_use_bil = false;
            continue;
        }

        if (strncmp("--compare-images", argv[i], 16) == 0) {
            m_compare_images = true;
            continue;
        }

        /*
         * Since the above "consume" inputs, update argv
         * so that it contains the trimmed list of args for glutInit
         */
        if (strncmp("--help", argv[i], 6) == 0 || strncmp("-h", argv[i], 2) == 0) {
            printf("\nOther options:\n");
            printf("\t--show-images\n"
                   "\t\tDisplay test images in viewer after tests complete.\n");
            printf("\t--save-images\n"
                   "\t\tSave tests images as ppm files in current working directory.\n"
                   "\t\tUsed to generate golden images for compare-images.\n");
            printf("\t--compare-images\n"
                   "\t\tCompare test images to 'golden' image in golden folder.\n"
                   "\t\tAlso saves the generated test image in current working\n"
                   "\t\t\tdirectory but only if the image is different from the golden\n"
                   "\t\tSetting RENDERTEST_GOLDEN_DIR environment variable can specify\n"
                   "\t\t\tdifferent directory for golden images\n"
                   "\t\tSignal test failure if different.\n");
            printf("\t--use-BIL\n"
                   "\t\tUse BIL code path (default).\n");
            printf("\t--no-BIL\n"
                   "\t\tUse built-in GLSL compiler rather than BIL code path.\n");
            exit(0);
        }

        argv[n] = argv[i];
        n++;
    }
}

void XglTestFramework::WritePPM( const char *basename, XglImage *image )
{
    string filename;
    XGL_RESULT err;
        int x, y;

    filename.append(basename);
    filename.append(".ppm");

    const XGL_IMAGE_SUBRESOURCE sr = {
        XGL_IMAGE_ASPECT_COLOR, 0, 0
    };
    XGL_SUBRESOURCE_LAYOUT sr_layout;
    size_t data_size = sizeof(sr_layout);

    err = xglGetImageSubresourceInfo( image->image(), &sr,
                                      XGL_INFO_TYPE_SUBRESOURCE_LAYOUT,
                                      &data_size, &sr_layout);
    ASSERT_XGL_SUCCESS( err );
    ASSERT_EQ(data_size, sizeof(sr_layout));

    const char *ptr;

    err = xglMapMemory( image->memory(), 0, (void **) &ptr );
    ASSERT_XGL_SUCCESS( err );

    ptr += sr_layout.offset;

    ofstream file (filename.c_str());
    ASSERT_TRUE(file.is_open()) << "Unable to open file: " << filename;

    file << "P6\n";
    file << image->width() << "\n";
    file << image->height() << "\n";
    file << 255 << "\n";

    for (y = 0; y < image->height(); y++) {
        const int *row = (const int *) ptr;
        int swapped;

        if (image->format() == XGL_FMT_B8G8R8A8_UNORM)
        {
            for (x = 0; x < image->width(); x++) {
                swapped = (*row & 0xff00ff00) | (*row & 0x000000ff) << 16 | (*row & 0x00ff0000) >> 16;
                file.write((char *) &swapped, 3);
                row++;
            }
        }
        else if (image->format() == XGL_FMT_R8G8B8A8_UNORM)
        {
            for (x = 0; x < image->width(); x++) {
                file.write((char *) row, 3);
                row++;
            }
        }
        else {
            printf("Unrecognized image format - will not write image files");
            break;
        }

        ptr += sr_layout.rowPitch;
    }

    file.close();

    err = xglUnmapMemory( image->memory() );
    ASSERT_XGL_SUCCESS( err );
}

void XglTestFramework::Compare(const char *basename, XglImage *image )
{

    MagickWand *magick_wand_1;
    MagickWand *magick_wand_2;
    MagickWand *compare_wand;
    MagickBooleanType status;
    char testimage[256],golden[PATH_MAX+256],golddir[PATH_MAX] = "./golden";
    double differenz;

    if (getenv("RENDERTEST_GOLDEN_DIR"))
    {
        strcpy(golddir,getenv("RENDERTEST_GOLDEN_DIR"));
    }

    MagickWandGenesis();
    magick_wand_1=NewMagickWand();
    sprintf(testimage,"%s.ppm",basename);
    status=MagickReadImage(magick_wand_1,testimage);
    ASSERT_TRUE(status) << "Unable to open file: " << testimage;


    MagickWandGenesis();
    magick_wand_2=NewMagickWand();
    sprintf(golden,"%s/%s.ppm",golddir,basename);
    status=MagickReadImage(magick_wand_2,golden);
    ASSERT_TRUE(status) << "Unable to open file: " << golden;

    compare_wand=MagickCompareImages(magick_wand_1,magick_wand_2, MeanAbsoluteErrorMetric, &differenz);
    if (differenz != 0.0)
    {
        char difference[256];

        sprintf(difference,"%s-diff.ppm",basename);
        status = MagickWriteImage(compare_wand, difference);
        ASSERT_TRUE(differenz == 0.0) << "Image comparison failed - diff file written";
    }
    DestroyMagickWand(compare_wand);

    DestroyMagickWand(magick_wand_1);
    DestroyMagickWand(magick_wand_2);
    MagickWandTerminus();

    if (differenz == 0.0)
    {
        /*
         * If test image and golden image match, we do not need to
         * keep around the test image.
         */
        remove(testimage);
    }
}

void XglTestFramework::Show(const char *comment, XglImage *image)
{
    XGL_RESULT err;

    const XGL_IMAGE_SUBRESOURCE sr = {
        XGL_IMAGE_ASPECT_COLOR, 0, 0
    };
    XGL_SUBRESOURCE_LAYOUT sr_layout;
    size_t data_size = sizeof(sr_layout);
    XglTestImageRecord record;

    if (!m_show_images) return;

    err = xglGetImageSubresourceInfo( image->image(), &sr, XGL_INFO_TYPE_SUBRESOURCE_LAYOUT,
                                      &data_size, &sr_layout);
    ASSERT_XGL_SUCCESS( err );
    ASSERT_EQ(data_size, sizeof(sr_layout));

    char *ptr;

    err = image->MapMemory( (void **) &ptr );
    ASSERT_XGL_SUCCESS( err );

    ptr += sr_layout.offset;

    record.m_title.append(comment);
    record.m_width = image->width();
    record.m_height = image->height();
    // TODO: Need to make this more robust to handle different image formats
    record.m_data_size = image->width()*image->height()*4;
    record.m_data = malloc(record.m_data_size);
    memcpy(record.m_data, ptr, record.m_data_size);
    m_images.push_back(record);
    m_display_image = --m_images.end();

    err = image->UnmapMemory();
    ASSERT_XGL_SUCCESS( err );

}

void XglTestFramework::RecordImage(XglImage *image, char *tag)
{
    const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
    ostringstream filestream;
    string filename;

    filestream << test_info->name() << "-" << tag;
    filename = filestream.str();
    // ToDo - scrub string for bad characters

    if (m_save_images || m_compare_images) {
        WritePPM(filename.c_str(), image);
        if (m_compare_images) {
            Compare(filename.c_str(), image);
        }
    }

    if (m_show_images) {
        Show(test_info->name(), image);
    }
}

void XglTestFramework::RecordImage(XglImage *image)
{
    const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
    ostringstream filestream;
    string filename;

    m_width = 40;

    if (strcmp(test_info->name(), m_testName.c_str())) {
        filestream << test_info->name();
        m_testName.assign(test_info->name());
        m_frameNum = 2;
        filename = filestream.str();
    }
    else {
        filestream << test_info->name() << "-" << m_frameNum;
        m_frameNum++;
        filename = filestream.str();
    }


    // ToDo - scrub string for bad characters

    if (m_save_images || m_compare_images) {
        WritePPM(filename.c_str(), image);
        if (m_compare_images) {
            Compare(filename.c_str(), image);
        }
    }

    if (m_show_images) {
        Show(test_info->name(), image);
    }
}

static xgl_testing::Environment *environment;

TestFrameworkXglPresent::TestFrameworkXglPresent() :
m_device(environment->default_device()),
m_queue(*m_device.graphics_queues()[0]),
m_cmdbuf(m_device, xgl_testing::CmdBuffer::create_info(XGL_QUEUE_TYPE_GRAPHICS))
{
    m_quit = false;
    m_pause = false;
    m_width = 0;
    m_height = 0;
}

void  TestFrameworkXglPresent::Display()
{
    XGL_RESULT err;

    XGL_WSI_X11_PRESENT_INFO present = {
        .destWindow = m_window,
        .srcImage = m_display_image->m_presentableImage,
    };

    xcb_change_property (environment->m_connection,
                         XCB_PROP_MODE_REPLACE,
                         m_window,
                         XCB_ATOM_WM_NAME,
                         XCB_ATOM_STRING,
                         8,
                         m_display_image->m_title.size(),
                         m_display_image->m_title.c_str());

    err = xglWsiX11QueuePresent(m_queue.obj(), &present, NULL);
    assert(!err);

    m_queue.wait();

}

void  TestFrameworkXglPresent::HandleEvent(xcb_generic_event_t *event)
{
    u_int8_t event_code = event->response_type & 0x7f;
    switch (event_code) {
    case XCB_EXPOSE:
        Display();  // TODO: handle resize
        break;
    case XCB_CLIENT_MESSAGE:
        if((*(xcb_client_message_event_t*)event).data.data32[0] ==
           (m_atom_wm_delete_window)->atom) {
            m_quit = true;
        }
        break;
    case XCB_KEY_RELEASE:
        {
            const xcb_key_release_event_t *key =
                (const xcb_key_release_event_t *) event;

            switch (key->detail) {
            case 0x9:           // Escape
                m_quit = true;
                break;
            case 0x71:          // left arrow key
                if (m_display_image == m_images.begin()) {
                    m_display_image = --m_images.end();
                } else {
                    --m_display_image;
                }
                break;
            case 0x72:          // right arrow key
                ++m_display_image;
                if (m_display_image == m_images.end()) {
                    m_display_image = m_images.begin();
                }
                break;
            case 0x41:
                 m_pause = !m_pause;
                 break;
            }
            Display();
        }
        break;
    default:
        break;
    }
}

void  TestFrameworkXglPresent::Run()
{
    xcb_flush(environment->m_connection);

    while (! m_quit) {
        xcb_generic_event_t *event;

        if (m_pause) {
            event = xcb_wait_for_event(environment->m_connection);
        } else {
            event = xcb_poll_for_event(environment->m_connection);
        }
        if (event) {
            HandleEvent(event);
            free(event);
        }
    }
}

void TestFrameworkXglPresent::CreatePresentableImages()
{
    XGL_RESULT err;

    m_display_image = m_images.begin();

    for (int x=0; x < m_images.size(); x++)
    {
        const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO presentable_image_info = {
            .format = XGL_FMT_B8G8R8A8_UNORM,
            .usage = XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .extent = {
                .width = m_display_image->m_width,
                .height = m_display_image->m_height,
            },
            .flags = 0,
        };

        void *dest_ptr;

        err = xglWsiX11CreatePresentableImage(m_device.obj(), &presentable_image_info,
                        &m_display_image->m_presentableImage, &m_display_image->m_presentableMemory);
        assert(!err);

        xgl_testing::Buffer buf;
        buf.init(m_device, (XGL_GPU_SIZE) m_display_image->m_data_size);
        dest_ptr = buf.map();
        memcpy(dest_ptr,m_display_image->m_data, m_display_image->m_data_size);
        buf.unmap();

        m_cmdbuf.begin();

        XGL_BUFFER_IMAGE_COPY region = {};
        region.imageExtent.height = m_display_image->m_height;
        region.imageExtent.width = m_display_image->m_width;
        region.imageExtent.depth = 1;

        xglCmdCopyBufferToImage(m_cmdbuf.obj(), buf.obj(), m_display_image->m_presentableImage, 1, &region);
        m_cmdbuf.end();

        uint32_t     numMemRefs=2;
        XGL_MEMORY_REF memRefs[2];
        memRefs[0].flags = 0;
        memRefs[0].mem = m_display_image->m_presentableMemory;
        memRefs[1].flags = 0;
        memRefs[1].mem = buf.memories()[0];

        XGL_CMD_BUFFER cmdBufs[1];
        cmdBufs[0] = m_cmdbuf.obj();

        xglQueueSubmit(m_queue.obj(), 1, cmdBufs, numMemRefs, memRefs, NULL);
        m_queue.wait();


        if (m_display_image->m_width > m_width)
            m_width = m_display_image->m_width;

        if (m_display_image->m_height > m_height)
            m_height = m_display_image->m_height;


        ++m_display_image;

    }

    m_display_image = m_images.begin();
}

void  TestFrameworkXglPresent::InitPresentFramework(std::list<XglTestImageRecord>  &imagesIn)
{
    m_images = imagesIn;
}

void  TestFrameworkXglPresent::CreateWindow()
{
    uint32_t value_mask, value_list[32];

    m_window = xcb_generate_id(environment->m_connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = environment->m_screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE |
                    XCB_EVENT_MASK_EXPOSURE |
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(environment->m_connection,
            XCB_COPY_FROM_PARENT,
            m_window, environment->m_screen->root,
            0, 0, m_width, m_height, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            environment->m_screen->root_visual,
            value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(environment->m_connection, 1, 12,
                                                      "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(environment->m_connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(environment->m_connection, 0, 16, "WM_DELETE_WINDOW");
    m_atom_wm_delete_window = xcb_intern_atom_reply(environment->m_connection, cookie2, 0);

    xcb_change_property(environment->m_connection, XCB_PROP_MODE_REPLACE,
                        m_window, (*reply).atom, 4, 32, 1,
                        &(*m_atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(environment->m_connection, m_window);
}

void TestFrameworkXglPresent::TearDown()
{
    xcb_destroy_window(environment->m_connection, m_window);
}

void XglTestFramework::Finish()
{
    if (m_images.size() == 0) return;

    environment = new xgl_testing::Environment();
    ::testing::AddGlobalTestEnvironment(environment);
    environment->X11SetUp();

    {
        TestFrameworkXglPresent xglPresent;

        xglPresent.InitPresentFramework(m_images);
        xglPresent.CreatePresentableImages();
        xglPresent.CreateWindow();
        xglPresent.Run();
        xglPresent.TearDown();
    }
    environment->TearDown();
}

//
// These are the default resources for TBuiltInResources, used for both
//  - parsing this string for the case where the user didn't supply one
//  - dumping out a template for user construction of a config file
//
static const char* DefaultConfig =
    "MaxLights 32\n"
    "MaxClipPlanes 6\n"
    "MaxTextureUnits 32\n"
    "MaxTextureCoords 32\n"
    "MaxVertexAttribs 64\n"
    "MaxVertexUniformComponents 4096\n"
    "MaxVaryingFloats 64\n"
    "MaxVertexTextureImageUnits 32\n"
    "MaxCombinedTextureImageUnits 80\n"
    "MaxTextureImageUnits 32\n"
    "MaxFragmentUniformComponents 4096\n"
    "MaxDrawBuffers 32\n"
    "MaxVertexUniformVectors 128\n"
    "MaxVaryingVectors 8\n"
    "MaxFragmentUniformVectors 16\n"
    "MaxVertexOutputVectors 16\n"
    "MaxFragmentInputVectors 15\n"
    "MinProgramTexelOffset -8\n"
    "MaxProgramTexelOffset 7\n"
    "MaxClipDistances 8\n"
    "MaxComputeWorkGroupCountX 65535\n"
    "MaxComputeWorkGroupCountY 65535\n"
    "MaxComputeWorkGroupCountZ 65535\n"
    "MaxComputeWorkGroupSizeX 1024\n"
    "MaxComputeWorkGroupSizeY 1024\n"
    "MaxComputeWorkGroupSizeZ 64\n"
    "MaxComputeUniformComponents 1024\n"
    "MaxComputeTextureImageUnits 16\n"
    "MaxComputeImageUniforms 8\n"
    "MaxComputeAtomicCounters 8\n"
    "MaxComputeAtomicCounterBuffers 1\n"
    "MaxVaryingComponents 60\n"
    "MaxVertexOutputComponents 64\n"
    "MaxGeometryInputComponents 64\n"
    "MaxGeometryOutputComponents 128\n"
    "MaxFragmentInputComponents 128\n"
    "MaxImageUnits 8\n"
    "MaxCombinedImageUnitsAndFragmentOutputs 8\n"
    "MaxCombinedShaderOutputResources 8\n"
    "MaxImageSamples 0\n"
    "MaxVertexImageUniforms 0\n"
    "MaxTessControlImageUniforms 0\n"
    "MaxTessEvaluationImageUniforms 0\n"
    "MaxGeometryImageUniforms 0\n"
    "MaxFragmentImageUniforms 8\n"
    "MaxCombinedImageUniforms 8\n"
    "MaxGeometryTextureImageUnits 16\n"
    "MaxGeometryOutputVertices 256\n"
    "MaxGeometryTotalOutputComponents 1024\n"
    "MaxGeometryUniformComponents 1024\n"
    "MaxGeometryVaryingComponents 64\n"
    "MaxTessControlInputComponents 128\n"
    "MaxTessControlOutputComponents 128\n"
    "MaxTessControlTextureImageUnits 16\n"
    "MaxTessControlUniformComponents 1024\n"
    "MaxTessControlTotalOutputComponents 4096\n"
    "MaxTessEvaluationInputComponents 128\n"
    "MaxTessEvaluationOutputComponents 128\n"
    "MaxTessEvaluationTextureImageUnits 16\n"
    "MaxTessEvaluationUniformComponents 1024\n"
    "MaxTessPatchComponents 120\n"
    "MaxPatchVertices 32\n"
    "MaxTessGenLevel 64\n"
    "MaxViewports 16\n"
    "MaxVertexAtomicCounters 0\n"
    "MaxTessControlAtomicCounters 0\n"
    "MaxTessEvaluationAtomicCounters 0\n"
    "MaxGeometryAtomicCounters 0\n"
    "MaxFragmentAtomicCounters 8\n"
    "MaxCombinedAtomicCounters 8\n"
    "MaxAtomicCounterBindings 1\n"
    "MaxVertexAtomicCounterBuffers 0\n"
    "MaxTessControlAtomicCounterBuffers 0\n"
    "MaxTessEvaluationAtomicCounterBuffers 0\n"
    "MaxGeometryAtomicCounterBuffers 0\n"
    "MaxFragmentAtomicCounterBuffers 1\n"
    "MaxCombinedAtomicCounterBuffers 1\n"
    "MaxAtomicCounterBufferSize 16384\n"
    "MaxTransformFeedbackBuffers 4\n"
    "MaxTransformFeedbackInterleavedComponents 64\n"
    "MaxCullDistances 8\n"
    "MaxCombinedClipAndCullDistances 8\n"
    "MaxSamples 4\n"

    "nonInductiveForLoops 1\n"
    "whileLoops 1\n"
    "doWhileLoops 1\n"
    "generalUniformIndexing 1\n"
    "generalAttributeMatrixVectorIndexing 1\n"
    "generalVaryingIndexing 1\n"
    "generalSamplerIndexing 1\n"
    "generalVariableIndexing 1\n"
    "generalConstantMatrixVectorIndexing 1\n"
    ;

//
// *.conf => this is a config file that can set limits/resources
//
bool XglTestFramework::SetConfigFile(const std::string& name)
{
    if (name.size() < 5)
        return false;

    if (name.compare(name.size() - 5, 5, ".conf") == 0) {
        ConfigFile = name;
        return true;
    }

    return false;
}

//
// Parse either a .conf file provided by the user or the default string above.
//
void XglTestFramework::ProcessConfigFile()
{
    char** configStrings = 0;
    char* config = 0;
    if (ConfigFile.size() > 0) {
        configStrings = ReadFileData(ConfigFile.c_str());
        if (configStrings)
            config = *configStrings;
        else {
            printf("Error opening configuration file; will instead use the default configuration\n");
        }
    }

    if (config == 0) {
        config = new char[strlen(DefaultConfig) + 1];
        strcpy(config, DefaultConfig);
    }

    const char* delims = " \t\n\r";
    const char* token = strtok(config, delims);
    while (token) {
        const char* valueStr = strtok(0, delims);
        if (valueStr == 0 || ! (valueStr[0] == '-' || (valueStr[0] >= '0' && valueStr[0] <= '9'))) {
            printf("Error: '%s' bad .conf file.  Each name must be followed by one number.\n", valueStr ? valueStr : "");
            return;
        }
        int value = atoi(valueStr);

        if (strcmp(token, "MaxLights") == 0)
            Resources.maxLights = value;
        else if (strcmp(token, "MaxClipPlanes") == 0)
            Resources.maxClipPlanes = value;
        else if (strcmp(token, "MaxTextureUnits") == 0)
            Resources.maxTextureUnits = value;
        else if (strcmp(token, "MaxTextureCoords") == 0)
            Resources.maxTextureCoords = value;
        else if (strcmp(token, "MaxVertexAttribs") == 0)
            Resources.maxVertexAttribs = value;
        else if (strcmp(token, "MaxVertexUniformComponents") == 0)
            Resources.maxVertexUniformComponents = value;
        else if (strcmp(token, "MaxVaryingFloats") == 0)
            Resources.maxVaryingFloats = value;
        else if (strcmp(token, "MaxVertexTextureImageUnits") == 0)
            Resources.maxVertexTextureImageUnits = value;
        else if (strcmp(token, "MaxCombinedTextureImageUnits") == 0)
            Resources.maxCombinedTextureImageUnits = value;
        else if (strcmp(token, "MaxTextureImageUnits") == 0)
            Resources.maxTextureImageUnits = value;
        else if (strcmp(token, "MaxFragmentUniformComponents") == 0)
            Resources.maxFragmentUniformComponents = value;
        else if (strcmp(token, "MaxDrawBuffers") == 0)
            Resources.maxDrawBuffers = value;
        else if (strcmp(token, "MaxVertexUniformVectors") == 0)
            Resources.maxVertexUniformVectors = value;
        else if (strcmp(token, "MaxVaryingVectors") == 0)
            Resources.maxVaryingVectors = value;
        else if (strcmp(token, "MaxFragmentUniformVectors") == 0)
            Resources.maxFragmentUniformVectors = value;
        else if (strcmp(token, "MaxVertexOutputVectors") == 0)
            Resources.maxVertexOutputVectors = value;
        else if (strcmp(token, "MaxFragmentInputVectors") == 0)
            Resources.maxFragmentInputVectors = value;
        else if (strcmp(token, "MinProgramTexelOffset") == 0)
            Resources.minProgramTexelOffset = value;
        else if (strcmp(token, "MaxProgramTexelOffset") == 0)
            Resources.maxProgramTexelOffset = value;
        else if (strcmp(token, "MaxClipDistances") == 0)
            Resources.maxClipDistances = value;
        else if (strcmp(token, "MaxComputeWorkGroupCountX") == 0)
            Resources.maxComputeWorkGroupCountX = value;
        else if (strcmp(token, "MaxComputeWorkGroupCountY") == 0)
            Resources.maxComputeWorkGroupCountY = value;
        else if (strcmp(token, "MaxComputeWorkGroupCountZ") == 0)
            Resources.maxComputeWorkGroupCountZ = value;
        else if (strcmp(token, "MaxComputeWorkGroupSizeX") == 0)
            Resources.maxComputeWorkGroupSizeX = value;
        else if (strcmp(token, "MaxComputeWorkGroupSizeY") == 0)
            Resources.maxComputeWorkGroupSizeY = value;
        else if (strcmp(token, "MaxComputeWorkGroupSizeZ") == 0)
            Resources.maxComputeWorkGroupSizeZ = value;
        else if (strcmp(token, "MaxComputeUniformComponents") == 0)
            Resources.maxComputeUniformComponents = value;
        else if (strcmp(token, "MaxComputeTextureImageUnits") == 0)
            Resources.maxComputeTextureImageUnits = value;
        else if (strcmp(token, "MaxComputeImageUniforms") == 0)
            Resources.maxComputeImageUniforms = value;
        else if (strcmp(token, "MaxComputeAtomicCounters") == 0)
            Resources.maxComputeAtomicCounters = value;
        else if (strcmp(token, "MaxComputeAtomicCounterBuffers") == 0)
            Resources.maxComputeAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxVaryingComponents") == 0)
            Resources.maxVaryingComponents = value;
        else if (strcmp(token, "MaxVertexOutputComponents") == 0)
            Resources.maxVertexOutputComponents = value;
        else if (strcmp(token, "MaxGeometryInputComponents") == 0)
            Resources.maxGeometryInputComponents = value;
        else if (strcmp(token, "MaxGeometryOutputComponents") == 0)
            Resources.maxGeometryOutputComponents = value;
        else if (strcmp(token, "MaxFragmentInputComponents") == 0)
            Resources.maxFragmentInputComponents = value;
        else if (strcmp(token, "MaxImageUnits") == 0)
            Resources.maxImageUnits = value;
        else if (strcmp(token, "MaxCombinedImageUnitsAndFragmentOutputs") == 0)
            Resources.maxCombinedImageUnitsAndFragmentOutputs = value;
        else if (strcmp(token, "MaxCombinedShaderOutputResources") == 0)
            Resources.maxCombinedShaderOutputResources = value;
        else if (strcmp(token, "MaxImageSamples") == 0)
            Resources.maxImageSamples = value;
        else if (strcmp(token, "MaxVertexImageUniforms") == 0)
            Resources.maxVertexImageUniforms = value;
        else if (strcmp(token, "MaxTessControlImageUniforms") == 0)
            Resources.maxTessControlImageUniforms = value;
        else if (strcmp(token, "MaxTessEvaluationImageUniforms") == 0)
            Resources.maxTessEvaluationImageUniforms = value;
        else if (strcmp(token, "MaxGeometryImageUniforms") == 0)
            Resources.maxGeometryImageUniforms = value;
        else if (strcmp(token, "MaxFragmentImageUniforms") == 0)
            Resources.maxFragmentImageUniforms = value;
        else if (strcmp(token, "MaxCombinedImageUniforms") == 0)
            Resources.maxCombinedImageUniforms = value;
        else if (strcmp(token, "MaxGeometryTextureImageUnits") == 0)
            Resources.maxGeometryTextureImageUnits = value;
        else if (strcmp(token, "MaxGeometryOutputVertices") == 0)
            Resources.maxGeometryOutputVertices = value;
        else if (strcmp(token, "MaxGeometryTotalOutputComponents") == 0)
            Resources.maxGeometryTotalOutputComponents = value;
        else if (strcmp(token, "MaxGeometryUniformComponents") == 0)
            Resources.maxGeometryUniformComponents = value;
        else if (strcmp(token, "MaxGeometryVaryingComponents") == 0)
            Resources.maxGeometryVaryingComponents = value;
        else if (strcmp(token, "MaxTessControlInputComponents") == 0)
            Resources.maxTessControlInputComponents = value;
        else if (strcmp(token, "MaxTessControlOutputComponents") == 0)
            Resources.maxTessControlOutputComponents = value;
        else if (strcmp(token, "MaxTessControlTextureImageUnits") == 0)
            Resources.maxTessControlTextureImageUnits = value;
        else if (strcmp(token, "MaxTessControlUniformComponents") == 0)
            Resources.maxTessControlUniformComponents = value;
        else if (strcmp(token, "MaxTessControlTotalOutputComponents") == 0)
            Resources.maxTessControlTotalOutputComponents = value;
        else if (strcmp(token, "MaxTessEvaluationInputComponents") == 0)
            Resources.maxTessEvaluationInputComponents = value;
        else if (strcmp(token, "MaxTessEvaluationOutputComponents") == 0)
            Resources.maxTessEvaluationOutputComponents = value;
        else if (strcmp(token, "MaxTessEvaluationTextureImageUnits") == 0)
            Resources.maxTessEvaluationTextureImageUnits = value;
        else if (strcmp(token, "MaxTessEvaluationUniformComponents") == 0)
            Resources.maxTessEvaluationUniformComponents = value;
        else if (strcmp(token, "MaxTessPatchComponents") == 0)
            Resources.maxTessPatchComponents = value;
        else if (strcmp(token, "MaxPatchVertices") == 0)
            Resources.maxPatchVertices = value;
        else if (strcmp(token, "MaxTessGenLevel") == 0)
            Resources.maxTessGenLevel = value;
        else if (strcmp(token, "MaxViewports") == 0)
            Resources.maxViewports = value;
        else if (strcmp(token, "MaxVertexAtomicCounters") == 0)
            Resources.maxVertexAtomicCounters = value;
        else if (strcmp(token, "MaxTessControlAtomicCounters") == 0)
            Resources.maxTessControlAtomicCounters = value;
        else if (strcmp(token, "MaxTessEvaluationAtomicCounters") == 0)
            Resources.maxTessEvaluationAtomicCounters = value;
        else if (strcmp(token, "MaxGeometryAtomicCounters") == 0)
            Resources.maxGeometryAtomicCounters = value;
        else if (strcmp(token, "MaxFragmentAtomicCounters") == 0)
            Resources.maxFragmentAtomicCounters = value;
        else if (strcmp(token, "MaxCombinedAtomicCounters") == 0)
            Resources.maxCombinedAtomicCounters = value;
        else if (strcmp(token, "MaxAtomicCounterBindings") == 0)
            Resources.maxAtomicCounterBindings = value;
        else if (strcmp(token, "MaxVertexAtomicCounterBuffers") == 0)
            Resources.maxVertexAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxTessControlAtomicCounterBuffers") == 0)
            Resources.maxTessControlAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxTessEvaluationAtomicCounterBuffers") == 0)
            Resources.maxTessEvaluationAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxGeometryAtomicCounterBuffers") == 0)
            Resources.maxGeometryAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxFragmentAtomicCounterBuffers") == 0)
            Resources.maxFragmentAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxCombinedAtomicCounterBuffers") == 0)
            Resources.maxCombinedAtomicCounterBuffers = value;
        else if (strcmp(token, "MaxAtomicCounterBufferSize") == 0)
            Resources.maxAtomicCounterBufferSize = value;
        else if (strcmp(token, "MaxTransformFeedbackBuffers") == 0)
            Resources.maxTransformFeedbackBuffers = value;
        else if (strcmp(token, "MaxTransformFeedbackInterleavedComponents") == 0)
            Resources.maxTransformFeedbackInterleavedComponents = value;
        else if (strcmp(token, "MaxCullDistances") == 0)
            Resources.maxCullDistances = value;
        else if (strcmp(token, "MaxCombinedClipAndCullDistances") == 0)
            Resources.maxCombinedClipAndCullDistances = value;
        else if (strcmp(token, "MaxSamples") == 0)
            Resources.maxSamples = value;

        else if (strcmp(token, "nonInductiveForLoops") == 0)
            Resources.limits.nonInductiveForLoops = (value != 0);
        else if (strcmp(token, "whileLoops") == 0)
            Resources.limits.whileLoops = (value != 0);
        else if (strcmp(token, "doWhileLoops") == 0)
            Resources.limits.doWhileLoops = (value != 0);
        else if (strcmp(token, "generalUniformIndexing") == 0)
            Resources.limits.generalUniformIndexing = (value != 0);
        else if (strcmp(token, "generalAttributeMatrixVectorIndexing") == 0)
            Resources.limits.generalAttributeMatrixVectorIndexing = (value != 0);
        else if (strcmp(token, "generalVaryingIndexing") == 0)
            Resources.limits.generalVaryingIndexing = (value != 0);
        else if (strcmp(token, "generalSamplerIndexing") == 0)
            Resources.limits.generalSamplerIndexing = (value != 0);
        else if (strcmp(token, "generalVariableIndexing") == 0)
            Resources.limits.generalVariableIndexing = (value != 0);
        else if (strcmp(token, "generalConstantMatrixVectorIndexing") == 0)
            Resources.limits.generalConstantMatrixVectorIndexing = (value != 0);
        else
            printf("Warning: unrecognized limit (%s) in configuration file.\n", token);

        token = strtok(0, delims);
    }
    if (configStrings)
        FreeFileData(configStrings);
}

void XglTestFramework::SetMessageOptions(EShMessages& messages)
{
    if (m_compile_options & EOptionRelaxedErrors)
        messages = (EShMessages)(messages | EShMsgRelaxedErrors);
    if (m_compile_options & EOptionIntermediate)
        messages = (EShMessages)(messages | EShMsgAST);
    if (m_compile_options & EOptionSuppressWarnings)
        messages = (EShMessages)(messages | EShMsgSuppressWarnings);
}

//
//   Malloc a string of sufficient size and read a string into it.
//
char** XglTestFramework::ReadFileData(const char* fileName)
{
    FILE *in;
    #if defined(_WIN32) && defined(__GNUC__)
        in = fopen(fileName, "r");
        int errorCode = in ? 0 : 1;
    #else
        int errorCode = fopen_s(&in, fileName, "r");
    #endif

    char *fdata;
    int count = 0;
    const int maxSourceStrings = 5;
    char** return_data = (char**)malloc(sizeof(char *) * (maxSourceStrings+1));

    if (errorCode) {
        printf("Error: unable to open input file: %s\n", fileName);
        return 0;
    }

    while (fgetc(in) != EOF)
        count++;

    fseek(in, 0, SEEK_SET);

    if (!(fdata = (char*)malloc(count+2))) {
        printf("Error allocating memory\n");
        return 0;
    }
    if (fread(fdata,1,count, in)!=count) {
            printf("Error reading input file: %s\n", fileName);
            return 0;
    }
    fdata[count] = '\0';
    fclose(in);
    if (count == 0) {
        return_data[0]=(char*)malloc(count+2);
        return_data[0][0]='\0';
        m_num_shader_strings = 0;
        return return_data;
    } else
        m_num_shader_strings = 1;

    int len = (int)(ceil)((float)count/(float)m_num_shader_strings);
    int ptr_len=0,i=0;
    while(count>0){
        return_data[i]=(char*)malloc(len+2);
        memcpy(return_data[i],fdata+ptr_len,len);
        return_data[i][len]='\0';
        count-=(len);
        ptr_len+=(len);
        if(count<len){
            if(count==0){
               m_num_shader_strings=(i+1);
               break;
            }
           len = count;
        }
        ++i;
    }
    return return_data;
}

void XglTestFramework::FreeFileData(char** data)
{
    for(int i=0;i<m_num_shader_strings;i++)
        free(data[i]);
}

//
//   Deduce the language from the filename.  Files must end in one of the
//   following extensions:
//
//   .vert = vertex
//   .tesc = tessellation control
//   .tese = tessellation evaluation
//   .geom = geometry
//   .frag = fragment
//   .comp = compute
//
EShLanguage XglTestFramework::FindLanguage(const std::string& name)
{
    size_t ext = name.rfind('.');
    if (ext == std::string::npos) {
        return EShLangVertex;
    }

    std::string suffix = name.substr(ext + 1, std::string::npos);
    if (suffix == "vert")
        return EShLangVertex;
    else if (suffix == "tesc")
        return EShLangTessControl;
    else if (suffix == "tese")
        return EShLangTessEvaluation;
    else if (suffix == "geom")
        return EShLangGeometry;
    else if (suffix == "frag")
        return EShLangFragment;
    else if (suffix == "comp")
        return EShLangCompute;

    return EShLangVertex;
}

//
// Convert XGL shader type to compiler's
//
EShLanguage XglTestFramework::FindLanguage(const XGL_PIPELINE_SHADER_STAGE shader_type)
{
    switch (shader_type) {
    case XGL_SHADER_STAGE_VERTEX:
        return EShLangVertex;

    case XGL_SHADER_STAGE_TESS_CONTROL:
        return EShLangTessControl;

    case XGL_SHADER_STAGE_TESS_EVALUATION:
        return EShLangTessEvaluation;

    case XGL_SHADER_STAGE_GEOMETRY:
        return EShLangGeometry;

    case XGL_SHADER_STAGE_FRAGMENT:
        return EShLangFragment;

    case XGL_SHADER_STAGE_COMPUTE:
        return EShLangCompute;

    default:
        return EShLangVertex;
    }
}


//
// Compile a given string containing GLSL into BIL for use by XGL
// Return value of false means an error was encountered.
//
bool XglTestFramework::GLSLtoBIL(const XGL_PIPELINE_SHADER_STAGE shader_type,
                                 const char *pshader,
                                 std::vector<unsigned int> &bil)
{
    glslang::TProgram& program = *new glslang::TProgram;
    const char *shaderStrings[1];

    // TODO: Do we want to load a special config file depending on the
    // shader source? Optional name maybe?
    //    SetConfigFile(fileName);

    ProcessConfigFile();

    EShMessages messages = EShMsgDefault;
    SetMessageOptions(messages);

    EShLanguage stage = FindLanguage(shader_type);
    glslang::TShader* shader = new glslang::TShader(stage);

    shaderStrings[0] = pshader;
    shader->setStrings(shaderStrings, 1);

    if (! shader->parse(&Resources, (m_compile_options & EOptionDefaultDesktop) ? 110 : 100, false, messages)) {

        if (! (m_compile_options & EOptionSuppressInfolog)) {
            puts(shader->getInfoLog());
            puts(shader->getInfoDebugLog());
        }

        return false; // something didn't work
    }

    program.addShader(shader);


    //
    // Program-level processing...
    //

    if (! program.link(messages)) {

        if (! (m_compile_options & EOptionSuppressInfolog)) {
            puts(shader->getInfoLog());
            puts(shader->getInfoDebugLog());
        }

        return false;
    }

    if (m_compile_options & EOptionDumpReflection) {
        program.buildReflection();
        program.dumpReflection();
    }

    glslang::GlslangToBil(*program.getIntermediate(stage), bil);

    return true;
}



XglTestImageRecord::XglTestImageRecord() : // Constructor
    m_width( 0 ),
    m_height( 0 ),
    m_data( NULL ),
    m_presentableImage( NULL ),
    m_presentableMemory( NULL),
    m_data_size( 0 )
{
}

XglTestImageRecord::~XglTestImageRecord()
{

}

XglTestImageRecord::XglTestImageRecord(const XglTestImageRecord &copyin)   // Copy constructor to handle pass by value.
{
    m_title = copyin.m_title;
    m_width = copyin.m_width;
    m_height = copyin.m_height;
    m_data_size = copyin.m_data_size;
    m_data = copyin.m_data; // TODO: Do we need to copy the data or is pointer okay?
    m_presentableImage = copyin.m_presentableImage;
    m_presentableMemory = copyin.m_presentableMemory;
}

ostream &operator<<(ostream &output, const XglTestImageRecord &XglTestImageRecord)
{
    output << XglTestImageRecord.m_title << " (" << XglTestImageRecord.m_width <<
              "," << XglTestImageRecord.m_height << ")" << endl;
    return output;
}

XglTestImageRecord& XglTestImageRecord::operator=(const XglTestImageRecord &rhs)
{
    m_title = rhs.m_title;
    m_width = rhs.m_width;
    m_height = rhs.m_height;
    m_data_size = rhs.m_data_size;
    m_data = rhs.m_data;
    m_presentableImage = rhs.m_presentableImage;
    m_presentableMemory = rhs.m_presentableMemory;
    return *this;
}

int XglTestImageRecord::operator==(const XglTestImageRecord &rhs) const
{
    if( this->m_data != rhs.m_data) return 0;
    return 1;
}

// This function is required for built-in STL list functions like sort
int XglTestImageRecord::operator<(const XglTestImageRecord &rhs) const
{
    if( this->m_data_size < rhs.m_data_size ) return 1;
    return 0;
}

