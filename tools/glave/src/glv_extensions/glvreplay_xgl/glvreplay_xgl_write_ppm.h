#ifndef WRITEPPM_H
#define WRITEPPM_H

#endif // WRITEPPM_H

#include <iostream>
#include <fstream>
#include <string>

void glvWritePPM( const char *basename, uint32_t width, uint32_t height, XGL_IMAGE img, XGL_GPU_MEMORY mem, xglFuncs *pXglFuncs)
{
    std::string filename;
    XGL_RESULT err;
    unsigned int x, y;

    filename.append(basename);
    filename.append(".ppm");

    const XGL_IMAGE_SUBRESOURCE sr = {
        XGL_IMAGE_ASPECT_COLOR, 0, 0
    };
    XGL_SUBRESOURCE_LAYOUT sr_layout;
    size_t data_size = sizeof(sr_layout);

    err =  pXglFuncs->real_xglGetImageSubresourceInfo( img, &sr,
                                      XGL_INFO_TYPE_SUBRESOURCE_LAYOUT,
                                      &data_size, &sr_layout);
    assert(!err);

    const char *ptr;

    err = pXglFuncs->real_xglMapMemory( mem, 0, (XGL_VOID **) &ptr );
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

    err = pXglFuncs->real_xglUnmapMemory( mem );
    assert(!err);
}
