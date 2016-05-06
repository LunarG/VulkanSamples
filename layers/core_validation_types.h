/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Courtney Goeltzenleuchter <courtneygo@google.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 */
#ifndef CORE_VALIDATION_TYPES_H_
#define CORE_VALIDATION_TYPES_H_

// Check for noexcept support
#if defined(__clang__)
#if __has_feature(cxx_noexcept)
#define HAS_NOEXCEPT
#endif
#else
#if defined(__GXX_EXPERIMENTAL_CXX0X__) && __GNUC__ * 10 + __GNUC_MINOR__ >= 46
#define HAS_NOEXCEPT
#else
#if defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 190023026 && defined(_HAS_EXCEPTIONS) && _HAS_EXCEPTIONS
#define HAS_NOEXCEPT
#endif
#endif
#endif

#ifdef HAS_NOEXCEPT
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

#include "vulkan/vulkan.h"
#include <atomic>
#include <string.h>
#include <unordered_set>
#include <vector>

class BASE_NODE {
  public:
    std::atomic_int in_use;
};

class BUFFER_NODE : public BASE_NODE {
  public:
    using BASE_NODE::in_use;
    VkDeviceMemory mem;
    VkBufferCreateInfo createInfo;
    BUFFER_NODE() : mem(VK_NULL_HANDLE), createInfo{} { in_use.store(0); };
    BUFFER_NODE(const VkBufferCreateInfo *pCreateInfo) : mem(VK_NULL_HANDLE), createInfo(*pCreateInfo) { in_use.store(0); };
    BUFFER_NODE(const BUFFER_NODE &rh_obj) : mem(rh_obj.mem), createInfo(rh_obj.createInfo) { in_use.store(rh_obj.in_use.load()); };
};

typedef struct _SAMPLER_NODE {
    VkSampler sampler;
    VkSamplerCreateInfo createInfo;

    _SAMPLER_NODE(const VkSampler *ps, const VkSamplerCreateInfo *pci) : sampler(*ps), createInfo(*pci){};
} SAMPLER_NODE;

class IMAGE_NODE : public BASE_NODE {
  public:
    using BASE_NODE::in_use;
    VkImageCreateInfo createInfo;
    VkDeviceMemory mem;
    bool valid; // If this is a swapchain image backing memory track valid here as it doesn't have DEVICE_MEM_INFO
    VkDeviceSize memOffset;
    VkDeviceSize memSize;
    IMAGE_NODE() : createInfo{}, mem(VK_NULL_HANDLE), valid(false), memOffset(0), memSize(0) { in_use.store(0); };
    IMAGE_NODE(const VkImageCreateInfo *pCreateInfo)
        : createInfo(*pCreateInfo), mem(VK_NULL_HANDLE), valid(false), memOffset(0), memSize(0) {
        in_use.store(0);
    };
    IMAGE_NODE(const IMAGE_NODE &rh_obj)
        : createInfo(rh_obj.createInfo), mem(rh_obj.mem), valid(rh_obj.valid), memOffset(rh_obj.memOffset),
          memSize(rh_obj.memSize) {
        in_use.store(rh_obj.in_use.load());
    };
};

// Simple struct to hold handle and type of object so they can be uniquely identified and looked up in appropriate map
struct MT_OBJ_HANDLE_TYPE {
    uint64_t handle;
    VkDebugReportObjectTypeEXT type;
};

inline bool operator==(MT_OBJ_HANDLE_TYPE a, MT_OBJ_HANDLE_TYPE b) NOEXCEPT { return a.handle == b.handle && a.type == b.type; }

namespace std {
template <> struct hash<MT_OBJ_HANDLE_TYPE> {
    size_t operator()(MT_OBJ_HANDLE_TYPE obj) const NOEXCEPT { return hash<uint64_t>()(obj.handle) ^ hash<uint32_t>()(obj.type); }
};
}

struct MemRange {
    VkDeviceSize offset;
    VkDeviceSize size;
};

struct MEMORY_RANGE {
    uint64_t handle;
    VkDeviceMemory memory;
    VkDeviceSize start;
    VkDeviceSize end;
};

// Data struct for tracking memory object
struct DEVICE_MEM_INFO {
    void *object; // Dispatchable object used to create this memory (device of swapchain)
    bool valid;   // Stores if the memory has valid data or not
    VkDeviceMemory mem;
    VkMemoryAllocateInfo allocInfo;
    std::unordered_set<MT_OBJ_HANDLE_TYPE> objBindings;        // objects bound to this memory
    std::unordered_set<VkCommandBuffer> commandBufferBindings; // cmd buffers referencing this memory
    std::vector<MEMORY_RANGE> bufferRanges;
    std::vector<MEMORY_RANGE> imageRanges;
    VkImage image; // If memory is bound to image, this will have VkImage handle, else VK_NULL_HANDLE
    MemRange memRange;
    void *pData, *pDriverData;
};

class SWAPCHAIN_NODE {
  public:
    VkSwapchainCreateInfoKHR createInfo;
    uint32_t *pQueueFamilyIndices;
    std::vector<VkImage> images;
    SWAPCHAIN_NODE(const VkSwapchainCreateInfoKHR *pCreateInfo) : createInfo(*pCreateInfo), pQueueFamilyIndices(NULL) {
        if (pCreateInfo->queueFamilyIndexCount && pCreateInfo->imageSharingMode == VK_SHARING_MODE_CONCURRENT) {
            pQueueFamilyIndices = new uint32_t[pCreateInfo->queueFamilyIndexCount];
            memcpy(pQueueFamilyIndices, pCreateInfo->pQueueFamilyIndices, pCreateInfo->queueFamilyIndexCount * sizeof(uint32_t));
            createInfo.pQueueFamilyIndices = pQueueFamilyIndices;
        }
    }
    ~SWAPCHAIN_NODE() { delete[] pQueueFamilyIndices; }
};
#endif // CORE_VALIDATION_TYPES_H_