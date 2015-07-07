/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
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
 *
 * Authors:
 *   Chia-I Wu <olv@lunarg.com>
 */

#include "dev.h"
#include "gpu.h"
#include "mem.h"
#include "obj.h"
#include "vk_debug_marker_lunarg.h"

VkResult intel_base_get_memory_requirements(struct intel_base *base, VkMemoryRequirements* pRequirements)
{
    memset(pRequirements, 0, sizeof(VkMemoryRequirements));
    pRequirements->memoryTypeBits = (1<< INTEL_MEMORY_TYPE_COUNT) - 1;

    return VK_SUCCESS;
}

static bool base_dbg_copy_create_info(const struct intel_handle *handle,
                                      struct intel_base_dbg *dbg,
                                      const void *create_info)
{
    const union {
        const void *ptr;
        const struct {
            VkStructureType struct_type;
            void *next;
        } *header;
    } info = { .ptr = create_info };
    size_t shallow_copy = 0;

    if (!create_info)
        return true;

    switch (dbg->type) {
    case VK_OBJECT_TYPE_DEVICE:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
        break;
    case VK_OBJECT_TYPE_DEVICE_MEMORY:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO);
        break;
    case VK_OBJECT_TYPE_EVENT:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_EVENT_CREATE_INFO);
        shallow_copy = sizeof(VkEventCreateInfo);
        break;
    case VK_OBJECT_TYPE_FENCE:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
        shallow_copy = sizeof(VkFenceCreateInfo);
        break;
    case VK_OBJECT_TYPE_QUERY_POOL:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO);
        shallow_copy = sizeof(VkQueryPoolCreateInfo);
        break;
    case VK_OBJECT_TYPE_BUFFER:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
        shallow_copy = sizeof(VkBufferCreateInfo);
        break;
    case VK_OBJECT_TYPE_BUFFER_VIEW:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO);
        shallow_copy = sizeof(VkBufferViewCreateInfo);
        break;
    case VK_OBJECT_TYPE_IMAGE:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
        shallow_copy = sizeof(VkImageCreateInfo);
        break;
    case VK_OBJECT_TYPE_IMAGE_VIEW:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
        shallow_copy = sizeof(VkImageViewCreateInfo);
        break;
    case VK_OBJECT_TYPE_ATTACHMENT_VIEW:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_ATTACHMENT_VIEW_CREATE_INFO);
        shallow_copy = sizeof(VkAttachmentViewCreateInfo);
        break;
    case VK_OBJECT_TYPE_SAMPLER:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
        shallow_copy = sizeof(VkSamplerCreateInfo);
        break;
    case VK_OBJECT_TYPE_DESCRIPTOR_SET:
        /* no create info */
        break;
    case VK_OBJECT_TYPE_DYNAMIC_VP_STATE:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO);
        shallow_copy = sizeof(VkDynamicVpStateCreateInfo);
        break;
    case VK_OBJECT_TYPE_DYNAMIC_RS_STATE:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO);
        shallow_copy = sizeof(VkDynamicRsStateCreateInfo);
        break;
    case VK_OBJECT_TYPE_DYNAMIC_CB_STATE:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO);
        shallow_copy = sizeof(VkDynamicCbStateCreateInfo);
        break;
    case VK_OBJECT_TYPE_DYNAMIC_DS_STATE:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO);
        shallow_copy = sizeof(VkDynamicDsStateCreateInfo);
        break;
    case VK_OBJECT_TYPE_COMMAND_BUFFER:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO);
        shallow_copy = sizeof(VkCmdBufferCreateInfo);
        break;
    case VK_OBJECT_TYPE_PIPELINE:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
        break;
    case VK_OBJECT_TYPE_SHADER:
        assert(info.header->struct_type == VK_STRUCTURE_TYPE_SHADER_CREATE_INFO);
        shallow_copy = sizeof(VkShaderCreateInfo);
        break;
    case VK_OBJECT_TYPE_FRAMEBUFFER:
        assert(info.header->struct_type ==  VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
        shallow_copy = sizeof(VkFramebufferCreateInfo);
        break;
    case VK_OBJECT_TYPE_RENDER_PASS:
        assert(info.header->struct_type ==  VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
        shallow_copy = sizeof(VkRenderPassCreateInfo);
        break;
    case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
        assert(info.header->struct_type ==  VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
        /* TODO */
        shallow_copy = sizeof(VkDescriptorSetLayoutCreateInfo) * 0;
        break;
    case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
        assert(info.header->struct_type ==  VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
        shallow_copy = sizeof(VkDescriptorPoolCreateInfo);
        break;
    default:
        assert(!"unknown dbg object type");
        return false;
        break;
    }

    if (shallow_copy) {
        dbg->create_info = intel_alloc(handle, shallow_copy, 0,
                VK_SYSTEM_ALLOC_TYPE_DEBUG);
        if (!dbg->create_info)
            return false;

        memcpy(dbg->create_info, create_info, shallow_copy);
        dbg->create_info_size = shallow_copy;
    } else if (info.header->struct_type ==
            VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO) {
        size_t size;
        const VkMemoryAllocInfo *src = info.ptr;
        VkMemoryAllocInfo *dst;
        uint8_t *d;
        size = sizeof(*src);

        dbg->create_info_size = size;
        dst = intel_alloc(handle, size, 0, VK_SYSTEM_ALLOC_TYPE_DEBUG);
        if (!dst)
            return false;
        memcpy(dst, src, sizeof(*src));

        d = (uint8_t *) dst;
        d += sizeof(*src);

        dbg->create_info = dst;
    } else if (info.header->struct_type ==
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO) {
        const VkDeviceCreateInfo *src = info.ptr;
        VkDeviceCreateInfo *dst;
        uint8_t *d;
        size_t size;

        size = sizeof(*src);
        dbg->create_info_size = size;

        size += sizeof(src->pRequestedQueues[0]) * src->queueRecordCount;
        size += sizeof(src->ppEnabledExtensionNames[0]) * src->extensionCount;
        for (uint32_t i = 0; i < src->extensionCount; i++) {
            size += strlen(src->ppEnabledExtensionNames[i]) + 1;
        }

        dst = intel_alloc(handle, size, 0, VK_SYSTEM_ALLOC_TYPE_DEBUG);
        if (!dst)
            return false;

        memcpy(dst, src, sizeof(*src));

        d = (uint8_t *) dst;
        d += sizeof(*src);

        size = sizeof(src->pRequestedQueues[0]) * src->queueRecordCount;
        memcpy(d, src->pRequestedQueues, size);
        dst->pRequestedQueues = (const VkDeviceQueueCreateInfo *) d;
        d += size;

        size = sizeof(src->ppEnabledExtensionNames[0]) * src->extensionCount;
        dst->ppEnabledExtensionNames = (const char **) d;
        memcpy(d, src->ppEnabledExtensionNames, size);
        d += size;
        for (uint32_t i = 0; i < src->extensionCount; i++) {
            char **ptr = (char **) &dst->ppEnabledExtensionNames[i];
            strcpy((char *) d, src->ppEnabledExtensionNames[i]);
            *ptr = (char *) d;
            d += strlen(src->ppEnabledExtensionNames[i]) + 1;
        }

        dbg->create_info = dst;
    } else if (info.header->struct_type == VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO) {
        // TODO: What do we want to copy here?
    }

    return true;
}

/**
 * Create an intel_base_dbg.  When dbg_size is non-zero, a buffer of that
 * size is allocated and zeroed.
 */
struct intel_base_dbg *intel_base_dbg_create(const struct intel_handle *handle,
                                             VkObjectType type,
                                             const void *create_info,
                                             size_t dbg_size)
{
    struct intel_base_dbg *dbg;

    if (!dbg_size)
        dbg_size = sizeof(*dbg);

    assert(dbg_size >= sizeof(*dbg));

    dbg = intel_alloc(handle, dbg_size, 0, VK_SYSTEM_ALLOC_TYPE_DEBUG);
    if (!dbg)
        return NULL;

    memset(dbg, 0, dbg_size);

    dbg->type = type;

    if (!base_dbg_copy_create_info(handle, dbg, create_info)) {
        intel_free(handle, dbg);
        return NULL;
    }

    return dbg;
}

void intel_base_dbg_destroy(const struct intel_handle *handle,
                            struct intel_base_dbg *dbg)
{
    if (dbg->tag)
        intel_free(handle, dbg->tag);

    if (dbg->create_info)
        intel_free(handle, dbg->create_info);

    intel_free(handle, dbg);
}

/**
 * Create an intel_base.  obj_size and dbg_size specify the real sizes of the
 * object and the debug metadata.  Memories are zeroed.
 */
struct intel_base *intel_base_create(const struct intel_handle *handle,
                                     size_t obj_size, bool debug,
                                     VkObjectType type,
                                     const void *create_info,
                                     size_t dbg_size)
{
    struct intel_base *base;

    if (!obj_size)
        obj_size = sizeof(*base);

    assert(obj_size >= sizeof(*base));

    base = intel_alloc(handle, obj_size, 0, VK_SYSTEM_ALLOC_TYPE_API_OBJECT);
    if (!base)
        return NULL;

    memset(base, 0, obj_size);
    intel_handle_init(&base->handle, type, handle->instance);

    if (debug) {
        base->dbg = intel_base_dbg_create(&base->handle,
                type, create_info, dbg_size);
        if (!base->dbg) {
            intel_free(handle, base);
            return NULL;
        }
    }

    base->get_memory_requirements = intel_base_get_memory_requirements;

    return base;
}

void intel_base_destroy(struct intel_base *base)
{
    if (base->dbg)
        intel_base_dbg_destroy(&base->handle, base->dbg);
    intel_free(base, base);
}

ICD_EXPORT VkResult VKAPI vkDestroyObject(
    VkDevice                                  device,
    VkObjectType                              objType,
    VkObject                                  object)
{
    struct intel_obj *obj = intel_obj(object);

    obj->destroy(obj);

    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkGetObjectMemoryRequirements(
    VkDevice                                    device,
    VkObjectType                                objType,
    VkObject                                    object,
    VkMemoryRequirements*                       pRequirements)
{
    struct intel_base *base = intel_base(object);

    return base->get_memory_requirements(base, pRequirements);
}

ICD_EXPORT VkResult VKAPI vkBindObjectMemory(
    VkDevice                                    device,
    VkObjectType                                objType,
    VkObject                                    object,
    VkDeviceMemory                              mem_,
    VkDeviceSize                                memOffset)
{
    struct intel_obj *obj = intel_obj(object);
    struct intel_mem *mem = intel_mem(mem_);

    intel_obj_bind_mem(obj, mem, memOffset);

    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkQueueBindSparseBufferMemory(
    VkQueue                                     queue,
    VkBuffer                                    buffer,
    uint32_t                                    numBindings,
    const VkSparseMemoryBindInfo*               pBindInfo)
{
    return VK_ERROR_UNKNOWN;
}

ICD_EXPORT VkResult VKAPI vkQueueBindSparseImageOpaqueMemory(
    VkQueue                                     queue,
    VkImage                                     image,
    uint32_t                                    numBindings,
    const VkSparseMemoryBindInfo*               pBindInfo)
{
    return VK_ERROR_UNKNOWN;
}


ICD_EXPORT VkResult VKAPI vkQueueBindSparseImageMemory(
    VkQueue                                     queue,
    VkImage                                     image,
    uint32_t                                    numBindings,
    const VkSparseImageMemoryBindInfo*          pBindInfo)
{
    return VK_ERROR_UNKNOWN;
}

