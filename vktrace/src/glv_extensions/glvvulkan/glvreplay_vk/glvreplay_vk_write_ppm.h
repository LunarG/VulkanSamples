#ifndef WRITEPPM_H
#define WRITEPPM_H

#endif // WRITEPPM_H

#include "vulkan.h"
#include <iostream>
#include <fstream>
#include <string>

void glvWritePPM( const char *basename, uint32_t width, uint32_t height, VkDevice device, VkImage img, VkDeviceMemory mem, vkFuncs *pVkFuncs)
{
    std::string filename;
    VkResult err;
    unsigned int x, y;

    filename.append(basename);
    filename.append(".ppm");

    const VkImageSubresource sr = {
        VK_IMAGE_ASPECT_COLOR, 0, 0
    };
    VkSubresourceLayout sr_layout;
//    size_t data_size = sizeof(sr_layout);
    err = pVkFuncs->real_vkGetImageSubresourceLayout(device, img, &sr, &sr_layout);
//            VkDevice device,
//        VkImage image,
//        const VkImageSubresource* pSubresource,
//        VkSubresourceLayout* pLayout);
//    err =  pVkFuncs->real_vkGetImageSubresourceInfo( device, img, &sr,
//                                      VK_SUBRESOURCE_INFO_TYPE_LAYOUT,
//                                      &data_size, &sr_layout);
    assert(!err);

    VkDeviceSize offset = 0;
    VkDeviceSize size = 0;
    VkMemoryMapFlags flags = 0;
    const char *ptr;

    err = pVkFuncs->real_vkMapMemory(device, mem, offset, size, flags, (void **) &ptr );
    assert(!err);

    ptr += sr_layout.offset;

    std::ofstream file (filename.c_str());

    file << "P6\n";
    file << width << "\n";
    file << height << "\n";
    file << 255 << "\n";

    for (y = 0; y < height; y++) {
        const int *row = (const int*) ptr;
        int swapped;

        for (x = 0; x < width; x++) {
            swapped = (*row & 0xff00ff00) | (*row & 0x000000ff) << 16 | (*row & 0x00ff0000) >> 16;
            file.write((char *)&swapped, 3);
            row++;
        }

        ptr += sr_layout.rowPitch;
    }

    file.close();

    err = pVkFuncs->real_vkUnmapMemory(device, mem );
    assert(!err);
}
