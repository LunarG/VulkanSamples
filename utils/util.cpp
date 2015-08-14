/*
 * Vulkan Samples Kit
 *
 * Copyright (C) 2015 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
VULKAN_SAMPLE_DESCRIPTION
samples utility functions
*/

#include <stdio.h>
#include <assert.h>
#include <cstdlib>
#include "util.hpp"

using namespace std;

string get_file_name(const string& s) {

    char sep = '/';

#ifdef _WIN32
    sep = '\\';
#endif

    //cout << "in get_file_name\n";
    size_t i = s.rfind(sep, s.length());
    if (i != string::npos) {
        return(s.substr(i+1, s.length() - i));
    }

    return("");
}


std::string get_base_data_dir()
{
    return std::string(VULKAN_SAMPLES_BASE_DIR) + "/data/";
}

std::string get_data_dir( std::string filename )
{
    std::string basedir = get_base_data_dir();
    // get the base filename
    std::string fname = get_file_name( filename );

    // get the prefix of the base filename, i.e. the part before the dash
    stringstream stream(fname);
    std::string prefix;
    getline(stream, prefix, '-');
    std::string ddir = basedir + prefix;
    return ddir;
}


VkResult memory_type_from_properties(struct sample_info &info, uint32_t typeBits, VkFlags properties, uint32_t *typeIndex)
{
     // Search memtypes to find first index with those properties
     for (uint32_t i = 0; i < 32; i++) {
         if ((typeBits & 1) == 1) {
             // Type is available, does it match user properties?
             if ((info.memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                 *typeIndex = i;
                 return VK_SUCCESS;
             }
         }
         typeBits >>= 1;
     }
     // No memory types matched, return failure
     return VK_UNSUPPORTED;
}

void set_image_layout(
        struct sample_info &info,
        VkImage image,
        VkImageAspect aspect,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout)
{
    VkResult res;

    if (info.cmd == VK_NULL_HANDLE) {
        VkCmdBufferCreateInfo cmd = {};
        cmd.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
        cmd.pNext = NULL;
        cmd.cmdPool = info.cmd_pool;
        cmd.level = VK_CMD_BUFFER_LEVEL_PRIMARY;
        cmd.flags = 0;

        res = vkCreateCommandBuffer(info.device, &cmd, &info.cmd);
        assert(!res);
    }

    VkCmdBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT |
                         VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT;

    res = vkBeginCommandBuffer(info.cmd, &cmd_buf_info);
    assert(!res);

    VkImageMemoryBarrier image_memory_barrier = {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = NULL;
    image_memory_barrier.outputMask = 0;
    image_memory_barrier.inputMask = 0;
    image_memory_barrier.oldLayout = old_image_layout;
    image_memory_barrier.newLayout = new_image_layout;
    image_memory_barrier.image = image;
    image_memory_barrier.subresourceRange.aspect = aspect;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.mipLevels = 1;
    image_memory_barrier.subresourceRange.baseArraySlice = 0;
    image_memory_barrier.subresourceRange.arraySize = 0;

    if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL) {
        /* Make sure anything that was copying from this image has completed */
        image_memory_barrier.inputMask = VK_MEMORY_INPUT_TRANSFER_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        /* Make sure any Copy or CPU writes to image are flushed */
        image_memory_barrier.outputMask = VK_MEMORY_OUTPUT_HOST_WRITE_BIT | VK_MEMORY_OUTPUT_TRANSFER_BIT;
    }

    VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    vkCmdPipelineBarrier(info.cmd, src_stages, dest_stages, false, 1, (const void * const*)&pmemory_barrier);
    res = vkEndCommandBuffer(info.cmd);
    assert(!res);
    const VkCmdBuffer cmd_bufs[] = { info.cmd };
    VkFence nullFence;
    nullFence.handle = VK_NULL_HANDLE;

    res = vkQueueSubmit(info.queue, 1, cmd_bufs, nullFence);
    assert(!res);

    res = vkQueueWaitIdle(info.queue);
    assert(!res);
}

bool read_ppm(char const*const filename, int *width, int *height, int rowPitch, char *dataPtr)
{
    /* TODO: make this more flexible in handling comments and whitespace in ppm file */

    FILE *fPtr = fopen(filename,"r");
    char header[16];

    if (!fPtr)
        return false;

    fgets(header, 16, fPtr); // P6
    fgets(header, 16, fPtr); // Width
    *width = atoi(header);
    fgets(header, 16, fPtr); // Height
    *height = atoi(header);
    if (dataPtr == NULL)
        return true;

    fgets(header, 16, fPtr); // Format

    for(int y = 0; y < *height; y++)
    {
        char *rowPtr = dataPtr;
        for(int x = 0; x < *width; x++)
        {
            fgets(rowPtr, 3, fPtr);
            rowPtr[3] = 0;
            rowPtr += 4;
        }
        dataPtr += rowPitch;
    }
    fclose(fPtr);
    return true;
}
