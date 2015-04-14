#ifndef WRITEPPM_H
#define WRITEPPM_H

#endif // WRITEPPM_H

#include <iostream>
#include <fstream>
#include <string>

void glvWritePPM( const char *basename, uint32_t width, uint32_t height, VK_IMAGE img, VK_GPU_MEMORY mem, vkFuncs *pVkFuncs)
{
    std::string filename;
    VK_RESULT err;
    unsigned int x, y;

    filename.append(basename);
    filename.append(".ppm");

    const VK_IMAGE_SUBRESOURCE sr = {
        VK_IMAGE_ASPECT_COLOR, 0, 0
    };
    VK_SUBRESOURCE_LAYOUT sr_layout;
    size_t data_size = sizeof(sr_layout);

    err =  pVkFuncs->real_vkGetImageSubresourceInfo( img, &sr,
                                      VK_INFO_TYPE_SUBRESOURCE_LAYOUT,
                                      &data_size, &sr_layout);
    assert(!err);

    const char *ptr;

    err = pVkFuncs->real_vkMapMemory( mem, 0, (void **) &ptr );
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

    err = pVkFuncs->real_vkUnmapMemory( mem );
    assert(!err);
}
