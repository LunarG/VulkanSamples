//
// Copyright (C) 2015 Valve Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// Author: Chia-I Wu <olv@lunarg.com>
// Author: Jeremy Hayes <jeremy@lunarg.com>
// Author: Mike Stroyan <mike@LunarG.com>

// Blit (copy, clear, and resolve) tests

#include "math.h"
#include "test_common.h"
#include "vktestbinding.h"
#include "test_environment.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

namespace vk_testing {

size_t get_format_size(VkFormat format);

class ImageChecker {
public:
    explicit ImageChecker(const VkImageCreateInfo &info, const std::vector<VkBufferImageCopy> &regions)
        : info_(info), regions_(regions), pattern_(HASH) {}
    explicit ImageChecker(const VkImageCreateInfo &info, const std::vector<VkImageSubresourceRange> &ranges);
    explicit ImageChecker(const VkImageCreateInfo &info);

    void set_solid_pattern(const std::vector<uint8_t> &solid);

    VkDeviceSize buffer_size() const;
    bool fill(Buffer &buf) const { return walk(FILL, buf); }
    bool fill(Image &img) const { return walk(FILL, img); }
    bool check(Buffer &buf) const { return walk(CHECK, buf); }
    bool check(Image &img) const { return walk(CHECK, img); }

    const std::vector<VkBufferImageCopy> &regions() const { return regions_; }

    static void hash_salt_generate() { hash_salt_++; }

private:
    enum Action {
        FILL,
        CHECK,
    };

    enum Pattern {
        HASH,
        SOLID,
    };

    size_t buffer_cpp() const;
    VkSubresourceLayout buffer_layout(const VkBufferImageCopy &region) const;

    bool walk(Action action, Buffer &buf) const;
    bool walk(Action action, Image &img) const;
    bool walk_region(Action action, const VkBufferImageCopy &region, const VkSubresourceLayout &layout, void *data) const;

    std::vector<uint8_t> pattern_hash(const VkImageSubresourceLayers &subres, const VkOffset3D &offset) const;

    static uint32_t hash_salt_;

    VkImageCreateInfo info_;
    std::vector<VkBufferImageCopy> regions_;

    Pattern pattern_;
    std::vector<uint8_t> pattern_solid_;
};


uint32_t ImageChecker::hash_salt_;

ImageChecker::ImageChecker(const VkImageCreateInfo &info)
    : info_(info), regions_(), pattern_(HASH)
{
    // create a region for every mip level in array slice 0
    VkDeviceSize offset = 0;
    for (uint32_t lv = 0; lv < info_.mipLevels; lv++) {
        VkBufferImageCopy region = {};

        region.bufferOffset = offset;
        region.imageSubresource.mipLevel = lv;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageExtent = Image::extent(info_.extent, lv);

        if (info_.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            if (info_.format != VK_FORMAT_S8_UINT) {
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                regions_.push_back(region);
            }

            if (info_.format == VK_FORMAT_D16_UNORM_S8_UINT ||
                info_.format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                info_.format == VK_FORMAT_S8_UINT) {
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
                regions_.push_back(region);
            }
        } else {
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            regions_.push_back(region);
        }

        offset += buffer_layout(region).size;
    }

    // arraySize should be limited in our tests.  If this proves to be an
    // issue, we can store only the regions for array slice 0 and be smart.
    if (info_.arrayLayers > 1) {
        const VkDeviceSize slice_pitch = offset;
        const uint32_t slice_region_count = regions_.size();

        regions_.reserve(slice_region_count * info_.arrayLayers);

        for (uint32_t slice = 1; slice < info_.arrayLayers; slice++) {
            for (uint32_t i = 0; i < slice_region_count; i++) {
                VkBufferImageCopy region = regions_[i];

                region.bufferOffset += slice_pitch * slice;
                region.imageSubresource.baseArrayLayer = slice;
                regions_.push_back(region);
            }
        }
    }
}

ImageChecker::ImageChecker(const VkImageCreateInfo &info, const std::vector<VkImageSubresourceRange> &ranges)
    : info_(info), regions_(), pattern_(HASH)
{
    VkDeviceSize offset = 0;
    for (std::vector<VkImageSubresourceRange>::const_iterator it = ranges.begin();
         it != ranges.end(); it++) {
        for (uint32_t lv = 0; lv < it->levelCount; lv++) {
            for (uint32_t layer = 0; layer < it->layerCount; layer++) {
                VkBufferImageCopy region = {};
                region.bufferOffset = offset;
                region.imageSubresource = Image::subresource(*it, lv, layer, 1);
                region.imageExtent = Image::extent(info_.extent, lv);

                regions_.push_back(region);

                offset += buffer_layout(region).size;
            }
        }
    }
}

void ImageChecker::set_solid_pattern(const std::vector<uint8_t> &solid)
{
    pattern_ = SOLID;
    pattern_solid_.clear();
    pattern_solid_.reserve(buffer_cpp());
    for (size_t i = 0; i < buffer_cpp(); i++)
        pattern_solid_.push_back(solid[i % solid.size()]);
}

size_t ImageChecker::buffer_cpp() const
{
    return get_format_size(info_.format);
}

VkSubresourceLayout ImageChecker::buffer_layout(const VkBufferImageCopy &region) const
{
    VkSubresourceLayout layout = {};
    layout.offset = region.bufferOffset;
    layout.rowPitch = buffer_cpp() * region.imageExtent.width;
    layout.depthPitch = layout.rowPitch * region.imageExtent.height;
    layout.size = layout.depthPitch * region.imageExtent.depth;

    return layout;
}

VkDeviceSize ImageChecker::buffer_size() const
{
    VkDeviceSize size = 0;

    for (std::vector<VkBufferImageCopy>::const_iterator it = regions_.begin();
         it != regions_.end(); it++) {
        const VkSubresourceLayout layout = buffer_layout(*it);
        if (size < layout.offset + layout.size)
            size = layout.offset + layout.size;
    }

    return size;
}

bool ImageChecker::walk_region(Action action, const VkBufferImageCopy &region,
                               const VkSubresourceLayout &layout, void *data) const
{
    for (int32_t z = 0; z < region.imageExtent.depth; z++) {
        for (int32_t y = 0; y < region.imageExtent.height; y++) {
            for (int32_t x = 0; x < region.imageExtent.width; x++) {
                uint8_t *dst = static_cast<uint8_t *>(data);
                dst += layout.offset + layout.depthPitch * z +
                    layout.rowPitch * y + buffer_cpp() * x;

                VkOffset3D offset = region.imageOffset;
                offset.x += x;
                offset.y += y;
                offset.z += z;

                const std::vector<uint8_t> &val = (pattern_ == HASH) ?
                    pattern_hash(region.imageSubresource, offset) :
                    pattern_solid_;
                assert(val.size() == buffer_cpp());

                if (action == FILL) {
                    memcpy(dst, &val[0], val.size());
                } else {
                    for (size_t i = 0; i < val.size(); i++) {
                        EXPECT_EQ(val[i], dst[i]) <<
                            "Offset is: (" << x << ", " << y << ", " << z << ")\n"
                            "Format is: (" << info_.format << ")\n";
                        if (val[i] != dst[i])
                            return false;
                    }
                }
            }
        }
    }

    return true;
}

bool ImageChecker::walk(Action action, Buffer &buf) const
{
    void *data = buf.memory().map();
    if (!data)
        return false;

    std::vector<VkBufferImageCopy>::const_iterator it;
    for (it = regions_.begin(); it != regions_.end(); it++) {
        if (!walk_region(action, *it, buffer_layout(*it), data))
            break;
    }

    buf.memory().unmap();

    return (it == regions_.end());
}

bool ImageChecker::walk(Action action, Image &img) const
{
    void *data = img.memory().map();
    if (!data)
        return false;

    std::vector<VkBufferImageCopy>::const_iterator it;
    for (it = regions_.begin(); it != regions_.end(); it++) {
        if (!walk_region(action, *it, img.subresource_layout(it->imageSubresource), data))
            break;
    }

    img.memory().unmap();

    return (it == regions_.end());
}

std::vector<uint8_t> ImageChecker::pattern_hash(const VkImageSubresourceLayers &subres, const VkOffset3D &offset) const
{
#define HASH_BYTE(val, b) static_cast<uint8_t>((static_cast<uint32_t>(val) >> (b * 8)) & 0xff)
#define HASH_BYTES(val) HASH_BYTE(val, 0), HASH_BYTE(val, 1), HASH_BYTE(val, 2), HASH_BYTE(val, 3)
    const unsigned char input[] = {
        HASH_BYTES(hash_salt_),
        HASH_BYTES(subres.mipLevel),
        HASH_BYTES(subres.baseArrayLayer),
        HASH_BYTES(offset.x),
        HASH_BYTES(offset.y),
        HASH_BYTES(offset.z),
    };
    unsigned long hash = 5381;

    for (int32_t i = 0; i < ARRAY_SIZE(input); i++)
        hash = ((hash << 5) + hash) + input[i];

    const uint8_t output[4] = { HASH_BYTES(hash) };
#undef HASH_BYTES
#undef HASH_BYTE

    std::vector<uint8_t> val;
    val.reserve(buffer_cpp());
    for (size_t i = 0; i < buffer_cpp(); i++)
        val.push_back(output[i % 4]);

    return val;
}

size_t get_format_size(VkFormat format)
{
    static bool format_table_unverified = true;
    static const struct format_info {
        VkFormat format;
        size_t size;
        uint32_t channel_count;
    } format_table[VK_FORMAT_RANGE_SIZE] = {
        { VK_FORMAT_UNDEFINED,             0,  0 },
        { VK_FORMAT_R4G4_UNORM,            1,  2 },
        { VK_FORMAT_R4G4_USCALED,          1,  2 },
        { VK_FORMAT_R4G4B4A4_UNORM,        2,  4 },
        { VK_FORMAT_R4G4B4A4_USCALED,      2,  4 },
        { VK_FORMAT_R5G6B5_UNORM,          2,  3 },
        { VK_FORMAT_R5G6B5_USCALED,        2,  3 },
        { VK_FORMAT_R5G5B5A1_UNORM,        2,  4 },
        { VK_FORMAT_R5G5B5A1_USCALED,      2,  4 },
        { VK_FORMAT_R8_UNORM,              1,  1 },
        { VK_FORMAT_R8_SNORM,              1,  1 },
        { VK_FORMAT_R8_USCALED,            1,  1 },
        { VK_FORMAT_R8_SSCALED,            1,  1 },
        { VK_FORMAT_R8_UINT,               1,  1 },
        { VK_FORMAT_R8_SINT,               1,  1 },
        { VK_FORMAT_R8_SRGB,               1,  1 },
        { VK_FORMAT_R8G8_UNORM,            2,  2 },
        { VK_FORMAT_R8G8_SNORM,            2,  2 },
        { VK_FORMAT_R8G8_USCALED,          2,  2 },
        { VK_FORMAT_R8G8_SSCALED,          2,  2 },
        { VK_FORMAT_R8G8_UINT,             2,  2 },
        { VK_FORMAT_R8G8_SINT,             2,  2 },
        { VK_FORMAT_R8G8_SRGB,             2,  2 },
        { VK_FORMAT_R8G8B8_UNORM,          3,  3 },
        { VK_FORMAT_R8G8B8_SNORM,          3,  3 },
        { VK_FORMAT_R8G8B8_USCALED,        3,  3 },
        { VK_FORMAT_R8G8B8_SSCALED,        3,  3 },
        { VK_FORMAT_R8G8B8_UINT,           3,  3 },
        { VK_FORMAT_R8G8B8_SINT,           3,  3 },
        { VK_FORMAT_R8G8B8_SRGB,           3,  3 },
        { VK_FORMAT_R8G8B8A8_UNORM,        4,  4 },
        { VK_FORMAT_R8G8B8A8_SNORM,        4,  4 },
        { VK_FORMAT_R8G8B8A8_USCALED,      4,  4 },
        { VK_FORMAT_R8G8B8A8_SSCALED,      4,  4 },
        { VK_FORMAT_R8G8B8A8_UINT,         4,  4 },
        { VK_FORMAT_R8G8B8A8_SINT,         4,  4 },
        { VK_FORMAT_R8G8B8A8_SRGB,         4,  4 },
        { VK_FORMAT_R10G10B10A2_UNORM,     4,  4 },
        { VK_FORMAT_R10G10B10A2_SNORM,     4,  4 },
        { VK_FORMAT_R10G10B10A2_USCALED,   4,  4 },
        { VK_FORMAT_R10G10B10A2_SSCALED,   4,  4 },
        { VK_FORMAT_R10G10B10A2_UINT,      4,  4 },
        { VK_FORMAT_R10G10B10A2_SINT,      4,  4 },
        { VK_FORMAT_R16_UNORM,             2,  1 },
        { VK_FORMAT_R16_SNORM,             2,  1 },
        { VK_FORMAT_R16_USCALED,           2,  1 },
        { VK_FORMAT_R16_SSCALED,           2,  1 },
        { VK_FORMAT_R16_UINT,              2,  1 },
        { VK_FORMAT_R16_SINT,              2,  1 },
        { VK_FORMAT_R16_SFLOAT,            2,  1 },
        { VK_FORMAT_R16G16_UNORM,          4,  2 },
        { VK_FORMAT_R16G16_SNORM,          4,  2 },
        { VK_FORMAT_R16G16_USCALED,        4,  2 },
        { VK_FORMAT_R16G16_SSCALED,        4,  2 },
        { VK_FORMAT_R16G16_UINT,           4,  2 },
        { VK_FORMAT_R16G16_SINT,           4,  2 },
        { VK_FORMAT_R16G16_SFLOAT,         4,  2 },
        { VK_FORMAT_R16G16B16_UNORM,       6,  3 },
        { VK_FORMAT_R16G16B16_SNORM,       6,  3 },
        { VK_FORMAT_R16G16B16_USCALED,     6,  3 },
        { VK_FORMAT_R16G16B16_SSCALED,     6,  3 },
        { VK_FORMAT_R16G16B16_UINT,        6,  3 },
        { VK_FORMAT_R16G16B16_SINT,        6,  3 },
        { VK_FORMAT_R16G16B16_SFLOAT,      6,  3 },
        { VK_FORMAT_R16G16B16A16_UNORM,    8,  4 },
        { VK_FORMAT_R16G16B16A16_SNORM,    8,  4 },
        { VK_FORMAT_R16G16B16A16_USCALED,  8,  4 },
        { VK_FORMAT_R16G16B16A16_SSCALED,  8,  4 },
        { VK_FORMAT_R16G16B16A16_UINT,     8,  4 },
        { VK_FORMAT_R16G16B16A16_SINT,     8,  4 },
        { VK_FORMAT_R16G16B16A16_SFLOAT,   8,  4 },
        { VK_FORMAT_R32_UINT,              4,  1 },
        { VK_FORMAT_R32_SINT,              4,  1 },
        { VK_FORMAT_R32_SFLOAT,            4,  1 },
        { VK_FORMAT_R32G32_UINT,           8,  2 },
        { VK_FORMAT_R32G32_SINT,           8,  2 },
        { VK_FORMAT_R32G32_SFLOAT,         8,  2 },
        { VK_FORMAT_R32G32B32_UINT,        12, 3 },
        { VK_FORMAT_R32G32B32_SINT,        12, 3 },
        { VK_FORMAT_R32G32B32_SFLOAT,      12, 3 },
        { VK_FORMAT_R32G32B32A32_UINT,     16, 4 },
        { VK_FORMAT_R32G32B32A32_SINT,     16, 4 },
        { VK_FORMAT_R32G32B32A32_SFLOAT,   16, 4 },
        { VK_FORMAT_R64_SFLOAT,            8,  1 },
        { VK_FORMAT_R64G64_SFLOAT,         16, 2 },
        { VK_FORMAT_R64G64B64_SFLOAT,      24, 3 },
        { VK_FORMAT_R64G64B64A64_SFLOAT,   32, 4 },
        { VK_FORMAT_R11G11B10_UFLOAT,      4,  3 },
        { VK_FORMAT_R9G9B9E5_UFLOAT,       4,  3 },
        { VK_FORMAT_D16_UNORM,             2,  1 },
        { VK_FORMAT_D24_UNORM_X8,          3,  1 },
        { VK_FORMAT_D32_SFLOAT,            4,  1 },
        { VK_FORMAT_S8_UINT,               1,  1 },
        { VK_FORMAT_D16_UNORM_S8_UINT,     3,  2 },
        { VK_FORMAT_D24_UNORM_S8_UINT,     4,  2 },
        { VK_FORMAT_D32_SFLOAT_S8_UINT,    8,  2 },
        { VK_FORMAT_BC1_RGB_UNORM_BLOCK,         8,  4 },
        { VK_FORMAT_BC1_RGB_SRGB_BLOCK,          8,  4 },
        { VK_FORMAT_BC1_RGBA_UNORM_BLOCK,        8,  4 },
        { VK_FORMAT_BC1_RGBA_SRGB_BLOCK,         8,  4 },
        { VK_FORMAT_BC2_UNORM_BLOCK,             16, 4 },
        { VK_FORMAT_BC2_SRGB_BLOCK,              16, 4 },
        { VK_FORMAT_BC3_UNORM_BLOCK,             16, 4 },
        { VK_FORMAT_BC3_SRGB_BLOCK,              16, 4 },
        { VK_FORMAT_BC4_UNORM_BLOCK,             8,  4 },
        { VK_FORMAT_BC4_SNORM_BLOCK,             8,  4 },
        { VK_FORMAT_BC5_UNORM_BLOCK,             16, 4 },
        { VK_FORMAT_BC5_SNORM_BLOCK,             16, 4 },
        { VK_FORMAT_BC6H_UFLOAT_BLOCK,           16, 4 },
        { VK_FORMAT_BC6H_SFLOAT_BLOCK,           16, 4 },
        { VK_FORMAT_BC7_UNORM_BLOCK,             16, 4 },
        { VK_FORMAT_BC7_SRGB_BLOCK,              16, 4 },
        // TODO: Initialize remaining compressed formats.
        { VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,     0, 0 },
        { VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,      0, 0 },
        { VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,   0, 0 },
        { VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,    0, 0 },
        { VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,   0, 0 },
        { VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,    0, 0 },
        { VK_FORMAT_EAC_R11_UNORM_BLOCK,         0, 0 },
        { VK_FORMAT_EAC_R11_SNORM_BLOCK,         0, 0 },
        { VK_FORMAT_EAC_R11G11_UNORM_BLOCK,      0, 0 },
        { VK_FORMAT_EAC_R11G11_SNORM_BLOCK,      0, 0 },
        { VK_FORMAT_ASTC_4x4_UNORM_BLOCK,        0, 0 },
        { VK_FORMAT_ASTC_4x4_SRGB_BLOCK,         0, 0 },
        { VK_FORMAT_ASTC_5x4_UNORM_BLOCK,        0, 0 },
        { VK_FORMAT_ASTC_5x4_SRGB_BLOCK,         0, 0 },
        { VK_FORMAT_ASTC_5x5_UNORM_BLOCK,        0, 0 },
        { VK_FORMAT_ASTC_5x5_SRGB_BLOCK,         0, 0 },
        { VK_FORMAT_ASTC_6x5_UNORM_BLOCK,        0, 0 },
        { VK_FORMAT_ASTC_6x5_SRGB_BLOCK,         0, 0 },
        { VK_FORMAT_ASTC_6x6_UNORM_BLOCK,        0, 0 },
        { VK_FORMAT_ASTC_6x6_SRGB_BLOCK,         0, 0 },
        { VK_FORMAT_ASTC_8x5_UNORM_BLOCK,        0, 0 },
        { VK_FORMAT_ASTC_8x5_SRGB_BLOCK,         0, 0 },
        { VK_FORMAT_ASTC_8x6_UNORM_BLOCK,        0, 0 },
        { VK_FORMAT_ASTC_8x6_SRGB_BLOCK,         0, 0 },
        { VK_FORMAT_ASTC_8x8_UNORM_BLOCK,        0, 0 },
        { VK_FORMAT_ASTC_8x8_SRGB_BLOCK,         0, 0 },
        { VK_FORMAT_ASTC_10x5_UNORM_BLOCK,       0, 0 },
        { VK_FORMAT_ASTC_10x5_SRGB_BLOCK,        0, 0 },
        { VK_FORMAT_ASTC_10x6_UNORM_BLOCK,       0, 0 },
        { VK_FORMAT_ASTC_10x6_SRGB_BLOCK,        0, 0 },
        { VK_FORMAT_ASTC_10x8_UNORM_BLOCK,       0, 0 },
        { VK_FORMAT_ASTC_10x8_SRGB_BLOCK,        0, 0 },
        { VK_FORMAT_ASTC_10x10_UNORM_BLOCK,      0, 0 },
        { VK_FORMAT_ASTC_10x10_SRGB_BLOCK,       0, 0 },
        { VK_FORMAT_ASTC_12x10_UNORM_BLOCK,      0, 0 },
        { VK_FORMAT_ASTC_12x10_SRGB_BLOCK,       0, 0 },
        { VK_FORMAT_ASTC_12x12_UNORM_BLOCK,      0, 0 },
        { VK_FORMAT_ASTC_12x12_SRGB_BLOCK,       0, 0 },
        { VK_FORMAT_B4G4R4A4_UNORM,        2, 4 },
        { VK_FORMAT_B5G5R5A1_UNORM,        2, 4 },
        { VK_FORMAT_B5G6R5_UNORM,          2, 3 },
        { VK_FORMAT_B5G6R5_USCALED,        2, 3 },
        { VK_FORMAT_B8G8R8_UNORM,          3, 3 },
        { VK_FORMAT_B8G8R8_SNORM,          3, 3 },
        { VK_FORMAT_B8G8R8_USCALED,        3, 3 },
        { VK_FORMAT_B8G8R8_SSCALED,        3, 3 },
        { VK_FORMAT_B8G8R8_UINT,           3, 3 },
        { VK_FORMAT_B8G8R8_SINT,           3, 3 },
        { VK_FORMAT_B8G8R8_SRGB,           3, 3 },
        { VK_FORMAT_B8G8R8A8_UNORM,        4, 4 },
        { VK_FORMAT_B8G8R8A8_SNORM,        4, 4 },
        { VK_FORMAT_B8G8R8A8_USCALED,      4, 4 },
        { VK_FORMAT_B8G8R8A8_SSCALED,      4, 4 },
        { VK_FORMAT_B8G8R8A8_UINT,         4, 4 },
        { VK_FORMAT_B8G8R8A8_SINT,         4, 4 },
        { VK_FORMAT_B8G8R8A8_SRGB,         4, 4 },
        { VK_FORMAT_B10G10R10A2_UNORM,     4, 4 },
        { VK_FORMAT_B10G10R10A2_SNORM,     4, 4 },
        { VK_FORMAT_B10G10R10A2_USCALED,   4, 4 },
        { VK_FORMAT_B10G10R10A2_SSCALED,   4, 4 },
        { VK_FORMAT_B10G10R10A2_UINT,      4, 4 },
        { VK_FORMAT_B10G10R10A2_SINT,      4, 4 },
    };
    if (format_table_unverified)
    {
        for (unsigned int i = 0; i < VK_FORMAT_RANGE_SIZE; i++)
        {
            assert(format_table[i].format == i);
        }
        format_table_unverified = false;
    }

    return format_table[format].size;
}

VkExtent3D get_mip_level_extent(const VkExtent3D &extent, uint32_t mip_level)
{
    const VkExtent3D ext = {
        (extent.width  >> mip_level) ? extent.width  >> mip_level : 1,
        (extent.height >> mip_level) ? extent.height >> mip_level : 1,
        (extent.depth  >> mip_level) ? extent.depth  >> mip_level : 1,
    };

    return ext;
}

}; // namespace vk_testing

namespace {

#define DO(action) ASSERT_EQ(true, action);

static vk_testing::Environment *environment;

class VkCmdBlitTest : public ::testing::Test {
protected:
    VkCmdBlitTest() :
        dev_(environment->default_device()),
        queue_(*dev_.graphics_queues()[0]),
        pool_(dev_, vk_testing::CommandPool::create_info(dev_.graphics_queue_node_index_)),
        cmd_(dev_, vk_testing::CommandBuffer::create_info(pool_.handle()))
    {
        // make sure every test uses a different pattern
        vk_testing::ImageChecker::hash_salt_generate();
    }

    bool submit_and_done()
    {
        queue_.submit(cmd_);
        queue_.wait();
        mem_refs_.clear();
        return true;
    }

    vk_testing::Device &dev_;
    vk_testing::Queue &queue_;
    vk_testing::CommandPool pool_;
    vk_testing::CommandBuffer cmd_;

    /* TODO: We should be able to remove these now */
    std::vector<VkDeviceMemory> mem_refs_;
};

typedef VkCmdBlitTest VkCmdFillBufferTest;

TEST_F(VkCmdFillBufferTest, Basic)
{
    vk_testing::Buffer buf;
    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    buf.init_as_dst(dev_, 20, reqs);

    cmd_.begin();
    vkCmdFillBuffer(cmd_.handle(), buf.handle(), 0, 4, 0x11111111);
    vkCmdFillBuffer(cmd_.handle(), buf.handle(), 4, 16, 0x22222222);
    cmd_.end();

    submit_and_done();

    const uint32_t *data = static_cast<const uint32_t *>(buf.memory().map());
    EXPECT_EQ(0x11111111, data[0]);
    EXPECT_EQ(0x22222222, data[1]);
    EXPECT_EQ(0x22222222, data[2]);
    EXPECT_EQ(0x22222222, data[3]);
    EXPECT_EQ(0x22222222, data[4]);
    buf.memory().unmap();
}

TEST_F(VkCmdFillBufferTest, Large)
{
    const VkDeviceSize size = 32 * 1024 * 1024;
    vk_testing::Buffer buf;
    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    buf.init_as_dst(dev_, size, reqs);

    cmd_.begin();
    vkCmdFillBuffer(cmd_.handle(), buf.handle(), 0, size / 2, 0x11111111);
    vkCmdFillBuffer(cmd_.handle(), buf.handle(), size / 2, size / 2, 0x22222222);
    cmd_.end();

    submit_and_done();

    const uint32_t *data = static_cast<const uint32_t *>(buf.memory().map());
    VkDeviceSize offset;
    for (offset = 0; offset < size / 2; offset += 4)
        EXPECT_EQ(0x11111111, data[offset / 4]) << "Offset is: " << offset;
    for (; offset < size; offset += 4)
        EXPECT_EQ(0x22222222, data[offset / 4]) << "Offset is: " << offset;
    buf.memory().unmap();
}

TEST_F(VkCmdFillBufferTest, Overlap)
{
    vk_testing::Buffer buf;
    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    buf.init_as_dst(dev_, 64, reqs);

    cmd_.begin();
    vkCmdFillBuffer(cmd_.handle(), buf.handle(), 0, 48, 0x11111111);
    vkCmdFillBuffer(cmd_.handle(), buf.handle(), 32, 32, 0x22222222);
    cmd_.end();

    submit_and_done();

    const uint32_t *data = static_cast<const uint32_t *>(buf.memory().map());
    VkDeviceSize offset;
    for (offset = 0; offset < 32; offset += 4)
        EXPECT_EQ(0x11111111, data[offset / 4]) << "Offset is: " << offset;
    for (; offset < 64; offset += 4)
        EXPECT_EQ(0x22222222, data[offset / 4]) << "Offset is: " << offset;
    buf.memory().unmap();
}

TEST_F(VkCmdFillBufferTest, MultiAlignments)
{
    vk_testing::Buffer bufs[9];
    VkDeviceSize size = 4;
    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    cmd_.begin();
    for (int i = 0; i < ARRAY_SIZE(bufs); i++) {
        bufs[i].init_as_dst(dev_, size, reqs);
        vkCmdFillBuffer(cmd_.handle(), bufs[i].handle(), 0, size, 0x11111111);
        size <<= 1;
    }
    cmd_.end();

    submit_and_done();

    size = 4;
    for (int i = 0; i < ARRAY_SIZE(bufs); i++) {
        const uint32_t *data = static_cast<const uint32_t *>(bufs[i].memory().map());
        VkDeviceSize offset;
        for (offset = 0; offset < size; offset += 4)
            EXPECT_EQ(0x11111111, data[offset / 4]) << "Buffser is: " << i << "\n" <<
                                                       "Offset is: " << offset;
        bufs[i].memory().unmap();

        size <<= 1;
    }
}

typedef VkCmdBlitTest VkCmdCopyBufferTest;

TEST_F(VkCmdCopyBufferTest, Basic)
{
    vk_testing::Buffer src, dst;
    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    src.init_as_src(dev_, 4, reqs);
    uint32_t *data = static_cast<uint32_t *>(src.memory().map());
    data[0] = 0x11111111;
    src.memory().unmap();

    dst.init_as_dst(dev_, 4, reqs);

    cmd_.begin();
    VkBufferCopy region = {};
    region.size = 4;
    vkCmdCopyBuffer(cmd_.handle(), src.handle(), dst.handle(), 1, &region);
    cmd_.end();

    submit_and_done();

    data = static_cast<uint32_t *>(dst.memory().map());
    EXPECT_EQ(0x11111111, data[0]);
    dst.memory().unmap();
}

TEST_F(VkCmdCopyBufferTest, Large)
{
    const VkDeviceSize size = 32 * 1024 * 1024;
    vk_testing::Buffer src, dst;
    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    src.init_as_src(dev_, size * sizeof(VkDeviceSize), reqs);
    VkDeviceSize *data = static_cast<VkDeviceSize *>(src.memory().map());
    VkDeviceSize offset;
    for (offset = 0; offset < size; offset += 4)
        data[offset / 4] = offset;
    src.memory().unmap();

    dst.init_as_dst(dev_, size * sizeof(VkDeviceSize), reqs);

    cmd_.begin();
    VkBufferCopy region = {};
    region.size = size * sizeof(VkDeviceSize);
    vkCmdCopyBuffer(cmd_.handle(), src.handle(), dst.handle(), 1, &region);
    cmd_.end();

    submit_and_done();

    data = static_cast<VkDeviceSize *>(dst.memory().map());
    for (offset = 0; offset < size; offset += 4)
        EXPECT_EQ(offset, data[offset / 4]);
    dst.memory().unmap();
}

TEST_F(VkCmdCopyBufferTest, MultiAlignments)
{
    const VkBufferCopy regions[] = {
        /* well aligned */
        {  0,   0,  256 },
        {  0, 256,  128 },
        {  0, 384,   64 },
        {  0, 448,   32 },
        {  0, 480,   16 },
        {  0, 496,    8 },

        /* ill aligned */
        {  7, 510,   16 },
        { 16, 530,   13 },
        { 32, 551,   16 },
        { 45, 570,   15 },
        { 50, 590,    1 },
    };
    vk_testing::Buffer src, dst;
    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    src.init_as_src(dev_, 256, reqs);
    uint8_t *data = static_cast<uint8_t *>(src.memory().map());
    for (int i = 0; i < 256; i++)
        data[i] = i;
    src.memory().unmap();

    dst.init_as_dst(dev_, 1024, reqs);

    cmd_.begin();
    vkCmdCopyBuffer(cmd_.handle(), src.handle(), dst.handle(), ARRAY_SIZE(regions), regions);
    cmd_.end();

    submit_and_done();

    data = static_cast<uint8_t *>(dst.memory().map());
    for (int i = 0; i < ARRAY_SIZE(regions); i++) {
        const VkBufferCopy &r = regions[i];

        for (int j = 0; j < r.size; j++) {
            EXPECT_EQ(r.srcOffset + j, data[r.dstOffset + j]) <<
                "Region is: " << i << "\n" <<
                "Offset is: " << r.dstOffset + j;
        }
    }
    dst.memory().unmap();
}

TEST_F(VkCmdCopyBufferTest, RAWHazard)
{
    vk_testing::Buffer bufs[3];
    VkEventCreateInfo event_info;
    VkEvent event;
    VkResult err;
    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    //        typedef struct VkEventCreateInfo_
    //        {
    //            VkStructureType                      sType;      // Must be VK_STRUCTURE_TYPE_EVENT_CREATE_INFO
    //            const void*                          pNext;      // Pointer to next structure
    //            VkFlags                              flags;      // Reserved
    //        } VkEventCreateInfo;
    memset(&event_info, 0, sizeof(event_info));
    event_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

    err = vkCreateEvent(dev_.handle(), &event_info, NULL, &event);
    ASSERT_VK_SUCCESS(err);

    err = vkResetEvent(dev_.handle(), event);
    ASSERT_VK_SUCCESS(err);

    for (int i = 0; i < ARRAY_SIZE(bufs); i++) {
        bufs[i].init_as_src_and_dst(dev_, 4, reqs);

        uint32_t *data = static_cast<uint32_t *>(bufs[i].memory().map());
        data[0] = 0x22222222 * (i + 1);
        bufs[i].memory().unmap();
    }

    cmd_.begin();

    vkCmdFillBuffer(cmd_.handle(), bufs[0].handle(), 0, 4, 0x11111111);
    // is this necessary?
    VkBufferMemoryBarrier memory_barrier = bufs[0].buffer_memory_barrier(
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, 0, 4);
    VkBufferMemoryBarrier *pmemory_barrier = &memory_barrier;

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    vkCmdPipelineBarrier(cmd_.handle(), src_stages, dest_stages, 0, 1, (const void * const*)&pmemory_barrier);

    VkBufferCopy region = {};
    region.size = 4;
    vkCmdCopyBuffer(cmd_.handle(), bufs[0].handle(), bufs[1].handle(), 1, &region);

    memory_barrier = bufs[1].buffer_memory_barrier(
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, 0, 4);
    pmemory_barrier = &memory_barrier;
    vkCmdPipelineBarrier(cmd_.handle(), src_stages, dest_stages, 0, 1, (const void * const*)&pmemory_barrier);

    vkCmdCopyBuffer(cmd_.handle(), bufs[1].handle(), bufs[2].handle(), 1, &region);

    /* Use vkCmdSetEvent and vkCmdWaitEvents to test them.
     * This could be vkCmdPipelineBarrier.
     */
    vkCmdSetEvent(cmd_.handle(), event, VK_PIPELINE_STAGE_TRANSFER_BIT);

    // Additional commands could go into the buffer here before the wait.

    memory_barrier = bufs[1].buffer_memory_barrier(
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, 0, 4);
    pmemory_barrier = &memory_barrier;
    vkCmdWaitEvents(cmd_.handle(), 1, &event, src_stages, dest_stages, 1, (const void **)&pmemory_barrier);

    cmd_.end();

    submit_and_done();

    const uint32_t *data = static_cast<const uint32_t *>(bufs[2].memory().map());
    EXPECT_EQ(0x11111111, data[0]);
    bufs[2].memory().unmap();

    vkDestroyEvent(dev_.handle(), event, NULL);

}

class VkCmdBlitImageTest : public VkCmdBlitTest {
protected:
    void init_test_formats(VkFlags features)
    {
        first_linear_format_ = VK_FORMAT_UNDEFINED;
        first_optimal_format_ = VK_FORMAT_UNDEFINED;

        for (std::vector<vk_testing::Device::Format>::const_iterator it = dev_.formats().begin();
             it != dev_.formats().end(); it++) {
            if (it->features & features) {
                test_formats_.push_back(*it);

                if (it->tiling == VK_IMAGE_TILING_LINEAR &&
                    first_linear_format_ == VK_FORMAT_UNDEFINED)
                    first_linear_format_ = it->format;
                if (it->tiling == VK_IMAGE_TILING_OPTIMAL &&
                    first_optimal_format_ == VK_FORMAT_UNDEFINED)
                    first_optimal_format_ = it->format;
            }
        }
    }

    void init_test_formats()
    {
        init_test_formats(static_cast<VkFlags>(-1));
    }

    void fill_src(vk_testing::Image &img, const vk_testing::ImageChecker &checker)
    {
        if (img.transparent()) {
            checker.fill(img);
            return;
        }

        ASSERT_EQ(true, img.copyable());

        vk_testing::Buffer in_buf;
        VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        in_buf.init_as_src(dev_, checker.buffer_size(), reqs);
        checker.fill(in_buf);

        // copy in and tile
        cmd_.begin();
        vkCmdCopyBufferToImage(cmd_.handle(), in_buf.handle(),
                img.handle(), VK_IMAGE_LAYOUT_GENERAL,
                checker.regions().size(), &checker.regions()[0]);
        cmd_.end();

        submit_and_done();
    }

    void check_dst(vk_testing::Image &img, const vk_testing::ImageChecker &checker)
    {
        if (img.transparent()) {
            DO(checker.check(img));
            return;
        }

        ASSERT_EQ(true, img.copyable());

        vk_testing::Buffer out_buf;
        VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        out_buf.init_as_dst(dev_, checker.buffer_size(), reqs);

        // copy out and linearize
        cmd_.begin();
        vkCmdCopyImageToBuffer(cmd_.handle(),
                img.handle(), VK_IMAGE_LAYOUT_GENERAL,
                out_buf.handle(),
                checker.regions().size(), &checker.regions()[0]);
        cmd_.end();

        submit_and_done();

        DO(checker.check(out_buf));
    }

    std::vector<vk_testing::Device::Format> test_formats_;
    VkFormat first_linear_format_;
    VkFormat first_optimal_format_;
};

class VkCmdCopyBufferToImageTest : public VkCmdBlitImageTest {
protected:
    virtual void SetUp()
    {
        VkCmdBlitTest::SetUp();
        init_test_formats(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
        ASSERT_NE(true, test_formats_.empty());
    }

    void test_copy_memory_to_image(const VkImageCreateInfo &img_info, const vk_testing::ImageChecker &checker)
    {
        vk_testing::Buffer buf;
        vk_testing::Image img;
        VkMemoryPropertyFlags buffer_reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        VkMemoryPropertyFlags image_reqs =
            (img_info.tiling == VK_IMAGE_TILING_LINEAR)?VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT:0;

        buf.init_as_src(dev_, checker.buffer_size(), buffer_reqs);
        checker.fill(buf);

        img.init(dev_, img_info, image_reqs);

        cmd_.begin();
        vkCmdCopyBufferToImage(cmd_.handle(),
                buf.handle(),
                img.handle(), VK_IMAGE_LAYOUT_GENERAL,
                checker.regions().size(), &checker.regions()[0]);
        cmd_.end();

        submit_and_done();

        check_dst(img, checker);
    }

    void test_copy_memory_to_image(const VkImageCreateInfo &img_info, const std::vector<VkBufferImageCopy> &regions)
    {
        vk_testing::ImageChecker checker(img_info, regions);
        test_copy_memory_to_image(img_info, checker);
    }

    void test_copy_memory_to_image(const VkImageCreateInfo &img_info)
    {
        vk_testing::ImageChecker checker(img_info);
        test_copy_memory_to_image(img_info, checker);
    }
};

TEST_F(VkCmdCopyBufferToImageTest, Basic)
{
    for (std::vector<vk_testing::Device::Format>::const_iterator it = test_formats_.begin();
         it != test_formats_.end(); it++) {

        // not sure what to do here
        if (it->format == VK_FORMAT_UNDEFINED ||
            (it->format >= VK_FORMAT_B8G8R8_UNORM &&
             it->format <= VK_FORMAT_B8G8R8_SRGB))
            continue;

        VkImageCreateInfo img_info = vk_testing::Image::create_info();
        img_info.imageType = VK_IMAGE_TYPE_2D;
        img_info.format = it->format;
        img_info.extent.width = 64;
        img_info.extent.height = 64;
        img_info.tiling = it->tiling;
        img_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        img_info.initialLayout = VK_IMAGE_LAYOUT_GENERAL;

        test_copy_memory_to_image(img_info);
    }
}

class VkCmdCopyImageToBufferTest : public VkCmdBlitImageTest {
protected:
    virtual void SetUp()
    {
        VkCmdBlitTest::SetUp();
        init_test_formats(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
        ASSERT_NE(true, test_formats_.empty());
    }

    void test_copy_image_to_memory(const VkImageCreateInfo &img_info, const vk_testing::ImageChecker &checker)
    {
        vk_testing::Image img;
        vk_testing::Buffer buf;
        VkMemoryPropertyFlags buffer_reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        VkMemoryPropertyFlags image_reqs =
            (img_info.tiling == VK_IMAGE_TILING_LINEAR)?VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT:0;

        img.init(dev_, img_info, image_reqs);
        fill_src(img, checker);

        buf.init_as_dst(dev_, checker.buffer_size(), buffer_reqs);

        cmd_.begin();
        vkCmdCopyImageToBuffer(cmd_.handle(),
                img.handle(), VK_IMAGE_LAYOUT_GENERAL,
                buf.handle(),
                checker.regions().size(), &checker.regions()[0]);
        cmd_.end();

        submit_and_done();

        checker.check(buf);
    }

    void test_copy_image_to_memory(const VkImageCreateInfo &img_info, const std::vector<VkBufferImageCopy> &regions)
    {
        vk_testing::ImageChecker checker(img_info, regions);
        test_copy_image_to_memory(img_info, checker);
    }

    void test_copy_image_to_memory(const VkImageCreateInfo &img_info)
    {
        vk_testing::ImageChecker checker(img_info);
        test_copy_image_to_memory(img_info, checker);
    }
};

TEST_F(VkCmdCopyImageToBufferTest, Basic)
{
    for (std::vector<vk_testing::Device::Format>::const_iterator it = test_formats_.begin();
         it != test_formats_.end(); it++) {

        // not sure what to do here
        if (it->format == VK_FORMAT_UNDEFINED ||
            (it->format >= VK_FORMAT_B8G8R8_UNORM &&
             it->format <= VK_FORMAT_B8G8R8_SRGB))
            continue;

        VkImageCreateInfo img_info = vk_testing::Image::create_info();
        img_info.imageType = VK_IMAGE_TYPE_2D;
        img_info.format = it->format;
        img_info.extent.width = 64;
        img_info.extent.height = 64;
        img_info.tiling = it->tiling;
        img_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                         VK_IMAGE_USAGE_TRANSFER_DST_BIT; // Going to fill it before copy
        img_info.initialLayout = VK_IMAGE_LAYOUT_GENERAL;

        test_copy_image_to_memory(img_info);
    }
}

class VkCmdCopyImageTest : public VkCmdBlitImageTest {
protected:
    virtual void SetUp()
    {
        VkCmdBlitTest::SetUp();
        init_test_formats(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
        ASSERT_NE(true, test_formats_.empty());
    }

    void test_copy_image(const VkImageCreateInfo &src_info, const VkImageCreateInfo &dst_info,
                         const std::vector<VkImageCopy> &copies)
    {
        // convert VkImageCopy to two sets of VkBufferImageCopy
        std::vector<VkBufferImageCopy> src_regions, dst_regions;
        VkDeviceSize src_offset = 0, dst_offset = 0;
        for (std::vector<VkImageCopy>::const_iterator it = copies.begin(); it != copies.end(); it++) {
            VkBufferImageCopy src_region = {}, dst_region = {};

            src_region.bufferOffset = src_offset;
            src_region.imageSubresource = it->srcSubresource;
            src_region.imageOffset = it->srcOffset;
            src_region.imageExtent = it->extent;
            src_regions.push_back(src_region);

            dst_region.bufferOffset = src_offset;
            dst_region.imageSubresource = it->dstSubresource;
            dst_region.imageOffset = it->dstOffset;
            dst_region.imageExtent = it->extent;
            dst_regions.push_back(dst_region);

            const VkDeviceSize size = it->extent.width * it->extent.height * it->extent.depth;
            src_offset += vk_testing::get_format_size(src_info.format) * size;
            dst_offset += vk_testing::get_format_size(dst_info.format) * size;
        }

        vk_testing::ImageChecker src_checker(src_info, src_regions);
        vk_testing::ImageChecker dst_checker(dst_info, dst_regions);
        VkMemoryPropertyFlags src_reqs =
            (src_info.tiling == VK_IMAGE_TILING_LINEAR)?VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT:0;
        VkMemoryPropertyFlags dst_reqs =
            (dst_info.tiling == VK_IMAGE_TILING_LINEAR)?VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT:0;



        vk_testing::Image src;
        src.init(dev_, src_info, src_reqs);
        fill_src(src, src_checker);

        vk_testing::Image dst;
        dst.init(dev_, dst_info, dst_reqs);

        cmd_.begin();
        vkCmdCopyImage(cmd_.handle(),
                        src.handle(), VK_IMAGE_LAYOUT_GENERAL,
                        dst.handle(), VK_IMAGE_LAYOUT_GENERAL,
                        copies.size(), &copies[0]);
        cmd_.end();

        submit_and_done();

        check_dst(dst, dst_checker);
    }
};

TEST_F(VkCmdCopyImageTest, Basic)
{
    for (std::vector<vk_testing::Device::Format>::const_iterator it = test_formats_.begin();
         it != test_formats_.end(); it++) {

        // not sure what to do here
        if (it->format == VK_FORMAT_UNDEFINED ||
            (it->format >= VK_FORMAT_B8G8R8_UNORM &&
             it->format <= VK_FORMAT_B8G8R8_SRGB))
            continue;

        VkImageCreateInfo img_info = vk_testing::Image::create_info();
        img_info.imageType = VK_IMAGE_TYPE_2D;
        img_info.format = it->format;
        img_info.extent.width = 64;
        img_info.extent.height = 64;
        img_info.tiling = it->tiling;
        img_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        img_info.initialLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkImageCopy copy = {};
        copy.srcSubresource = vk_testing::Image::subresource(VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1);
        copy.dstSubresource = copy.srcSubresource;
        copy.extent = img_info.extent;

        test_copy_image(img_info, img_info, std::vector<VkImageCopy>(&copy, &copy + 1));
    }
}

class VkCmdClearColorImageTest : public VkCmdBlitImageTest {
protected:
    VkCmdClearColorImageTest() {}

    virtual void SetUp()
    {
        VkCmdBlitTest::SetUp();

        init_test_formats(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

        ASSERT_NE(true, test_formats_.empty());
    }

    union Color {
        float color[4];
        uint32_t raw[4];
    };

    std::vector<uint8_t> color_to_raw(VkFormat format, const VkClearColorValue &color)
    {
        std::vector<uint8_t> raw;

        // TODO support all formats
        switch (format) {
        case VK_FORMAT_R8G8B8A8_UNORM:
            raw.push_back((uint8_t)(color.float32[0] * 255.0f));
            raw.push_back((uint8_t)(color.float32[1] * 255.0f));
            raw.push_back((uint8_t)(color.float32[2] * 255.0f));
            raw.push_back((uint8_t)(color.float32[3] * 255.0f));
            break;
        case VK_FORMAT_B8G8R8A8_UNORM:
            raw.push_back((uint8_t)(color.float32[2] * 255.0f));
            raw.push_back((uint8_t)(color.float32[1] * 255.0f));
            raw.push_back((uint8_t)(color.float32[0] * 255.0f));
            raw.push_back((uint8_t)(color.float32[3] * 255.0f));
            break;
        default:
            break;
        }

        return raw;
    }

    void test_clear_color_image(const VkImageCreateInfo &img_info,
                                const VkClearColorValue &clear_color,
                                const std::vector<VkImageSubresourceRange> &ranges)
    {
        vk_testing::Image img;
        VkMemoryPropertyFlags image_reqs =
            (img_info.tiling == VK_IMAGE_TILING_LINEAR)?VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT:0;

        img.init(dev_, img_info, image_reqs);
        const VkFlags all_cache_outputs =
                VK_ACCESS_HOST_WRITE_BIT |
                VK_ACCESS_SHADER_WRITE_BIT |
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_TRANSFER_WRITE_BIT;
        const VkFlags all_cache_inputs =
                VK_ACCESS_HOST_READ_BIT |
                VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
                VK_ACCESS_INDEX_READ_BIT |
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                VK_ACCESS_UNIFORM_READ_BIT |
                VK_ACCESS_SHADER_READ_BIT |
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_MEMORY_READ_BIT;

        std::vector<VkImageMemoryBarrier> to_clear;
        std::vector<VkImageMemoryBarrier *> p_to_clear;
        std::vector<VkImageMemoryBarrier> to_xfer;
        std::vector<VkImageMemoryBarrier *> p_to_xfer;

        for (std::vector<VkImageSubresourceRange>::const_iterator it = ranges.begin();
             it != ranges.end(); it++) {
            to_clear.push_back(img.image_memory_barrier(all_cache_outputs, all_cache_inputs,
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_GENERAL,
                    *it));
            p_to_clear.push_back(&to_clear.back());
            to_xfer.push_back(img.image_memory_barrier(all_cache_outputs, all_cache_inputs,
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_GENERAL, *it));
            p_to_xfer.push_back(&to_xfer.back());
        }

        cmd_.begin();

        VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        vkCmdPipelineBarrier(cmd_.handle(), src_stages, dest_stages, 0, 1, (const void * const*)&p_to_clear[0]);

        vkCmdClearColorImage(cmd_.handle(),
                              img.handle(), VK_IMAGE_LAYOUT_GENERAL,
                              &clear_color, ranges.size(), &ranges[0]);

        vkCmdPipelineBarrier(cmd_.handle(), src_stages, dest_stages, 0, 1, (const void * const*)&p_to_xfer[0]);

        cmd_.end();

        submit_and_done();

        // cannot verify
        if (!img.transparent() && !img.copyable())
            return;

        vk_testing::ImageChecker checker(img_info, ranges);

        const std::vector<uint8_t> solid_pattern = color_to_raw(img_info.format, clear_color);
        if (solid_pattern.empty())
            return;

        checker.set_solid_pattern(solid_pattern);
        check_dst(img, checker);
    }

    void test_clear_color_image(const VkImageCreateInfo &img_info,
                                const float color[4],
                                const std::vector<VkImageSubresourceRange> &ranges)
    {
        VkClearColorValue c = {};
        memcpy(c.float32, color, sizeof(c.float32));
        test_clear_color_image(img_info, c, ranges);
    }
};

TEST_F(VkCmdClearColorImageTest, Basic)
{
    for (std::vector<vk_testing::Device::Format>::const_iterator it = test_formats_.begin();
         it != test_formats_.end(); it++) {
        const float color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
        VkFormatProperties props;

        vkGetPhysicalDeviceFormatProperties(dev_.phy().handle(), it->format, &props);

        if (it->tiling == VK_IMAGE_TILING_LINEAR && !(props.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
            continue;

        if (it->tiling == VK_IMAGE_TILING_OPTIMAL && !(props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
            continue;

        VkImageCreateInfo img_info = vk_testing::Image::create_info();
        img_info.imageType = VK_IMAGE_TYPE_2D;
        img_info.format = it->format;
        img_info.extent.width = 64;
        img_info.extent.height = 64;
        img_info.tiling = it->tiling;
        img_info.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
        img_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // Going to check contents

        const VkImageSubresourceRange range =
            vk_testing::Image::subresource_range(img_info, VK_IMAGE_ASPECT_COLOR_BIT);
        std::vector<VkImageSubresourceRange> ranges(&range, &range + 1);

        test_clear_color_image(img_info, color, ranges);
    }
}

class VkCmdClearDepthStencilTest : public VkCmdBlitImageTest {
protected:
    virtual void SetUp()
    {
        VkCmdBlitTest::SetUp();
        init_test_formats(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        ASSERT_NE(true, test_formats_.empty());
    }

    std::vector<uint8_t> ds_to_raw(VkFormat format, float depth, uint32_t stencil)
    {
        std::vector<uint8_t> raw;

        // depth
        switch (format) {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D16_UNORM_S8_UINT:
            {
                const uint16_t unorm = (uint16_t)roundf(depth * 65535.0f);
                raw.push_back(unorm & 0xff);
                raw.push_back(unorm >> 8);
            }
            break;
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            {
                const union {
                    float depth;
                    uint32_t u32;
                } u = { depth };

                raw.push_back((u.u32      ) & 0xff);
                raw.push_back((u.u32 >>  8) & 0xff);
                raw.push_back((u.u32 >> 16) & 0xff);
                raw.push_back((u.u32 >> 24) & 0xff);
            }
            break;
        default:
            break;
        }

        // stencil
        switch (format) {
        case VK_FORMAT_S8_UINT:
            raw.push_back(stencil);
            break;
        case VK_FORMAT_D16_UNORM_S8_UINT:
            raw.push_back(stencil);
            raw.push_back(0);
            break;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            raw.push_back(stencil);
            raw.push_back(0);
            raw.push_back(0);
            raw.push_back(0);
            break;
        default:
            break;
        }

        return raw;
    }

    void test_clear_depth_stencil(const VkImageCreateInfo &img_info,
                                  float depth, uint32_t stencil,
                                  const std::vector<VkImageSubresourceRange> &ranges)
    {
        vk_testing::Image img;
        VkMemoryPropertyFlags image_reqs =
            (img_info.tiling == VK_IMAGE_TILING_LINEAR)?VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT:0;

        img.init(dev_, img_info, image_reqs);
        const VkFlags all_cache_outputs =
                VK_ACCESS_HOST_WRITE_BIT |
                VK_ACCESS_SHADER_WRITE_BIT |
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_TRANSFER_WRITE_BIT;
        const VkFlags all_cache_inputs =
                VK_ACCESS_HOST_READ_BIT |
                VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
                VK_ACCESS_INDEX_READ_BIT |
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                VK_ACCESS_UNIFORM_READ_BIT |
                VK_ACCESS_SHADER_READ_BIT |
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_MEMORY_READ_BIT;

        std::vector<VkImageMemoryBarrier> to_clear;
        std::vector<VkImageMemoryBarrier *> p_to_clear;
        std::vector<VkImageMemoryBarrier> to_xfer;
        std::vector<VkImageMemoryBarrier *> p_to_xfer;
        unsigned int i = 0;

        for (std::vector<VkImageSubresourceRange>::const_iterator it = ranges.begin();
             it != ranges.end(); it++) {
            to_clear.push_back(img.image_memory_barrier(all_cache_outputs, all_cache_inputs,
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_GENERAL,
                    *it));

            to_xfer.push_back(img.image_memory_barrier(all_cache_outputs, all_cache_inputs,
                    VK_IMAGE_LAYOUT_GENERAL,
                    VK_IMAGE_LAYOUT_GENERAL, *it));
        }
        for (std::vector<VkImageSubresourceRange>::const_iterator it = ranges.begin();
             it != ranges.end(); it++) {
            p_to_clear.push_back(to_clear.data() + i);
            p_to_xfer.push_back(to_xfer.data() + i);
            i++;
        }

        cmd_.begin();

        VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        vkCmdPipelineBarrier(cmd_.handle(), src_stages, dest_stages, 0, to_clear.size(), (const void * const*) p_to_clear.data());

        VkClearDepthStencilValue clear_value = {
            depth,
            stencil
        };
        vkCmdClearDepthStencilImage(cmd_.handle(),
                                    img.handle(), VK_IMAGE_LAYOUT_GENERAL,
                                    &clear_value,
                                    ranges.size(), &ranges[0]);

        vkCmdPipelineBarrier(cmd_.handle(), src_stages, dest_stages, 0, to_xfer.size(), (const void * const*)p_to_xfer.data());

        cmd_.end();

        submit_and_done();

        // cannot verify
        if (!img.transparent() && !img.copyable())
            return;

        vk_testing::ImageChecker checker(img_info, ranges);

        checker.set_solid_pattern(ds_to_raw(img_info.format, depth, stencil));
        check_dst(img, checker);
    }
};

TEST_F(VkCmdClearDepthStencilTest, Basic)
{
    for (std::vector<vk_testing::Device::Format>::const_iterator it = test_formats_.begin();
         it != test_formats_.end(); it++) {
        VkFormatProperties props;

        vkGetPhysicalDeviceFormatProperties(dev_.phy().handle(), it->format, &props);

        if (it->tiling == VK_IMAGE_TILING_LINEAR && !(props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
            continue;

        if (it->tiling == VK_IMAGE_TILING_OPTIMAL && !(props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
            continue;

        // known driver issues
        if (it->format == VK_FORMAT_S8_UINT ||
            it->format == VK_FORMAT_D24_UNORM_X8 ||
            it->format == VK_FORMAT_D16_UNORM_S8_UINT ||
            it->format == VK_FORMAT_D24_UNORM_S8_UINT)
            continue;

        VkImageCreateInfo img_info = vk_testing::Image::create_info();
        img_info.imageType = VK_IMAGE_TYPE_2D;
        img_info.format = it->format;
        img_info.extent.width = 64;
        img_info.extent.height = 64;
        img_info.tiling = it->tiling;
        img_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        img_info.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (it->format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
            it->format == VK_FORMAT_D16_UNORM_S8_UINT ||
            it->format == VK_FORMAT_D24_UNORM_S8_UINT) {
            aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        const VkImageSubresourceRange range =
            vk_testing::Image::subresource_range(img_info, aspect);
        std::vector<VkImageSubresourceRange> ranges(&range, &range + 1);

        test_clear_depth_stencil(img_info, 0.25f, 63, ranges);
    }
}

}; // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    vk_testing::set_error_callback(test_error_callback);

    environment = new vk_testing::Environment();

    if (!environment->parse_args(argc, argv))
        return -1;

    ::testing::AddGlobalTestEnvironment(environment);

    return RUN_ALL_TESTS();
}
