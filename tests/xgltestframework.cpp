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
#include "GL/freeglut_std.h"
//#include "ShaderLang.h"
#include "GlslangToBil.h"
#include <math.h>

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
bool XglTestFramework::m_use_bil = true;
int XglTestFramework::m_width = 0;
int XglTestFramework::m_height = 0;
int XglTestFramework::m_window = 0;
bool XglTestFramework::m_glut_initialized = false;
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

        /*
         * Since the above "consume" inputs, update argv
         * so that it contains the trimmed list of args for glutInit
         */
        argv[n] = argv[i];
        n++;
    }

    if (m_show_images) {
        glutInit(argc, argv);
    }
}

void XglTestFramework::Reshape( int w, int h )
{
    if (!m_show_images) return;  // Do nothing except save info if not enabled

    // Resize window to be large enough to handle biggest image we've seen
    // TODO: Probably need some sort of limits for the Window system.
    if (w > m_width) {
        m_width = w;
    }
    if (h > m_height) {
        m_height = h;
    }

    glutReshapeWindow(m_width, m_height);

    glViewport( 0, 0, m_width, m_height );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( 0.0, m_width, 0.0, m_height, 0.0, 2.0 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

//    glScissor(width/4, height/4, width/2, height/2);
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
    XGL_UINT data_size = sizeof(sr_layout);

    err = xglGetImageSubresourceInfo( image->image(), &sr,
                                      XGL_INFO_TYPE_SUBRESOURCE_LAYOUT,
                                      &data_size, &sr_layout);
    ASSERT_XGL_SUCCESS( err );
    ASSERT_EQ(data_size, sizeof(sr_layout));

    const char *ptr;

    err = xglMapMemory( image->memory(), 0, (XGL_VOID **) &ptr );
    ASSERT_XGL_SUCCESS( err );

    ptr += sr_layout.offset;

    ofstream file (filename.c_str());
    ASSERT_TRUE(file.is_open()) << "Unable to open file: " << filename;

    file << "P6\n";
    file << image->width() << "\n";
    file << image->height() << "\n";
    file << 255 << "\n";

    for (y = 0; y < image->height(); y++) {
        const char *row = ptr;

        for (x = 0; x < image->width(); x++) {
            file.write(row, 3);
            row += 4;
        }

        ptr += sr_layout.rowPitch;
    }

    file.close();

    err = xglUnmapMemory( image->memory() );
    ASSERT_XGL_SUCCESS( err );
}

void XglTestFramework::InitGLUT(int w, int h)
{

    if (!m_show_images) return;

    if (!m_glut_initialized) {
        glutInitWindowSize(w, h);

        glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
        m_window = glutCreateWindow(NULL);
        m_glut_initialized = true;
    }

    Reshape(w, h);
}

void XglTestFramework::Show(const char *comment, XglImage *image)
{
    XGL_RESULT err;

    const XGL_IMAGE_SUBRESOURCE sr = {
        XGL_IMAGE_ASPECT_COLOR, 0, 0
    };
    XGL_SUBRESOURCE_LAYOUT sr_layout;
    XGL_UINT data_size = sizeof(sr_layout);

    if (!m_show_images) return;

    InitGLUT(image->width(), image->height());

    err = xglGetImageSubresourceInfo( image->image(), &sr, XGL_INFO_TYPE_SUBRESOURCE_LAYOUT,
                                      &data_size, &sr_layout);
    ASSERT_XGL_SUCCESS( err );
    ASSERT_EQ(data_size, sizeof(sr_layout));

    const char *ptr;

    err = image->MapMemory( (XGL_VOID **) &ptr );
    ASSERT_XGL_SUCCESS( err );

    ptr += sr_layout.offset;

    XglTestImageRecord record;
    record.m_title.append(comment);
    record.m_width = image->width();
    record.m_height = image->height();
    // TODO: Need to make this more robust to handle different image formats
    record.m_data_size = image->width()*image->height()*4;
    record.m_data = malloc(record.m_data_size);
    memcpy(record.m_data, ptr, record.m_data_size);
    m_images.push_back(record);
    m_display_image = --m_images.end();

//    Display();
    glutPostRedisplay();

    err = image->UnmapMemory();
    ASSERT_XGL_SUCCESS( err );
}

void XglTestFramework::RecordImage(XglImage *image)
{
    const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();

    if (m_save_images) {
        WritePPM(test_info->name(), image);
    }

    if (m_show_images) {
        Show(test_info->name(), image);
    }
}

void XglTestFramework::Display()
{
    glutSetWindowTitle(m_display_image->m_title.c_str());

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glRasterPos3f(0, 0, 0);
    glBitmap(0, 0, 0, 0, 0, 0, NULL);
    glDrawPixels(m_display_image->m_width, m_display_image->m_height,
                 GL_RGBA, GL_UNSIGNED_BYTE, m_display_image->m_data);

    glutSwapBuffers();
}

void XglTestFramework::Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         glutDestroyWindow(m_window);
         exit(0);
         break;
   }
   glutPostRedisplay();
}

void XglTestFramework::SpecialKey( int key, int x, int y )
{
    (void) x;
    (void) y;
    switch (key) {
    case GLUT_KEY_UP:
    case GLUT_KEY_RIGHT:
        ++m_display_image;
        if (m_display_image == m_images.end()) {
            m_display_image = m_images.begin();
        }
        break;
    case GLUT_KEY_DOWN:
    case GLUT_KEY_LEFT:
        if (m_display_image == m_images.begin()) {
            m_display_image = --m_images.end();
        } else {
            --m_display_image;
        }

        break;
    }
    glutPostRedisplay();
}

void XglTestFramework::Finish()
{
    if (m_images.size() == 0) return;

    glutReshapeFunc( Reshape );
    glutKeyboardFunc( Key );
    glutSpecialFunc( SpecialKey );
    glutDisplayFunc( Display );
    glutIdleFunc(NULL);

    glutMainLoop();
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
    }

    return EShLangVertex;
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
    m_data( NULL )
{
    m_data_size = 0;
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

