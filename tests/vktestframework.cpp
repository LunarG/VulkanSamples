//
//  Copyright (C) 2015 Valve Corporation
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
//
// Author: Chia-I Wu <olv@lunarg.com>
// Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
// Author: Tony Barbour <tony@LunarG.com>

#include "vktestframework.h"
#include "vkrenderframework.h"
//TODO FIXME remove this once glslang doesn't define this
#undef BadValue
#include "SPIRV/GlslangToSpv.h"
#include "SPIRV/SPVRemapper.h"
#include <limits.h>
#include <math.h>
#include <wand/MagickWand.h>


#if defined(PATH_MAX) && !defined(MAX_PATH)
#define MAX_PATH PATH_MAX
#endif

#ifdef _WIN32
#define ERR_EXIT(err_msg, err_class)                    \
    do {                                                \
        MessageBox(NULL, err_msg, err_class, MB_OK);    \
        exit(1);                                        \
   } while (0)
#else  // _WIN32

#define ERR_EXIT(err_msg, err_class)                    \
    do {                                                \
        printf(err_msg);                                \
        fflush(stdout);                                 \
        exit(1);                                        \
   } while (0)
#endif // _WIN32

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                        \
{                                                                       \
    m_fp##entrypoint = (PFN_vk##entrypoint) vkGetInstanceProcAddr(inst, "vk"#entrypoint); \
    if (m_fp##entrypoint == NULL) {                                 \
        ERR_EXIT("vkGetInstanceProcAddr failed to find vk"#entrypoint,  \
                 "vkGetInstanceProcAddr Failure");                      \
    }                                                                   \
}

#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                           \
{                                                                       \
    m_fp##entrypoint = (PFN_vk##entrypoint) vkGetDeviceProcAddr(dev, "vk"#entrypoint);   \
    if (m_fp##entrypoint == NULL) {                                 \
        ERR_EXIT("vkGetDeviceProcAddr failed to find vk"#entrypoint,    \
                 "vkGetDeviceProcAddr Failure");                        \
    }                                                                   \
}

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
    EOptionSpv                = 0x800,
    EOptionDefaultDesktop     = 0x1000,
};

typedef struct _SwapchainBuffers {
    VkImage image;
    VkCommandBuffer cmd;
    VkImageView view;
} SwapchainBuffers;

class TestFrameworkVkPresent
{
public:
    TestFrameworkVkPresent(vk_testing::Device &device);

    void Run();
    void InitPresentFramework(std::list<VkTestImageRecord> &imagesIn, VkInstance inst);
    VkFormat GetPresentFormat();
    void DestroyMyWindow();
    void CreateSwapchain();
    void SetImageLayout(VkImage image, VkImageAspectFlags aspectMask,
                        VkImageLayout old_image_layout, VkImageLayout new_image_layout);
    void TearDown();
#ifdef _WIN32
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void CreateMyWindow(bool register_class = true);
#else
    void CreateMyWindow();
#endif


protected:
    vk_testing::Device                    &m_device;
    vk_testing::Queue                     &m_queue;
    vk_testing::CommandPool                    m_cmdpool;
    vk_testing::CommandBuffer                  m_cmdbuf;

private:
    VkInstance                             m_instance;
#ifdef _WIN32
    HINSTANCE                              m_connection;        // hInstance - Windows Instance
    HWND                                   m_window;          // hWnd - window handle

#else
    xcb_connection_t                       *m_connection;
    xcb_screen_t                           *m_screen;
    xcb_window_t                            m_window;
    xcb_intern_atom_reply_t                *m_atom_wm_delete_window;
#endif
    VkSurfaceKHR                            m_surface;
    std::list<VkTestImageRecord>            m_images;
    uint32_t                                m_present_queue_node_index;

    PFN_vkGetPhysicalDeviceSurfaceSupportKHR m_fpGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR m_fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR m_fpGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR m_fpGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkCreateSwapchainKHR                m_fpCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR               m_fpDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR             m_fpGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR               m_fpAcquireNextImageKHR;
    PFN_vkQueuePresentKHR                   m_fpQueuePresentKHR;
    PFN_vkDestroySurfaceKHR                 m_fpDestroySurfaceKHR;
    uint32_t                                m_swapchainImageCount;
    VkSwapchainKHR                          m_swap_chain;
    SwapchainBuffers                       *m_buffers;
    VkFormat                                m_format;
    VkColorSpaceKHR                         m_color_space;

    uint32_t                                m_current_buffer;

    bool                                    m_quit;
    bool                                    m_pause;

    int                                     m_width;
    int                                     m_height;

    std::list<VkTestImageRecord>::iterator m_display_image;

    void Display();
#ifndef _WIN32
    void HandleEvent(xcb_generic_event_t *event);
#endif
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
    // Initialize GLSL to SPV compiler utility
    glslang::InitializeProcess();

    vk_testing::set_error_callback(test_error_callback);
}

void TestEnvironment::TearDown()
{
    glslang::FinalizeProcess();
}

VkTestFramework::VkTestFramework() :
    m_compile_options( 0 ),
    m_num_shader_strings( 0 )
{

}

VkTestFramework::~VkTestFramework()
{

}

// Define all the static elements
bool VkTestFramework::m_show_images       = false;
bool VkTestFramework::m_save_images       = false;
bool VkTestFramework::m_compare_images    = false;
bool VkTestFramework::m_use_glsl          = false;
bool VkTestFramework::m_canonicalize_spv  = false;
bool VkTestFramework::m_strip_spv         = false;
bool VkTestFramework::m_do_everything_spv = false;
int VkTestFramework::m_width = 0;
int VkTestFramework::m_height = 0;
std::list<VkTestImageRecord> VkTestFramework::m_images;
std::list<VkTestImageRecord>::iterator VkTestFramework::m_display_image;
int m_display_image_idx = 0;

bool VkTestFramework::optionMatch(const char* option, char* optionLine)
{
    if (strncmp(option, optionLine, strlen(option)) == 0)
        return true;
    else
        return false;
}

void VkTestFramework::InitArgs(int *argc, char *argv[])
{
    int i, n;

    for (i=1, n=1; i< *argc; i++) {
        if (optionMatch("--show-images", argv[i]))
            m_show_images = true;
        else if (optionMatch("--save-images", argv[i]))
            m_save_images = true;
        else if (optionMatch("--no-SPV", argv[i]))
            m_use_glsl = true;
        else if (optionMatch("--strip-SPV", argv[i]))
             m_strip_spv = true;
        else if (optionMatch("--canonicalize-SPV", argv[i]))
            m_canonicalize_spv = true;
        else if (optionMatch("--compare-images", argv[i]))
            m_compare_images = true;

        else if (optionMatch("--help", argv[i]) ||
                 optionMatch("-h", argv[i])) {
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
            printf("\t--no-SPV\n"
                   "\t\tUse built-in GLSL compiler rather than SPV code path.\n");
            printf("\t--strip-SPV\n"
                   "\t\tStrip SPIR-V debug information (line numbers, names, etc).\n");
            printf("\t--canonicalize-SPV\n"
                   "\t\tRemap SPIR-V ids before submission to aid compression.\n");
            exit(0);
        } else {
            printf("\nUnrecognized option: %s\n", argv[i]);
            printf("\nUse --help or -h for option list.\n");
            exit(0);
        }

        /*
         * Since the above "consume" inputs, update argv
         * so that it contains the trimmed list of args for glutInit
         */

        argv[n] = argv[i];
        n++;
    }
}

VkFormat VkTestFramework::GetFormat(VkInstance instance, vk_testing::Device *device)
{
    VkFormatProperties format_props;
    if (!m_show_images)
    {
        vkGetPhysicalDeviceFormatProperties(device->phy().handle(), VK_FORMAT_B8G8R8A8_UNORM, &format_props);
        if (format_props.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT ||
            format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
        {
            return VK_FORMAT_B8G8R8A8_UNORM;
        }
        vkGetPhysicalDeviceFormatProperties(device->phy().handle(), VK_FORMAT_R8G8B8A8_UNORM, &format_props);
        if (format_props.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT ||
            format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
        {
            return VK_FORMAT_R8G8B8A8_UNORM;
        }
        printf("Error - device does not support VK_FORMAT_B8G8R8A8_UNORM nor VK_FORMAT_R8G8B8A8_UNORM - exiting\n");
        exit(0);
    }
    else
    {
        /* To find which formats are presentable, you have to provide a surface to vkGetPhysicalDeviceSurfaceFormatsKHR */
        /* To create a surface, you need a window.  Use the present object to create a window, use that to create a     */
        /* KHR surface, and then find out what formats are presentable                                                  */
        VkFormat presentFormat;
        std::list<VkTestImageRecord> list;
        VkTestImageRecord placeholder;
        /* Use a dummy image record with non-zero area so the window will create on Windows */
        placeholder.m_width = placeholder.m_height = 20;
        list.push_back(placeholder);
        TestFrameworkVkPresent vkPresent(*device);
        vkPresent.InitPresentFramework(list, instance);
        vkPresent.CreateMyWindow();
        presentFormat = vkPresent.GetPresentFormat();
        vkPresent.DestroyMyWindow();
        return presentFormat;
    }
}

void VkTestFramework::WritePPM( const char *basename, VkImageObj *image )
{
    string filename;
    uint32_t x, y;
    VkImageObj displayImage(image->device());
    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    displayImage.init(image->extent().width, image->extent().height, image->format(), VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TILING_LINEAR, reqs);
    displayImage.CopyImage(*image);

    filename.append(basename);
    filename.append(".ppm");
    
    const VkImageSubresource sr = {
        VK_IMAGE_ASPECT_COLOR_BIT, 0, 0
    };
    VkSubresourceLayout sr_layout;
   
    vkGetImageSubresourceLayout(image->device()->device(), displayImage.image(), &sr, &sr_layout);

    char *ptr;
    ptr = (char *) displayImage.MapMemory();
    ptr += sr_layout.offset;
    ofstream file (filename.c_str(), ios::binary);
    ASSERT_TRUE(file.is_open()) << "Unable to open file: " << filename;

    file << "P6\n";
    file << displayImage.width() << " ";
    file << displayImage.height() << "\n";
    file << 255 << "\n";

    for (y = 0; y < displayImage.height(); y++) {
        const int *row = (const int *) ptr;
        int swapped;

        if (displayImage.format() == VK_FORMAT_B8G8R8A8_UNORM)
        {
            for (x = 0; x < displayImage.width(); x++) {
                swapped = (*row & 0xff00ff00) | (*row & 0x000000ff) << 16 | (*row & 0x00ff0000) >> 16;
                file.write((char *) &swapped, 3);
                row++;
            }
        }
        else if (displayImage.format() == VK_FORMAT_R8G8B8A8_UNORM)
        {
            for (x = 0; x < displayImage.width(); x++) {
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
    displayImage.UnmapMemory();
}

void VkTestFramework::Compare(const char *basename, VkImageObj *image )
{

    MagickWand *magick_wand_1;
    MagickWand *magick_wand_2;
    MagickWand *compare_wand;
    MagickBooleanType status;
    char testimage[256],golden[MAX_PATH+256],golddir[MAX_PATH] = "./golden";
    double differenz;

    if (getenv("RENDERTEST_GOLDEN_DIR"))
    {
        strcpy(golddir,getenv("RENDERTEST_GOLDEN_DIR"));
    }

    MagickWandGenesis();
    magick_wand_1=NewMagickWand();
    sprintf(testimage,"%s.ppm",basename);
    status=MagickReadImage(magick_wand_1,testimage);
    ASSERT_EQ(status, MagickTrue) << "Unable to open file: " << testimage;


    MagickWandGenesis();
    magick_wand_2=NewMagickWand();
    sprintf(golden,"%s/%s.ppm",golddir,basename);
    status=MagickReadImage(magick_wand_2,golden);
    ASSERT_EQ(status, MagickTrue) << "Unable to open file: " << golden;

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

void VkTestFramework::Show(const char *comment, VkImageObj *image)
{
    VkSubresourceLayout sr_layout;
    char *ptr;
    VkTestImageRecord record;
    VkImageObj displayImage(image->device());
    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    displayImage.init(image->extent().width, image->extent().height, image->format(), VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TILING_LINEAR, reqs);

    displayImage.CopyImage(*image);

    const VkImageSubresource sr = {
        VK_IMAGE_ASPECT_COLOR_BIT, 0, 0
    };

    vkGetImageSubresourceLayout(displayImage.device()->device(), displayImage.image(), &sr, &sr_layout);

    ptr = (char *) displayImage.MapMemory();
    ptr += sr_layout.offset;

    record.m_title.append(comment);
    record.m_width = displayImage.width();
    record.m_height = displayImage.height();
    // TODO: Need to make this more robust to handle different image formats
    record.m_data_size = displayImage.width() * displayImage.height() * 4;
    record.m_data = malloc(record.m_data_size);
    memcpy(record.m_data, ptr, record.m_data_size);
    m_images.push_back(record);
    m_display_image = --m_images.end();

    displayImage.UnmapMemory();
}

void VkTestFramework::RecordImages(vector<VkImageObj *> images)
{
    for (size_t i = 0; i < images.size(); i++) {
        RecordImage(images[i]);
    }
}

void VkTestFramework::RecordImage(VkImageObj * image)
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

TestFrameworkVkPresent::TestFrameworkVkPresent(vk_testing::Device &device) :
   m_device(device),
   m_queue(*m_device.graphics_queues()[0]),
   m_cmdpool(m_device, vk_testing::CommandPool::create_info(m_device.graphics_queue_node_index_)),
   m_cmdbuf(m_device, vk_testing::CommandBuffer::create_info(m_cmdpool.handle()))
{
    m_quit = false;
    m_pause = false;
    m_width = 0;
    m_height = 0;
}

VkFormat TestFrameworkVkPresent::GetPresentFormat()
{
    uint32_t formatCount;
    VkResult U_ASSERT_ONLY res;
    VkSurfaceKHR surface;
    VkFormat returnFormat;

#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.hinstance = m_connection;
    createInfo.hwnd = m_window;

    res = vkCreateWin32SurfaceKHR(m_instance, &createInfo, NULL, &surface);
#else  // _WIN32
    VkXcbSurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.connection = m_connection;
    createInfo.window = m_window;

    res = vkCreateXcbSurfaceKHR(m_instance, &createInfo, NULL, &surface);
#endif // _WIN32
    assert(res == VK_SUCCESS);

    m_fpGetPhysicalDeviceSurfaceFormatsKHR(
                                    m_device.phy().handle(),
                                    surface,
                                    &formatCount, NULL);
    VkSurfaceFormatKHR *surfFormats =
        (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    m_fpGetPhysicalDeviceSurfaceFormatsKHR(
                                    m_device.phy().handle(),
                                    surface,
                                    &formatCount, surfFormats);

    m_fpDestroySurfaceKHR(m_instance, surface, NULL);

    returnFormat = surfFormats[0].format;
    free(surfFormats);
    return returnFormat;
}

void  TestFrameworkVkPresent::Display()
{
    VkResult U_ASSERT_ONLY err;
    vk_testing::Buffer buf;
    void *dest_ptr;

    VkSemaphore presentCompleteSemaphore;
    VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo = {};
    presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    presentCompleteSemaphoreCreateInfo.pNext = NULL;
    presentCompleteSemaphoreCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


    err = vkCreateSemaphore(m_device.handle(),
                            &presentCompleteSemaphoreCreateInfo,
                            NULL, &presentCompleteSemaphore);
    assert(!err);

    // Get the index of the next available swapchain image:
    err = m_fpAcquireNextImageKHR(m_device.handle(), m_swap_chain,
                                      UINT64_MAX,
                                      presentCompleteSemaphore,
                                      VK_NULL_HANDLE,
                                      &m_current_buffer);
    // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
    // return codes
    assert(!err);

    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    buf.init_as_src(m_device, (VkDeviceSize)m_display_image->m_data_size, flags);
    dest_ptr = buf.memory().map();
    memcpy(dest_ptr, m_display_image->m_data, m_display_image->m_data_size);
    buf.memory().unmap();

    m_cmdbuf.begin();
    VkImageMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memoryBarrier.pNext = NULL;
    memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    memoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    memoryBarrier.subresourceRange.baseMipLevel = 0;
    memoryBarrier.subresourceRange.levelCount = 1;
    memoryBarrier.subresourceRange.baseArrayLayer = 0;
    memoryBarrier.subresourceRange.layerCount = 1;
    memoryBarrier.image = m_buffers[m_current_buffer].image;
    VkImageMemoryBarrier *pmemory_barrier = &memoryBarrier;
    vkCmdPipelineBarrier(m_cmdbuf.handle(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, NULL, 0, NULL, 1, pmemory_barrier);

    VkBufferImageCopy region = {};
    region.bufferRowLength = m_display_image->m_width;
    region.bufferImageHeight = m_display_image->m_height;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.height = m_display_image->m_height;
    region.imageExtent.width = m_display_image->m_width;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(m_cmdbuf.handle(),
        buf.handle(),
        m_buffers[m_current_buffer].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &region);

    memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    memoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    vkCmdPipelineBarrier(m_cmdbuf.handle(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, NULL, 0, NULL, 1, pmemory_barrier);
    m_cmdbuf.end();

    VkCommandBuffer cmdBufs[1];
    cmdBufs[0] = m_cmdbuf.handle();

    // Wait for the present complete semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.
    VkFence nullFence = { VK_NULL_HANDLE };
    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &presentCompleteSemaphore;
    submit_info.pWaitDstStageMask = &pipe_stage_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = cmdBufs;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    vkQueueSubmit(m_queue.handle(), 1, &submit_info, nullFence);
    m_queue.wait();

    vkDestroySemaphore(m_device.handle(), presentCompleteSemaphore, NULL);

    VkPresentInfoKHR present = {};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.pSwapchains = & m_swap_chain;
    present.pImageIndices = &m_current_buffer;

#ifndef _WIN32
    xcb_change_property (m_connection,
                         XCB_PROP_MODE_REPLACE,
                         m_window,
                         XCB_ATOM_WM_NAME,
                         XCB_ATOM_STRING,
                         8,
                         m_display_image->m_title.size(),
                         m_display_image->m_title.c_str());
#endif

    err = m_fpQueuePresentKHR(m_queue.handle(), &present);
    assert(!err);

    m_queue.wait();
    m_current_buffer = (m_current_buffer + 1) % 2;
}

#ifdef _WIN32
# define PREVIOUSLY_DOWN 1<<29
// MS-Windows event handling function:
LRESULT CALLBACK TestFrameworkVkPresent::WndProc(HWND hWnd,
                        UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
	
    switch(uMsg)
    {
       case WM_CLOSE:
            PostQuitMessage(0);
            break;
            
       case WM_PAINT:
       {
           TestFrameworkVkPresent* me = reinterpret_cast<TestFrameworkVkPresent*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
           if (me) {
               SetWindowText(hWnd, me->m_display_image->m_title.c_str());
               me->Display();
           }
       }
       break;

       case WM_KEYDOWN:
       {
           if (lParam & (PREVIOUSLY_DOWN)){
               break;
           }
          // To be able to be a CALLBACK, WndProc had to be static, so it doesn't get a this pointer.  When we created
          // the window, we put the this pointer into the window's user data so we could get it back now
          TestFrameworkVkPresent* me = reinterpret_cast<TestFrameworkVkPresent*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
          switch (wParam)
          {
          case VK_ESCAPE: me->m_quit = true;
              break;

          case VK_LEFT:          // left arrow key
              if (me->m_display_image == me->m_images.begin()) {
                  me->m_display_image = --me->m_images.end();
              }
              else {
                  --me->m_display_image;
              }
              break;

          case VK_RIGHT:          // right arrow key
              ++me->m_display_image;
              if (me->m_display_image == me->m_images.end()) {
                  me->m_display_image = me->m_images.begin();
              }
              break;

          default:
              break;
          }
          SetWindowText(hWnd, me->m_display_image->m_title.c_str());
          me->Display();
       }
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

void TestFrameworkVkPresent::Run()
{
    MSG msg;         // message

    /* main message loop*/
    while(! m_quit) {
        GetMessage(&msg, m_window, 0, 0);
        if (msg.message == WM_QUIT) {    
            m_quit = true; //if found, quit app
        } else {
            /* Translate and dispatch to event queue*/
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

#else
void  TestFrameworkVkPresent::HandleEvent(xcb_generic_event_t *event)
{
    uint8_t event_code = event->response_type & 0x7f;
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

void  TestFrameworkVkPresent::Run()
{
    xcb_flush(m_connection);

    while (! m_quit) {
        xcb_generic_event_t *event;

        if (m_pause) {
            event = xcb_wait_for_event(m_connection);
        } else {
            event = xcb_poll_for_event(m_connection);
        }
        if (event) {
            HandleEvent(event);
            free(event);
        }
    }
}
#endif // _WIN32

void TestFrameworkVkPresent::CreateSwapchain()
{
    VkResult U_ASSERT_ONLY err;

    m_display_image = m_images.begin();
    m_current_buffer = 0;

    // Create the WSI surface:
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.hinstance = m_connection;
    createInfo.hwnd = m_window;

    err = vkCreateWin32SurfaceKHR(m_instance, &createInfo, NULL, &m_surface);
#else  // _WIN32
    VkXcbSurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.connection = m_connection;
    createInfo.window = m_window;

    err = vkCreateXcbSurfaceKHR(m_instance, &createInfo, NULL, &m_surface);
#endif // _WIN32
    assert(!err);

    // Iterate over each queue to learn whether it supports presenting to WSI:
    VkBool32 supportsPresent;
    m_present_queue_node_index = UINT32_MAX;
    std::vector<vk_testing::Queue *> queues = m_device.graphics_queues();
    for (size_t i=0; i < queues.size(); i++)
    {
        int family_index = queues[i]->get_family_index();
        m_fpGetPhysicalDeviceSurfaceSupportKHR(m_device.phy().handle(),
                                               family_index,
                                               m_surface,
                                               &supportsPresent);
        if (supportsPresent) {
            m_present_queue_node_index = family_index;
        }
    }

    assert(m_present_queue_node_index != UINT32_MAX);


    // Get the list of VkFormat's that are supported:
    uint32_t formatCount;
    err = m_fpGetPhysicalDeviceSurfaceFormatsKHR(m_device.phy().handle(),
                                   m_surface,
                                   &formatCount, NULL);
    assert(!err);
    VkSurfaceFormatKHR *surfFormats =
        (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    err = m_fpGetPhysicalDeviceSurfaceFormatsKHR(m_device.phy().handle(),
                                   m_surface,
                                   &formatCount, surfFormats);
    assert(!err);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        m_format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        assert(formatCount >= 1);
        m_format = surfFormats[0].format;
    }
    m_color_space = surfFormats[0].colorSpace;

    // Check the surface capabilities and formats
    VkSurfaceCapabilitiesKHR surfCapabilities;
    err = m_fpGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device.phy().handle(),
        m_surface,
        &surfCapabilities);
    assert(!err);

    uint32_t presentModeCount;
    err = m_fpGetPhysicalDeviceSurfacePresentModesKHR(m_device.phy().handle(),
        m_surface,
        &presentModeCount, NULL);
    assert(!err);
    VkPresentModeKHR *presentModes =
        (VkPresentModeKHR *)malloc(presentModeCount * sizeof(VkPresentModeKHR));
    assert(presentModes);
    err = m_fpGetPhysicalDeviceSurfacePresentModesKHR(m_device.phy().handle(),
        m_surface,
        &presentModeCount, presentModes);
    assert(!err);

    VkExtent2D swapchainExtent;
    // width and height are either both -1, or both not -1.
    if (surfCapabilities.currentExtent.width == -1)
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width = m_width;
        swapchainExtent.height = m_height;
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfCapabilities.currentExtent;
    }

    // If mailbox mode is available, use it, as is the lowest-latency non-
    // tearing mode.  If not, try IMMEDIATE which will usually be available,
    // and is fastest (though it tears).  If not, fall back to FIFO which is
    // always available.
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (size_t i = 0; i < presentModeCount; i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
        if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
            (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
            swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    // Determine the number of VkImage's to use in the swap chain (we desire to
    // own only 1 image at a time, besides the images being displayed and
    // queued for display):
    uint32_t desiredNumberOfSwapchainImages = surfCapabilities.minImageCount + 1;
    if ((surfCapabilities.maxImageCount > 0) &&
        (desiredNumberOfSwapchainImages > surfCapabilities.maxImageCount))
    {
        // Application must settle for fewer images than desired:
        desiredNumberOfSwapchainImages = surfCapabilities.maxImageCount;
    }

    VkSurfaceTransformFlagsKHR preTransform;
    if (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surfCapabilities.currentTransform;
    }

    // We want to blit to the swap chain, ensure the driver supports it.  Color is always supported, per WSI spec.
    assert((surfCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) != 0);

    VkSwapchainCreateInfoKHR swap_chain = {};
    swap_chain.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_chain.pNext = NULL;
    swap_chain.surface = m_surface;
    swap_chain.minImageCount = desiredNumberOfSwapchainImages;
    swap_chain.imageFormat = m_format;
    swap_chain.imageColorSpace = m_color_space;
    swap_chain.imageExtent.width = swapchainExtent.width;
    swap_chain.imageExtent.height = swapchainExtent.height;
    swap_chain.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swap_chain.preTransform = (VkSurfaceTransformFlagBitsKHR) preTransform;
    swap_chain.imageArrayLayers = 1;
    swap_chain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swap_chain.queueFamilyIndexCount = 0;
    swap_chain.pQueueFamilyIndices = NULL;
    swap_chain.presentMode = swapchainPresentMode;
    swap_chain.oldSwapchain = VK_NULL_HANDLE;
    swap_chain.clipped = true;

    uint32_t i;

    err = m_fpCreateSwapchainKHR(m_device.handle(), &swap_chain, NULL, &m_swap_chain);
    assert(!err);

    err = m_fpGetSwapchainImagesKHR(m_device.handle(), m_swap_chain,
                                    &m_swapchainImageCount, NULL);
    assert(!err);

    VkImage* swapchainImages = (VkImage*)malloc(m_swapchainImageCount * sizeof(VkImage));
    assert(swapchainImages);
    err = m_fpGetSwapchainImagesKHR(m_device.handle(), m_swap_chain,
                                    &m_swapchainImageCount, swapchainImages);
    assert(!err);

    m_buffers = (SwapchainBuffers*)malloc(sizeof(SwapchainBuffers)*m_swapchainImageCount);
    assert(m_buffers);

    for (i = 0; i < m_swapchainImageCount; i++) {
        VkImageViewCreateInfo color_image_view = {};
        color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        color_image_view.pNext = NULL;
        color_image_view.format = m_format;
        color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
        color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
        color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
        color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
        color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_view.subresourceRange.baseMipLevel = 0;
        color_image_view.subresourceRange.levelCount = 1;
        color_image_view.subresourceRange.baseArrayLayer = 0;
        color_image_view.subresourceRange.layerCount = 1;
        color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        color_image_view.flags = 0;

        m_buffers[i].image = swapchainImages[i];

        color_image_view.image = m_buffers[i].image;
        err = vkCreateImageView(m_device.handle(),
                &color_image_view, NULL, &m_buffers[i].view);
        assert(!err);

        /* Set image layout to PRESENT_SOURCE_KHR so that before the copy, it can be set to */
        /* TRANSFER_DESTINATION_OPTIMAL                                                     */
        SetImageLayout(m_buffers[i].image, VK_IMAGE_ASPECT_COLOR_BIT,
                       VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    }
}
void TestFrameworkVkPresent::SetImageLayout(VkImage image, VkImageAspectFlags aspectMask,
                    VkImageLayout old_image_layout, VkImageLayout new_image_layout)
{
    VkResult U_ASSERT_ONLY err;

    VkCommandBufferBeginInfo cmd_buf_info = {};
    VkCommandBufferInheritanceInfo cmd_buf_hinfo = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmd_buf_info.pInheritanceInfo = &cmd_buf_hinfo;
    cmd_buf_hinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    cmd_buf_hinfo.pNext = NULL;
    cmd_buf_hinfo.renderPass = { VK_NULL_HANDLE };
    cmd_buf_hinfo.subpass = 0;
    cmd_buf_hinfo.framebuffer = { VK_NULL_HANDLE };
    cmd_buf_hinfo.occlusionQueryEnable = VK_FALSE;
    cmd_buf_hinfo.queryFlags = 0;
    cmd_buf_hinfo.pipelineStatistics = 0;

    err = vkBeginCommandBuffer(m_cmdbuf.handle(), &cmd_buf_info);
    assert(!err);

    VkImageMemoryBarrier image_memory_barrier = {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = NULL;
    image_memory_barrier.srcAccessMask = 0;
    image_memory_barrier.dstAccessMask = 0;
    image_memory_barrier.oldLayout = old_image_layout;
    image_memory_barrier.newLayout = new_image_layout;
    image_memory_barrier.image = image;
    image_memory_barrier.subresourceRange.aspectMask = aspectMask;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        /* Make sure anything that was copying from this image has completed */
        image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        /* Make sure any Copy or CPU writes to image are flushed */
        image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    }

    VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    vkCmdPipelineBarrier(m_cmdbuf.handle(), src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, pmemory_barrier);

    err = vkEndCommandBuffer(m_cmdbuf.handle());
    assert(!err);

    const VkCommandBuffer cmd_bufs[] = { m_cmdbuf.handle() };
    VkFence nullFence = { VK_NULL_HANDLE };
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = cmd_bufs;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    err = vkQueueSubmit(m_queue.handle(), 1, &submit_info, nullFence);
    assert(!err);

    err = vkQueueWaitIdle(m_queue.handle());
    assert(!err);

}

void  TestFrameworkVkPresent::InitPresentFramework(std::list<VkTestImageRecord>  &imagesIn, VkInstance inst)
{
    m_instance = inst;
    GET_INSTANCE_PROC_ADDR(inst, GetPhysicalDeviceSurfaceSupportKHR);
    GET_INSTANCE_PROC_ADDR(inst, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_INSTANCE_PROC_ADDR(inst, GetPhysicalDeviceSurfaceFormatsKHR);
    GET_INSTANCE_PROC_ADDR(inst, GetPhysicalDeviceSurfacePresentModesKHR);
    GET_INSTANCE_PROC_ADDR(inst, DestroySurfaceKHR);
    GET_DEVICE_PROC_ADDR(m_device.handle(), CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(m_device.handle(), CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(m_device.handle(), DestroySwapchainKHR);
    GET_DEVICE_PROC_ADDR(m_device.handle(), GetSwapchainImagesKHR);
    GET_DEVICE_PROC_ADDR(m_device.handle(), AcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(m_device.handle(), QueuePresentKHR);

    m_images = imagesIn;
}

#ifdef _WIN32
void  TestFrameworkVkPresent::CreateMyWindow(bool register_class)
{
    WNDCLASSEX  win_class;
    // const ::testing::TestInfo* const test_info =
    // 	::testing::UnitTest::GetInstance()->current_test_info();
    m_connection = GetModuleHandle(NULL);

    for (std::list<VkTestImageRecord>::const_iterator it = m_images.begin();
        it != m_images.end(); it++) {
        if (m_width < it->m_width)
            m_width = it->m_width;
        if (m_height < it->m_height)
            m_height = it->m_height;
    }

    if (register_class) {
        // Initialize the window class structure:
        win_class.cbSize = sizeof(WNDCLASSEX);
        win_class.style = CS_HREDRAW | CS_VREDRAW;
        win_class.lpfnWndProc = (WNDPROC) &TestFrameworkVkPresent::WndProc;
        win_class.cbClsExtra = 0;
        win_class.cbWndExtra = 0;
        win_class.hInstance = m_connection; // hInstance
        win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
        win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        win_class.lpszMenuName = NULL;
        win_class.lpszClassName = "Test";
        win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
        // Register window class:
        if (!RegisterClassEx(&win_class)) {
            // It didn't work, so try to give a useful error:
            printf("Unexpected error trying to start the application!\n");
            fflush(stdout);
            exit(1);
        }
    }
   // Create window with the registered class:
    RECT wr = { 0, 0, m_width, m_height };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    m_window = CreateWindowEx(0,
                                  "Test",           // class name
                                  "Test",           // app name
                                  WS_OVERLAPPEDWINDOW | // window style
                                  WS_VISIBLE |
                                  WS_SYSMENU,
                                  100,100,              // x/y coords
                                  wr.right - wr.left,   // width
                                  wr.bottom - wr.top,   // height
                                  NULL,                 // handle to parent
                                  NULL,                 // handle to menu
                                  m_connection,     // hInstance
                                  NULL);                // no extra parameters

   if (!m_window) {
        // It didn't work, so try to give a useful error:
        DWORD error = GetLastError();
        char message[120];
        sprintf(message, "Cannot create a window in which to draw!\n GetLastError = %d", error);
        MessageBox(NULL, message, "Error", MB_OK);
        exit(1);
    }
    // Put our this pointer into the window's user data so our WndProc can use it when it starts.
    SetWindowLongPtr(m_window, GWLP_USERDATA, (LONG_PTR) this);
}
#else
void  TestFrameworkVkPresent::CreateMyWindow()
{
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;
    uint32_t value_mask, value_list[32];

    m_connection = xcb_connect(NULL, &scr);

    setup = xcb_get_setup(m_connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);

    m_screen = iter.data;

    for (std::list<VkTestImageRecord>::const_iterator it = m_images.begin();
         it != m_images.end(); it++) {
        if (m_width < it->m_width)
            m_width = it->m_width;
        if (m_height < it->m_height)
            m_height = it->m_height;
    }

    m_window = xcb_generate_id(m_connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = m_screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE |
                    XCB_EVENT_MASK_EXPOSURE |
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(m_connection,
            XCB_COPY_FROM_PARENT,
            m_window, m_screen->root,
            0, 0, m_width, m_height, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            m_screen->root_visual,
            value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(m_connection, 1, 12,
                                                      "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(m_connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(m_connection, 0, 16, "WM_DELETE_WINDOW");
    m_atom_wm_delete_window = xcb_intern_atom_reply(m_connection, cookie2, 0);

    xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE,
                        m_window, (*reply).atom, 4, 32, 1,
                        &(*m_atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(m_connection, m_window);
}
#endif

void TestFrameworkVkPresent::TearDown()
{
    m_fpDestroySwapchainKHR(m_device.handle(), m_swap_chain, NULL);

    for (uint32_t i = 0; i < m_swapchainImageCount; i++) {
        vkDestroyImageView(m_device.handle(), m_buffers[i].view, NULL);
    }
#ifndef _WIN32
    xcb_destroy_window(m_connection, m_window);
    xcb_disconnect(m_connection);
#endif
}

void TestFrameworkVkPresent::DestroyMyWindow()
{
#ifndef _WIN32
    xcb_destroy_window(m_connection, m_window);
    xcb_disconnect(m_connection);
#else
    DestroyWindow(m_window);
#endif
}

void VkTestFramework::Finish()
{
    if (m_images.size() == 0) return;

    vk_testing::Environment env;
    env.SetUp();
    {
        TestFrameworkVkPresent vkPresent(env.default_device());

        vkPresent.InitPresentFramework(m_images, env.get_instance());
#ifdef _WIN32
        vkPresent.CreateMyWindow(false);
#else
        vkPresent.CreateMyWindow();
#endif
        vkPresent.CreateSwapchain();
        vkPresent.Run();
        vkPresent.TearDown();
    }
    env.TearDown();
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
bool VkTestFramework::SetConfigFile(const std::string& name)
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
void VkTestFramework::ProcessConfigFile()
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
        config = (char *) alloca(strlen(DefaultConfig) + 1);
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

void VkTestFramework::SetMessageOptions(EShMessages& messages)
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
char** VkTestFramework::ReadFileData(const char* fileName)
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

void VkTestFramework::FreeFileData(char** data)
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
EShLanguage VkTestFramework::FindLanguage(const std::string& name)
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
// Convert VK shader type to compiler's
//
EShLanguage VkTestFramework::FindLanguage(const VkShaderStageFlagBits shader_type)
{
    switch (shader_type) {
    case VK_SHADER_STAGE_VERTEX_BIT:
        return EShLangVertex;

    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        return EShLangTessControl;

    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        return EShLangTessEvaluation;

    case VK_SHADER_STAGE_GEOMETRY_BIT:
        return EShLangGeometry;

    case VK_SHADER_STAGE_FRAGMENT_BIT:
        return EShLangFragment;

    case VK_SHADER_STAGE_COMPUTE_BIT:
        return EShLangCompute;

    default:
        return EShLangVertex;
    }
}


//
// Compile a given string containing GLSL into SPV for use by VK
// Return value of false means an error was encountered.
//
bool VkTestFramework::GLSLtoSPV(const VkShaderStageFlagBits shader_type,
                                 const char *pshader,
                                 std::vector<unsigned int> &spirv)
{
    glslang::TProgram program;
    const char *shaderStrings[1];

    // TODO: Do we want to load a special config file depending on the
    // shader source? Optional name maybe?
    //    SetConfigFile(fileName);

    ProcessConfigFile();

    EShMessages messages = EShMsgDefault;
    SetMessageOptions(messages);
    messages = static_cast<EShMessages>(messages | EShMsgSpvRules | EShMsgVulkanRules);

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

    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);

    //
    // Test the different modes of SPIR-V modification
    //
    if (this->m_canonicalize_spv) {
        spv::spirvbin_t(0).remap(spirv, spv::spirvbin_t::ALL_BUT_STRIP);
    }

    if (this->m_strip_spv) {
        spv::spirvbin_t(0).remap(spirv, spv::spirvbin_t::STRIP);
    }

    if (this->m_do_everything_spv) {
        spv::spirvbin_t(0).remap(spirv, spv::spirvbin_t::DO_EVERYTHING);
    }

    delete shader;

    return true;
}



VkTestImageRecord::VkTestImageRecord() : // Constructor
    m_width( 0 ),
    m_height( 0 ),
    m_data( NULL ),
    m_data_size( 0 )
{
}

VkTestImageRecord::~VkTestImageRecord()
{

}

VkTestImageRecord::VkTestImageRecord(const VkTestImageRecord &copyin)   // Copy constructor to handle pass by value.
{
    m_title = copyin.m_title;
    m_width = copyin.m_width;
    m_height = copyin.m_height;
    m_data_size = copyin.m_data_size;
    m_data = copyin.m_data; // TODO: Do we need to copy the data or is pointer okay?
}

ostream &operator<<(ostream &output, const VkTestImageRecord &VkTestImageRecord)
{
    output << VkTestImageRecord.m_title << " (" << VkTestImageRecord.m_width <<
              "," << VkTestImageRecord.m_height << ")" << endl;
    return output;
}

VkTestImageRecord& VkTestImageRecord::operator=(const VkTestImageRecord &rhs)
{
    m_title = rhs.m_title;
    m_width = rhs.m_width;
    m_height = rhs.m_height;
    m_data_size = rhs.m_data_size;
    m_data = rhs.m_data;
    return *this;
}

int VkTestImageRecord::operator==(const VkTestImageRecord &rhs) const
{
    if( this->m_data != rhs.m_data) return 0;
    return 1;
}

// This function is required for built-in STL list functions like sort
int VkTestImageRecord::operator<(const VkTestImageRecord &rhs) const
{
    if( this->m_data_size < rhs.m_data_size ) return 1;
    return 0;
}

