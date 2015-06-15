/*
 * Vulkan
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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <list>
#include <map>
#include <vector>
using namespace std;

#include "loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_struct_string_helper_cpp.h"
#include "mem_tracker.h"
#include "layers_config.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"
#include "layers_table.h"
#include "layer_data.h"
#include "layer_logging.h"
 static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(g_initOnce);

typedef struct _layer_data {
    debug_report_data *report_data;
    // TODO: put instance data here
    VkDbgMsgCallback logging_callback;
} layer_data;

struct devExts {
    bool wsi_lunarg_enabled;
};
static std::unordered_map<void *, struct devExts>     deviceExtMap;
static std::unordered_map<void *, layer_data *> layer_data_map;
static device_table_map mem_tracker_device_table_map;
static instance_table_map mem_tracker_instance_table_map;

// TODO : This can be much smarter, using separate locks for separate global data
static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;

#define MAX_BINDING 0xFFFFFFFF

unordered_map<VkCmdBuffer,    MT_CB_INFO>           cbMap;
unordered_map<VkDeviceMemory, MT_MEM_OBJ_INFO>      memObjMap;
unordered_map<VkObject,       MT_OBJ_INFO>          objectMap;
unordered_map<VkFence,        MT_FENCE_INFO>        fenceMap;    // Map fence to fence info
unordered_map<VkQueue,        MT_QUEUE_INFO>        queueMap;
unordered_map<VkSwapChainWSI, MT_SWAP_CHAIN_INFO*>  swapChainMap;

// TODO : Add per-device fence completion
static uint64_t   g_currentFenceId  = 1;

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key,
        std::unordered_map<void *, layer_data *> &data_map);

debug_report_data *mdd(VkObject object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MDD: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, my_data);
#endif
    assert(my_data->report_data != NULL);
    return my_data->report_data;
}

debug_report_data *mid(VkInstance object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(object), layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MID: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, my_data);
#endif
    assert(my_data->report_data != NULL);
    return my_data->report_data;
}

// Add new queue for this device to map container
static void add_queue_info(const VkQueue queue)
{
    MT_QUEUE_INFO* pInfo   = &queueMap[queue];
    pInfo->lastRetiredId   = 0;
    pInfo->lastSubmittedId = 0;
}

static void delete_queue_info_list(
    void)
{
    // Process queue list, cleaning up each entry before deleting
    queueMap.clear();
}

static void add_swap_chain_info(
    const VkSwapChainWSI swapChain)
{
    MT_SWAP_CHAIN_INFO* pInfo = new MT_SWAP_CHAIN_INFO;
    swapChainMap[swapChain] = pInfo;
}

// Add new CBInfo for this cb to map container
static void add_cmd_buf_info(
    const VkCmdBuffer cb)
{
    cbMap[cb].cmdBuffer = cb;
}

// Return ptr to Info in CB map, or NULL if not found
static MT_CB_INFO* get_cmd_buf_info(
    const VkCmdBuffer cb)
{
    unordered_map<VkCmdBuffer, MT_CB_INFO>::iterator item = cbMap.find(cb);
    if (item != cbMap.end()) {
        return &(*item).second;
    } else {
        return NULL;
    }
}

// Return object info for 'object' or return NULL if no info exists
static MT_OBJ_INFO* get_object_info(
    const VkObject object)
{
    unordered_map<VkObject, MT_OBJ_INFO>::iterator item = objectMap.find(object);
    if (item != objectMap.end()) {
        return &(*item).second;
    } else {
        return NULL;
    }
}

static MT_OBJ_INFO* add_object_info(
    VkObject         object,
    VkStructureType  sType,
    const void      *pCreateInfo,
    const int        struct_size,
    const char      *name_prefix)
{
    MT_OBJ_INFO* pInfo = &objectMap[object];
    memset(pInfo, 0, sizeof(MT_OBJ_INFO));
    memcpy(&pInfo->create_info, pCreateInfo, struct_size);
    sprintf(pInfo->object_name, "%s_%p", name_prefix, object);

    pInfo->object     = object;
    pInfo->ref_count  = 1;
    pInfo->sType      = sType;

    return pInfo;
}

// Add a fence, creating one if necessary to our list of fences/fenceIds
static uint64_t add_fence_info(
    VkFence fence,
    VkQueue queue)
{
    // Create fence object
    uint64_t       fenceId    = g_currentFenceId++;
    // If no fence, create an internal fence to track the submissions
    if (fence != NULL) {
        fenceMap[fence].fenceId = fenceId;
        fenceMap[fence].queue = queue;
        // Validate that fence is in UNSIGNALED state
        MT_OBJ_INFO* pObjectInfo = get_object_info(fence);
        if (pObjectInfo != NULL) {
            if (pObjectInfo->create_info.fence_create_info.flags & VK_FENCE_CREATE_SIGNALED_BIT) {
                log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_FENCE, fence, 0, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                        "Fence %p submitted in SIGNALED state.  Fences must be reset before being submitted", fence);
            }
        }
    }
    // Update most recently submitted fence and fenceId for Queue
    queueMap[queue].lastSubmittedId = fenceId;
    return fenceId;
}

// Remove a fenceInfo from our list of fences/fenceIds
static void delete_fence_info(
    VkFence fence)
{
    fenceMap.erase(fence);
}

// Record information when a fence is known to be signalled
static void update_fence_tracking(
    VkFence fence)
{
    unordered_map<VkFence, MT_FENCE_INFO>::iterator fence_item = fenceMap.find(fence);
    if (fence_item != fenceMap.end()) {
        MT_FENCE_INFO *pCurFenceInfo = &(*fence_item).second;
        VkQueue queue = pCurFenceInfo->queue;
        unordered_map<VkQueue, MT_QUEUE_INFO>::iterator queue_item = queueMap.find(queue);
        if (queue_item != queueMap.end()) {
            MT_QUEUE_INFO *pQueueInfo = &(*queue_item).second;
            if (pQueueInfo->lastRetiredId < pCurFenceInfo->fenceId) {
                pQueueInfo->lastRetiredId = pCurFenceInfo->fenceId;
            }
        }
    }

    // Update fence state in fenceCreateInfo structure
    MT_OBJ_INFO* pObjectInfo = get_object_info(fence);
    if (pObjectInfo != NULL) {
        pObjectInfo->create_info.fence_create_info.flags =
            static_cast<VkFenceCreateFlags>(
                pObjectInfo->create_info.fence_create_info.flags | VK_FENCE_CREATE_SIGNALED_BIT);
    }
}

// Helper routine that updates the fence list for a specific queue to all-retired
static void retire_queue_fences(
    VkQueue queue)
{
    MT_QUEUE_INFO *pQueueInfo = &queueMap[queue];
    // Set queue's lastRetired to lastSubmitted indicating all fences completed
    pQueueInfo->lastRetiredId = pQueueInfo->lastSubmittedId;
}

// Helper routine that updates all queues to all-retired
static void retire_device_fences(
    VkDevice device)
{
    // Process each queue for device
    // TODO: Add multiple device support
    for (unordered_map<VkQueue, MT_QUEUE_INFO>::iterator ii=queueMap.begin(); ii!=queueMap.end(); ++ii) {
        // Set queue's lastRetired to lastSubmitted indicating all fences completed
        MT_QUEUE_INFO *pQueueInfo = &(*ii).second;
        pQueueInfo->lastRetiredId = pQueueInfo->lastSubmittedId;
    }
}

// Return ptr to info in map container containing mem, or NULL if not found
//  Calls to this function should be wrapped in mutex
static MT_MEM_OBJ_INFO* get_mem_obj_info(
    const VkDeviceMemory mem)
{
    unordered_map<VkDeviceMemory, MT_MEM_OBJ_INFO>::iterator item = memObjMap.find(mem);
    if (item != memObjMap.end()) {
        assert((*item).second.object != VK_NULL_HANDLE);
        return &(*item).second;
    } else {
        return NULL;
    }
}

static void add_mem_obj_info(
    const VkObject           object,
    const VkDeviceMemory     mem,
    const VkMemoryAllocInfo *pAllocInfo)
{
    assert(object != NULL);

    if (pAllocInfo) {  // MEM alloc created by vkCreateSwapChainWSI() doesn't have alloc info struct
        memcpy(&memObjMap[mem].allocInfo, pAllocInfo, sizeof(VkMemoryAllocInfo));
        // TODO:  Update for real hardware, actually process allocation info structures
        memObjMap[mem].allocInfo.pNext = NULL;
    } else {
        memset(&memObjMap[mem].allocInfo, 0, sizeof(VkMemoryAllocInfo));
    }
    memObjMap[mem].object = object;
    memObjMap[mem].refCount = 0;
    memObjMap[mem].mem = mem;
}

// Find CB Info and add mem reference to list container
// Find Mem Obj Info and add CB reference to list container
static bool32_t update_cmd_buf_and_mem_references(
    const VkCmdBuffer    cb,
    const VkDeviceMemory mem)
{
    bool32_t result = VK_TRUE;
    // First update CB binding in MemObj mini CB list
    MT_MEM_OBJ_INFO* pMemInfo = get_mem_obj_info(mem);
    if (!pMemInfo) {
        log_msg(mdd(cb), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cb, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                "Trying to bind mem obj %p to CB %p but no info for that mem obj.\n    "
                     "Was it correctly allocated? Did it already get freed?", mem, cb);
        result = VK_FALSE;
    } else {
        // Search for cmd buffer object in memory object's binding list
        bool32_t found  = VK_FALSE;
        if (pMemInfo->pCmdBufferBindings.size() > 0) {
            for (list<VkCmdBuffer>::iterator it = pMemInfo->pCmdBufferBindings.begin(); it != pMemInfo->pCmdBufferBindings.end(); ++it) {
                if ((*it) == cb) {
                    found = VK_TRUE;
                    break;
                }
            }
        }
        // If not present, add to list
        if (found == VK_FALSE) {
            pMemInfo->pCmdBufferBindings.push_front(cb);
            pMemInfo->refCount++;
        }

        // Now update CBInfo's Mem reference list
        MT_CB_INFO* pCBInfo = get_cmd_buf_info(cb);
        // TODO: keep track of all destroyed CBs so we know if this is a stale or simply invalid object
        if (!pCBInfo) {
            log_msg(mdd(cb), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cb, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                    "Trying to bind mem obj %p to CB %p but no info for that CB. Was CB incorrectly destroyed?", mem, cb);
            result = VK_FALSE;
        } else {
            // Search for memory object in cmd buffer's reference list
            bool32_t found  = VK_FALSE;
            if (pCBInfo->pMemObjList.size() > 0) {
                for (list<VkDeviceMemory>::iterator it = pCBInfo->pMemObjList.begin(); it != pCBInfo->pMemObjList.end(); ++it) {
                    if ((*it) == mem) {
                        found = VK_TRUE;
                        break;
                    }
                }
            }
            // If not present, add to list
            if (found == VK_FALSE) {
                pCBInfo->pMemObjList.push_front(mem);
            }
        }
    }
    return result;
}

// Clear the CB Binding for mem
// Calls to this function should be wrapped in mutex
static void remove_cmd_buf_and_mem_reference(
    const VkCmdBuffer    cb,
    const VkDeviceMemory mem)
{
    MT_MEM_OBJ_INFO* pInfo = get_mem_obj_info(mem);
    // TODO : Having this check is not ideal, really if memInfo was deleted,
    //   its CB bindings should be cleared and then clear_cmd_buf_and_mem_references wouldn't call
    //   us here with stale mem objs
    if (pInfo) {
        pInfo->pCmdBufferBindings.remove(cb);
        pInfo->refCount--;
    }
}

// Free bindings related to CB
static bool32_t clear_cmd_buf_and_mem_references(
    const VkCmdBuffer cb)
{
    bool32_t result = VK_TRUE;
    MT_CB_INFO* pCBInfo = get_cmd_buf_info(cb);
    if (!pCBInfo) {
        log_msg(mdd(cb), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cb, 0, MEMTRACK_INVALID_CB, "MEM",
                "Unable to find global CB info %p for deletion", cb);
        result = VK_FALSE;
    } else {
        if (pCBInfo->pMemObjList.size() > 0) {
            list<VkDeviceMemory> mem_obj_list = pCBInfo->pMemObjList;
            for (list<VkDeviceMemory>::iterator it=mem_obj_list.begin(); it!=mem_obj_list.end(); ++it) {
                remove_cmd_buf_and_mem_reference(cb, (*it));
            }
        }
        pCBInfo->pMemObjList.clear();
    }
    return result;
}

// Delete CBInfo from list along with all of it's mini MemObjInfo
//   and also clear mem references to CB
static bool32_t delete_cmd_buf_info(
    const VkCmdBuffer cb)
{
    bool32_t result = VK_TRUE;
    result = clear_cmd_buf_and_mem_references(cb);
    // Delete the CBInfo info
    if (result == VK_TRUE) {
        cbMap.erase(cb);
    }
    return result;
}

// Delete the entire CB list
static bool32_t delete_cmd_buf_info_list(
    void)
{
    for (unordered_map<VkCmdBuffer, MT_CB_INFO>::iterator ii=cbMap.begin(); ii!=cbMap.end(); ++ii) {
        clear_cmd_buf_and_mem_references((*ii).first);
    }
    cbMap.clear();
    return VK_TRUE;
}

// For given MemObjInfo, report Obj & CB bindings
static void reportMemReferencesAndCleanUp(
    MT_MEM_OBJ_INFO* pMemObjInfo)
{
    size_t cmdBufRefCount = pMemObjInfo->pCmdBufferBindings.size();
    size_t objRefCount    = pMemObjInfo->pObjBindings.size();

    if ((pMemObjInfo->pCmdBufferBindings.size() + pMemObjInfo->pObjBindings.size()) != 0) {
        log_msg(mdd(pMemObjInfo->object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, pMemObjInfo->mem, 0, MEMTRACK_INTERNAL_ERROR, "MEM",
                "Attempting to free memory object %p which still contains %lu references",
            pMemObjInfo->mem, (cmdBufRefCount + objRefCount));
    }

    if (cmdBufRefCount > 0 && pMemObjInfo->pCmdBufferBindings.size() > 0) {
        for (list<VkCmdBuffer>::const_iterator it = pMemObjInfo->pCmdBufferBindings.begin(); it != pMemObjInfo->pCmdBufferBindings.end(); ++it) {
            log_msg(mdd(pMemObjInfo->object), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (*it), 0, MEMTRACK_NONE, "MEM",
                    "Command Buffer %p still has a reference to mem obj %p", (*it), pMemObjInfo->mem);
        }
        // Clear the list of hanging references
        pMemObjInfo->pCmdBufferBindings.clear();
    }

    if (objRefCount > 0 && pMemObjInfo->pObjBindings.size() > 0) {
        for (list<VkObject>::const_iterator it = pMemObjInfo->pObjBindings.begin(); it != pMemObjInfo->pObjBindings.end(); ++it) {
            /* TODO: Would be nice to return the actual object type */
            log_msg(mdd(pMemObjInfo->object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, (*it), 0, MEMTRACK_NONE, "MEM",
                    "VK Object %p still has a reference to mem obj %p", (*it), pMemObjInfo->mem);

            // Found an object referencing this memory, remove its pointer to this memobj
            MT_OBJ_INFO *pObjInfo = get_object_info(*it);
            pObjInfo->pMemObjInfo = NULL;
        }
        // Clear the list of hanging references
        pMemObjInfo->pObjBindings.clear();
    }

}

static void deleteMemObjInfo(
    VkObject       object,
    VkDeviceMemory mem)
{
    unordered_map<VkDeviceMemory, MT_MEM_OBJ_INFO>::iterator item = memObjMap.find(mem);
    if (item != memObjMap.end()) {
        memObjMap.erase(item);
    }
    else {
        log_msg(mdd(object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                "Request to delete memory object %p not present in memory Object Map", mem);
    }
}

// Check if fence for given CB is completed
static bool32_t checkCBCompleted(
    const VkCmdBuffer cb)
{
    bool32_t result = VK_TRUE;
    MT_CB_INFO* pCBInfo = get_cmd_buf_info(cb);
    if (!pCBInfo) {
        log_msg(mdd(cb), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cb, 0, MEMTRACK_INVALID_CB, "MEM",
                "Unable to find global CB info %p to check for completion", cb);
        result = VK_FALSE;
    } else if (pCBInfo->lastSubmittedQueue != NULL) {
        VkQueue queue = pCBInfo->lastSubmittedQueue;
        MT_QUEUE_INFO *pQueueInfo = &queueMap[queue];
        if (pCBInfo->fenceId > pQueueInfo->lastRetiredId) {
            log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cb, 0, MEMTRACK_NONE, "MEM",
                    "fence %p for CB %p has not been checked for completion",
                    (void*)pCBInfo->lastSubmittedFence, cb);
            result = VK_FALSE;
        }
    }
    return result;
}

static bool32_t freeMemObjInfo(
    VkObject       object,
    VkDeviceMemory mem,
    bool           internal)
{
    bool32_t result = VK_TRUE;
    // Parse global list to find info w/ mem
    MT_MEM_OBJ_INFO* pInfo = get_mem_obj_info(mem);
    if (!pInfo) {
        log_msg(mdd(object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                "Couldn't find mem info object for %p\n    Was %p never allocated or previously freed?",
                (void*)mem, (void*)mem);
        result = VK_FALSE;
    } else {
        if (pInfo->allocInfo.allocationSize == 0 && !internal) {
            log_msg(mdd(pInfo->object), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                    "Attempting to free memory associated with a Persistent Image, %p, "
                    "this should not be explicitly freed\n", (void*)mem);
            result = VK_FALSE;
        } else {
            // Clear any CB bindings for completed CBs
            //   TODO : Is there a better place to do this?

            assert(pInfo->object != VK_NULL_HANDLE);
            list<VkCmdBuffer>::iterator it = pInfo->pCmdBufferBindings.begin();
            list<VkCmdBuffer>::iterator temp;
            while (pInfo->pCmdBufferBindings.size() > 0 && it != pInfo->pCmdBufferBindings.end()) {
                if (VK_TRUE == checkCBCompleted(*it)) {
                    temp = it;
                    ++temp;
                    clear_cmd_buf_and_mem_references(*it);
                    it = temp;
                } else {
                    ++it;
                }
            }

            // Now verify that no references to this mem obj remain
            if (0 != pInfo->refCount) {
                reportMemReferencesAndCleanUp(pInfo);
                result = VK_FALSE;
            }
            // Delete mem obj info
            deleteMemObjInfo(object, mem);
        }
    }
    return result;
}

// Remove object binding performs 3 tasks:
// 1. Remove ObjectInfo from MemObjInfo list container of obj bindings & free it
// 2. Decrement refCount for MemObjInfo
// 3. Clear MemObjInfo ptr from ObjectInfo
static bool32_t clear_object_binding(
    VkObject object)
{
    bool32_t result = VK_FALSE;
    MT_OBJ_INFO* pObjInfo = get_object_info(object);
    if (pObjInfo) {
        if (!pObjInfo->pMemObjInfo || pObjInfo->pMemObjInfo->pObjBindings.size() <= 0) {
            log_msg(mdd(object), VK_DBG_REPORT_WARN_BIT, (VkObjectType) 0, object, 0, MEMTRACK_MEM_OBJ_CLEAR_EMPTY_BINDINGS, "MEM",
                    "Attempting to clear mem binding on obj %p but it has no binding.", (void*)object);
        } else {
            // This obj is bound to a memory object. Remove the reference to this object in that memory object's list, decrement the memObj's refcount
            // and set the objects memory binding pointer to NULL.
            for (list<VkObject>::iterator it = pObjInfo->pMemObjInfo->pObjBindings.begin(); it != pObjInfo->pMemObjInfo->pObjBindings.end(); ++it) {
                if ((*it) == object) {
                    pObjInfo->pMemObjInfo->refCount--;
                    pObjInfo->pMemObjInfo->pObjBindings.erase(it);
                    pObjInfo->pMemObjInfo = NULL;
                    result = VK_TRUE;
                    break;
                }
            }
            if (result == VK_FALSE) {
                log_msg(mdd(object), VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, object, 0, MEMTRACK_INTERNAL_ERROR, "MEM",
                        "While trying to clear mem binding for object %p, unable to find that object referenced by mem obj %p",
                        object, pObjInfo->pMemObjInfo->mem);
            }
        }
    }
    return result;
}

// For NULL mem case, output warning
// Make sure given object is in global object map
//  IF a previous binding existed, output validation error
//  Otherwise, add reference from objectInfo to memoryInfo
//  Add reference off of objInfo
//  device is required for error logging, need a dispatchable
//  object for that.
// Return VK_TRUE if addition is successful, VK_FALSE otherwise
static bool32_t set_object_binding(
    VkObject       dispatch_object,
    VkObject       object,
    VkDeviceMemory mem)
{
    bool32_t result = VK_FALSE;
    // Handle NULL case separately, just clear previous binding & decrement reference
    if (mem == VK_NULL_HANDLE) {
        log_msg(mdd(dispatch_object), VK_DBG_REPORT_WARN_BIT, (VkObjectType) 0, object, 0, MEMTRACK_INTERNAL_ERROR, "MEM",
                "Attempting to Bind Obj(%p) to NULL", (void*)object);
        return VK_TRUE;
    } else {
        MT_OBJ_INFO* pObjInfo = get_object_info(object);
        if (!pObjInfo) {
            log_msg(mdd(dispatch_object), VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, object, 0, MEMTRACK_INTERNAL_ERROR, "MEM",
                    "Attempting to update Binding of Obj(%p) that's not in global list()", (void*)object);
            return VK_FALSE;
        }
        // non-null case so should have real mem obj
        MT_MEM_OBJ_INFO* pInfo = get_mem_obj_info(mem);
        if (!pInfo) {
            log_msg(mdd(dispatch_object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                    "While trying to bind mem for obj %p, couldn't find info for mem obj %p", (void*)object, (void*)mem);
            return VK_FALSE;
        } else {
            if (pObjInfo->pMemObjInfo != NULL) {
                log_msg(mdd(dispatch_object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem, 0, MEMTRACK_REBIND_OBJECT, "MEM",
                        "Attempting to bind memory (%p) to object (%p) which has already been bound to mem object %p",
                        (void*)mem, (void*)object, (void*)pObjInfo->pMemObjInfo->mem);
                return VK_FALSE;
            }
            else {
                pInfo->pObjBindings.push_front(object);
                pInfo->refCount++;

                // For image objects, make sure default memory state is correctly set
                // TODO : What's the best/correct way to handle this?
                if (VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO == pObjInfo->sType) {
                    if (pObjInfo->create_info.image_create_info.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                                         VK_IMAGE_USAGE_DEPTH_STENCIL_BIT)) {
                        // TODO::  More memory state transition stuff.
                    }
                }
                pObjInfo->pMemObjInfo = pInfo;
            }
        }
    }
    return VK_TRUE;
}

// For NULL mem case, clear any previous binding Else...
// Make sure given object is in global object map
//  IF a previous binding existed, update binding
//  Add reference from objectInfo to memoryInfo
//  Add reference off of objInfo
// Return VK_TRUE if addition is successful, VK_FALSE otherwise
static bool32_t set_sparse_buffer_binding(
    VkObject       object,
    VkDeviceMemory mem)
{
    bool32_t result = VK_FALSE;
    // Handle NULL case separately, just clear previous binding & decrement reference
    if (mem == VK_NULL_HANDLE) {
        clear_object_binding(object);
        return VK_TRUE;
    } else {
        MT_OBJ_INFO* pObjInfo = get_object_info(object);
        if (!pObjInfo) {
            log_msg(mdd(object), VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, object, 0, MEMTRACK_INTERNAL_ERROR, "MEM",
                    "Attempting to update Binding of Obj(%p) that's not in global list()", (void*)object);
            return VK_FALSE;
        }
        // non-null case so should have real mem obj
        MT_MEM_OBJ_INFO* pInfo = get_mem_obj_info(mem);
        if (!pInfo) {
            log_msg(mdd(object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                    "While trying to bind mem for obj %p, couldn't find info for mem obj %p", (void*)object, (void*)mem);
            return VK_FALSE;
        } else {
            // Search for object in memory object's binding list
            bool32_t found  = VK_FALSE;
            if (pInfo->pObjBindings.size() > 0) {
                for (list<VkObject>::iterator it = pInfo->pObjBindings.begin(); it != pInfo->pObjBindings.end(); ++it) {
                    if ((*it) == object) {
                        found = VK_TRUE;
                        break;
                    }
                }
            }
            // If not present, add to list
            if (found == VK_FALSE) {
                pInfo->pObjBindings.push_front(object);
                pInfo->refCount++;
            }

            if (pObjInfo->pMemObjInfo) {
                clear_object_binding(object); // Need to clear the previous object binding before setting new binding
                log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, object, 0, MEMTRACK_NONE, "MEM",
                        "Updating memory binding for object %p from mem obj %p to %p", object, pObjInfo->pMemObjInfo->mem, mem);
            }
            pObjInfo->pMemObjInfo = pInfo;
        }
    }
    return VK_TRUE;
}

// Print details of global Obj tracking list
static void print_object_list(
    VkObject object)
{
    MT_OBJ_INFO* pInfo = NULL;
    log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
            "Details of Object list of size %lu elements", objectMap.size());
    if (objectMap.size() <= 0)
        return;
    for (unordered_map<VkObject, MT_OBJ_INFO>::iterator ii=objectMap.begin(); ii!=objectMap.end(); ++ii) {
        pInfo = &(*ii).second;
        log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, pInfo->object, 0, MEMTRACK_NONE, "MEM",
                "    ObjInfo %p has object %p, pMemObjInfo %p", pInfo, pInfo->object, pInfo->pMemObjInfo);
    }
}

// For given Object, get 'mem' obj that it's bound to or NULL if no binding
static VkDeviceMemory get_mem_binding_from_object(
    const VkObject dispObj, const VkObject object)
{
    VkDeviceMemory mem = NULL;
    MT_OBJ_INFO* pObjInfo = get_object_info(object);
    if (pObjInfo) {
        if (pObjInfo->pMemObjInfo) {
            mem = pObjInfo->pMemObjInfo->mem;
        } else {
            log_msg(mdd(dispObj), VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, object, 0, MEMTRACK_MISSING_MEM_BINDINGS, "MEM",
                    "Trying to get mem binding for object %p but object has no mem binding", (void*)object);
            print_object_list(object);
        }
    } else {
        log_msg(mdd(dispObj), VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, object, 0, MEMTRACK_INVALID_OBJECT, "MEM",
                "Trying to get mem binding for object %p but no such object in global list", (void*)object);
        print_object_list(object);
    }
    return mem;
}

// Print details of MemObjInfo list
static void print_mem_list(
    VkObject object)
{
    MT_MEM_OBJ_INFO* pInfo = NULL;
    // Just printing each msg individually for now, may want to package these into single large print
    log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
            "MEM INFO : Details of Memory Object list of size %lu elements", memObjMap.size());

    if (memObjMap.size() <= 0)
        return;

    for (unordered_map<VkDeviceMemory, MT_MEM_OBJ_INFO>::iterator ii=memObjMap.begin(); ii!=memObjMap.end(); ++ii) {
        pInfo = &(*ii).second;

        log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
                "    ===MemObjInfo at %p===", (void*)pInfo);
        log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
                "    Mem object: %p", (void*)pInfo->mem);
        log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
                "    Ref Count: %u", pInfo->refCount);
        if (0 != pInfo->allocInfo.allocationSize) {
            string pAllocInfoMsg = vk_print_vkmemoryallocinfo(&pInfo->allocInfo, "{MEM}INFO :       ");
            log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
                    "    Mem Alloc info:\n%s", pAllocInfoMsg.c_str());
        } else {
            log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
                    "    Mem Alloc info is NULL (alloc done by vkCreateSwapChainWSI())");
        }

        log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
                "    VK OBJECT Binding list of size %lu elements:", pInfo->pObjBindings.size());
        if (pInfo->pObjBindings.size() > 0) {
            for (list<VkObject>::iterator it = pInfo->pObjBindings.begin(); it != pInfo->pObjBindings.end(); ++it) {
                log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
                        "       VK OBJECT %p", (*it));
            }
        }

        log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
                "    VK Command Buffer (CB) binding list of size %lu elements", pInfo->pCmdBufferBindings.size());
        if (pInfo->pCmdBufferBindings.size() > 0)
        {
            for (list<VkCmdBuffer>::iterator it = pInfo->pCmdBufferBindings.begin(); it != pInfo->pCmdBufferBindings.end(); ++it) {
                log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
                        "      VK CB %p", (*it));
            }
        }
    }
}

static void printCBList(
    VkObject object)
{
    MT_CB_INFO* pCBInfo = NULL;
    log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
            "Details of CB list of size %lu elements", cbMap.size());

    if (cbMap.size() <= 0)
        return;

    for (unordered_map<VkCmdBuffer, MT_CB_INFO>::iterator ii=cbMap.begin(); ii!=cbMap.end(); ++ii) {
        pCBInfo = &(*ii).second;

        log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
                "    CB Info (%p) has CB %p, fenceId %" PRIx64", and fence %p",
                (void*)pCBInfo, (void*)pCBInfo->cmdBuffer, pCBInfo->fenceId,
                (void*)pCBInfo->lastSubmittedFence);

        if (pCBInfo->pMemObjList.size() <= 0)
            continue;
        for (list<VkDeviceMemory>::iterator it = pCBInfo->pMemObjList.begin(); it != pCBInfo->pMemObjList.end(); ++it) {
            log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
                    "      Mem obj %p", (*it));
        }
    }
}

static void init_mem_tracker(
    layer_data *my_data)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    // initialize MemTracker options
    report_flags = getLayerOptionFlags("MemTrackerReportFlags", 0);
    getLayerOptionEnum("MemTrackerDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        option_str = getLayerOption("MemTrackerLogFilename");
        if (option_str) {
            log_output = fopen(option_str, "w");
        }
        if (log_output == NULL) {
            log_output = stdout;
        }

        layer_create_msg_callback(my_data->report_data, report_flags, log_callback, (void *) log_output, &my_data->logging_callback);
    }

    if (!globalLockInitialized)
    {
        // TODO/TBD: Need to delete this mutex sometime.  How???  One
        // suggestion is to call this during vkCreateInstance(), and then we
        // can clean it up during vkDestroyInstance().  However, that requires
        // that the layer have per-instance locks.  We need to come back and
        // address this soon.
        loader_platform_thread_create_mutex(&globalLock);
        globalLockInitialized = 1;
    }
}

// hook DestroyInstance to remove tableInstanceMap entry
VK_LAYER_EXPORT VkResult VKAPI vkDestroyInstance(VkInstance instance)
{
    // Grab the key before the instance is destroyed.
    dispatch_key key = get_dispatch_key(instance);
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(mem_tracker_instance_table_map, instance);
    VkResult res = pTable->DestroyInstance(instance);

    // Clean up logging callback, if any
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    if (my_data->logging_callback) {
        layer_destroy_msg_callback(my_data->report_data, my_data->logging_callback);
    }

    layer_debug_report_destroy_instance(mid(instance));
    layer_data_map.erase(pTable);

    mem_tracker_instance_table_map.erase(key);
    assert(mem_tracker_instance_table_map.size() == 0 && "Should not have any instance mappings hanging around");
    return res;
}

VkResult VKAPI vkCreateInstance(
    const VkInstanceCreateInfo*                 pCreateInfo,
    VkInstance*                                 pInstance)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(mem_tracker_instance_table_map, *pInstance);
    VkResult result = pTable->CreateInstance(pCreateInfo, pInstance);

    if (result == VK_SUCCESS) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        my_data->report_data = debug_report_create_instance(
                                   pTable,
                                   *pInstance,
                                   pCreateInfo->extensionCount,
                                   pCreateInfo->pEnabledExtensions);

        init_mem_tracker(my_data);
    }
    return result;
}

static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    uint32_t i, ext_idx;
    VkLayerDispatchTable *pDisp  = get_dispatch_table(mem_tracker_device_table_map, device);
    deviceExtMap[pDisp].wsi_lunarg_enabled = false;
    for (i = 0; i < pCreateInfo->extensionCount; i++) {
        if (strcmp(pCreateInfo->pEnabledExtensions[i].name, VK_WSI_LUNARG_EXTENSION_NAME) == 0)
            deviceExtMap[pDisp].wsi_lunarg_enabled = true;

    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(
    VkPhysicalDevice          gpu,
    const VkDeviceCreateInfo *pCreateInfo,
    VkDevice                 *pDevice)
{
    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(mem_tracker_instance_table_map, gpu);
    VkResult result = pInstanceTable->CreateDevice(gpu, pCreateInfo, pDevice);
    if (result == VK_SUCCESS) {
        layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
        layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
        my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
        createDeviceRegisterExtensions(pCreateInfo, *pDevice);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyDevice(
    VkDevice device)
{
    loader_platform_thread_lock_mutex(&globalLock);
    log_msg(mdd(device), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE, device, 0, MEMTRACK_NONE, "MEM",
            "Printing List details prior to vkDestroyDevice()");
    print_mem_list(device);
    printCBList(device);
    print_object_list(device);
    if (VK_FALSE == delete_cmd_buf_info_list()) {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, device, 0, MEMTRACK_INTERNAL_ERROR, "MEM",
                "Issue deleting global CB list in vkDestroyDevice()");
    }
    // Report any memory leaks
    MT_MEM_OBJ_INFO* pInfo = NULL;
    if (memObjMap.size() > 0) {
        for (unordered_map<VkDeviceMemory, MT_MEM_OBJ_INFO>::iterator ii=memObjMap.begin(); ii!=memObjMap.end(); ++ii) {
            pInfo = &(*ii).second;

            if (pInfo->allocInfo.allocationSize != 0) {
                log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, pInfo->mem, 0, MEMTRACK_MEMORY_LEAK, "MEM",
                        "Mem Object %p has not been freed. You should clean up this memory by calling "
                         "vkFreeMemory(%p) prior to vkDestroyDevice().", pInfo->mem, pInfo->mem);
            }
        }
    }

    // Queues persist until device is destroyed
    delete_queue_info_list();

    layer_debug_report_destroy_device(device);

    loader_platform_thread_unlock_mutex(&globalLock);

    dispatch_key key = get_dispatch_key(device);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "Device: %p, key: %p\n", device, key);
#endif
    VkLayerDispatchTable *pDisp  = get_dispatch_table(mem_tracker_device_table_map, device);
    VkResult result = pDisp->DestroyDevice(device);
    deviceExtMap.erase(pDisp);
    mem_tracker_device_table_map.erase(key);
    assert(mem_tracker_device_table_map.size() == 0 && "Should not have any instance mappings hanging around");

    return result;
}

struct extProps {
    uint32_t version;
    const char * const name;
};
#define MEM_TRACKER_LAYER_EXT_ARRAY_SIZE 2
static const VkExtensionProperties mtExts[MEM_TRACKER_LAYER_EXT_ARRAY_SIZE] = {
    {
        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,
        "MemTracker",
        0x10,
        "Validation layer: MemTracker",
    },
    {
        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,
        "Validation",
        0x10,
        "Validation layer: MemTracker",
    }
};

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionInfo(
    VkExtensionInfoType  infoType,
    uint32_t             extensionIndex,
    size_t              *pDataSize,
    void                *pData)
{
    // This entrypoint is NOT going to init its own dispatch table since loader calls here early
    uint32_t *count;

    if (pDataSize == NULL) {
        return VK_ERROR_INVALID_POINTER;
    }

    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL) {
                return VK_SUCCESS;
            }
            count = (uint32_t *) pData;
            *count = MEM_TRACKER_LAYER_EXT_ARRAY_SIZE;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL) {
                return VK_SUCCESS;
            }
            if (extensionIndex >= MEM_TRACKER_LAYER_EXT_ARRAY_SIZE) {
                return VK_ERROR_INVALID_VALUE;
            }
            memcpy((VkExtensionProperties *) pData, &mtExts[extensionIndex], sizeof(VkExtensionProperties));
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}

#define MEM_TRACKER_LAYER_DEV_EXT_ARRAY_SIZE 3
static const VkExtensionProperties mtDevExts[MEM_TRACKER_LAYER_DEV_EXT_ARRAY_SIZE] = {
    {
        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,
        "MemTracker",
        0x10,
        "Validation layer: MemTracker",
    },
    {
        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,
        "Validation",
        0x10,
        "Validation layer: MemTracker",
    },
    {
        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,
        VK_WSI_LUNARG_EXTENSION_NAME,
        0x10,
        "Validation layer: MemTracker",
    }
};

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceExtensionInfo(
    VkPhysicalDevice     physical_device,
    VkExtensionInfoType  infoType,
    uint32_t             extensionIndex,
    size_t              *pDataSize,
    void                *pData)
{
    uint32_t *count;

    if (pDataSize == NULL) {
        return VK_ERROR_INVALID_POINTER;
    }

    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL) {
                return VK_SUCCESS;
            }
            count = (uint32_t *) pData;
            *count = MEM_TRACKER_LAYER_DEV_EXT_ARRAY_SIZE;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL) {
                return VK_SUCCESS;
            }
            if (extensionIndex >= MEM_TRACKER_LAYER_DEV_EXT_ARRAY_SIZE) {
                return VK_ERROR_INVALID_VALUE;
            }
            memcpy((VkExtensionProperties *) pData, &mtDevExts[extensionIndex], sizeof(VkExtensionProperties));
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    }

    return VK_SUCCESS;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetDeviceQueue(
    VkDevice  device,
    uint32_t  queueNodeIndex,
    uint32_t  queueIndex,
    VkQueue   *pQueue)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_queue_info(*pQueue);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueSubmit(
    VkQueue             queue,
    uint32_t            cmdBufferCount,
    const VkCmdBuffer  *pCmdBuffers,
    VkFence             fence)
{
    loader_platform_thread_lock_mutex(&globalLock);
    // TODO : Need to track fence and clear mem references when fence clears
    MT_CB_INFO* pCBInfo = NULL;
    uint64_t    fenceId = add_fence_info(fence, queue);

    print_mem_list(queue);
    printCBList(queue);
    for (uint32_t i = 0; i < cmdBufferCount; i++) {
        pCBInfo = get_cmd_buf_info(pCmdBuffers[i]);
        pCBInfo->fenceId = fenceId;
        pCBInfo->lastSubmittedFence = fence;
        pCBInfo->lastSubmittedQueue = queue;
    }

    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, queue)->QueueSubmit(
        queue, cmdBufferCount, pCmdBuffers, fence);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocMemory(
    VkDevice                 device,
    const VkMemoryAllocInfo *pAllocInfo,
    VkDeviceMemory          *pMem)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->AllocMemory(device, pAllocInfo, pMem);
    // TODO : Track allocations and overall size here
    loader_platform_thread_lock_mutex(&globalLock);
    add_mem_obj_info(device, *pMem, pAllocInfo);
    print_mem_list(device);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkFreeMemory(
    VkDevice       device,
    VkDeviceMemory mem)
{
    /* From spec : A memory object is freed by calling vkFreeMemory() when it is no longer needed. Before
     * freeing a memory object, an application must ensure the memory object is unbound from
     * all API objects referencing it and that it is not referenced by any queued command buffers
     */
    loader_platform_thread_lock_mutex(&globalLock);
    bool32_t noerror = freeMemObjInfo(device, mem, false);
    print_mem_list(device);
    print_object_list(device);
    printCBList(device);
    // Output an warning message for proper error/warning handling
    if (noerror == VK_FALSE) {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem, 0, MEMTRACK_FREED_MEM_REF, "MEM",
                "Freeing memory object while it still has references: mem obj %p", (void*)mem);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->FreeMemory(device, mem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkMapMemory(
    VkDevice         device,
    VkDeviceMemory   mem,
    VkDeviceSize     offset,
    VkDeviceSize     size,
    VkFlags          flags,
    void           **ppData)
{
    // TODO : Track when memory is mapped
    loader_platform_thread_lock_mutex(&globalLock);
    MT_MEM_OBJ_INFO *pMemObj = get_mem_obj_info(mem);
    if ((pMemObj->allocInfo.memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem, 0, MEMTRACK_INVALID_STATE, "MEM",
                "Mapping Memory without VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT set: mem obj %p", (void*)mem);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->MapMemory(device, mem, offset, size, flags, ppData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkUnmapMemory(
    VkDevice       device,
    VkDeviceMemory mem)
{
    // TODO : Track as memory gets unmapped, do we want to check what changed following map?
    //   Make sure that memory was ever mapped to begin with
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->UnmapMemory(device, mem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenSharedMemory(
    VkDevice                device,
    const VkMemoryOpenInfo *pOpenInfo,
    VkDeviceMemory         *pMem)
{
    // TODO : Track this
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->OpenSharedMemory(device, pOpenInfo, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenPeerMemory(
    VkDevice                    device,
    const VkPeerMemoryOpenInfo *pOpenInfo,
    VkDeviceMemory             *pMem)
{
    // TODO : Track this
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->OpenPeerMemory(device, pOpenInfo, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenPeerImage(
    VkDevice                   device,
    const VkPeerImageOpenInfo *pOpenInfo,
    VkImage                   *pImage,
    VkDeviceMemory            *pMem)
{
    // TODO : Track this
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->OpenPeerImage(device, pOpenInfo, pImage, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyObject(
    VkDevice     device,
    VkObjectType objType,
    VkObject     object)
{
    unordered_map<VkObject, MT_OBJ_INFO>::iterator item;
    loader_platform_thread_lock_mutex(&globalLock);

    // First check if this is a CmdBuffer or fence
    switch (objType) {
    case VK_OBJECT_TYPE_COMMAND_BUFFER:
        delete_cmd_buf_info((VkCmdBuffer)object);
        break;
    case VK_OBJECT_TYPE_FENCE:
        delete_fence_info((VkFence)object);
        break;
    default:
        break;
    }

    if ((item = objectMap.find(object)) != objectMap.end()) {
        MT_OBJ_INFO* pDelInfo = &(*item).second;
        if (pDelInfo->pMemObjInfo) {
            // Wsi allocated Memory is tied to image object so clear the binding and free that memory automatically
            if (0 == pDelInfo->pMemObjInfo->allocInfo.allocationSize) { // Wsi allocated memory has NULL allocInfo w/ 0 size
                VkDeviceMemory memToFree = pDelInfo->pMemObjInfo->mem;
                clear_object_binding(object);
                freeMemObjInfo(device, memToFree, true);
            }
            else {
                // Remove this object from memory object's reference list and decrement its ref counter
                clear_object_binding(object);
            }
        }
        objectMap.erase(item);
    }

    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyObject(device, objType, object);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetObjectInfo(
    VkDevice          device,
    VkObjectType      objType,
    VkObject          object,
    VkObjectInfoType  infoType,
    size_t           *pDataSize,
    void             *pData)
{
    // TODO : What to track here?
    //   Could potentially save returned mem requirements and validate values passed into BindObjectMemory for this object
    // From spec : The only objects that are guaranteed to have no external memory requirements are devices, queues,
    //             command buffers, shaders and memory objects.
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->GetObjectInfo(device, objType, object, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBindObjectMemory(
    VkDevice       device,
    VkObjectType   objType,
    VkObject       object,
    VkDeviceMemory mem,
    VkDeviceSize   offset)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->BindObjectMemory(device, objType, object, mem, offset);
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    set_object_binding(device, object, mem);
    print_object_list(device);
    print_mem_list(device);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindSparseBufferMemory(
    VkQueue        queue,
    VkBuffer       buffer,
    VkDeviceSize   rangeOffset,
    VkDeviceSize   rangeSize,
    VkDeviceMemory mem,
    VkDeviceSize   memOffset)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, queue)->QueueBindSparseBufferMemory(
        queue, buffer, rangeOffset, rangeSize, mem, memOffset);
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    if (VK_FALSE == set_sparse_buffer_binding(buffer, mem)) {
        log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_BUFFER, buffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "Unable to set object %p binding to mem obj %p", (void*)buffer, (void*)mem);
    }
    print_object_list(queue);
    print_mem_list(queue);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateFence(
    VkDevice                 device,
    const VkFenceCreateInfo *pCreateInfo,
    VkFence                 *pFence)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateFence(device, pCreateInfo, pFence);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pFence, pCreateInfo->sType, pCreateInfo, sizeof(VkFenceCreateInfo), "fence");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetFences(
    VkDevice  device,
    uint32_t  fenceCount,
    VkFence  *pFences)
{
    /*
     * TODO: Shouldn't we check for error conditions before passing down the chain?
     * What if reason result is not VK_SUCCESS is something we could report as a validation error?
     */
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->ResetFences(device, fenceCount, pFences);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        // Reset fence state in fenceCreateInfo structure
        for (uint32_t i = 0; i < fenceCount; i++) {
            MT_OBJ_INFO* pObjectInfo = get_object_info(pFences[i]);
            if (pObjectInfo != NULL) {
                // Validate fences in SIGNALED state
                if (!(pObjectInfo->create_info.fence_create_info.flags & VK_FENCE_CREATE_SIGNALED_BIT)) {
                    log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_FENCE, pFences[i], 0, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                            "Fence %p submitted to VkResetFences in UNSIGNALED STATE", pFences[i]);
                    result = VK_ERROR_INVALID_VALUE;
                }
                else {
                    pObjectInfo->create_info.fence_create_info.flags =
                        static_cast<VkFenceCreateFlags>(pObjectInfo->create_info.fence_create_info.flags & ~VK_FENCE_CREATE_SIGNALED_BIT);
                }
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetFenceStatus(
    VkDevice device,
    VkFence  fence)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->GetFenceStatus(device, fence);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        update_fence_tracking(fence);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkWaitForFences(
    VkDevice       device,
    uint32_t       fenceCount,
    const VkFence *pFences,
    bool32_t       waitAll,
    uint64_t       timeout)
{
    // Verify fence status of submitted fences
    for(uint32_t i = 0; i < fenceCount; i++) {
        MT_OBJ_INFO* pObjectInfo = get_object_info(pFences[i]);
        if (pObjectInfo != NULL) {
            if (pObjectInfo->create_info.fence_create_info.flags & VK_FENCE_CREATE_SIGNALED_BIT) {
                log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_FENCE, pFences[i], 0, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                        "VkWaitForFences specified fence %p already in SIGNALED state.", pFences[i]);
            }
        }
    }

    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    loader_platform_thread_lock_mutex(&globalLock);

    if (VK_SUCCESS == result) {
        if (waitAll || fenceCount == 1) { // Clear all the fences
            for(uint32_t i = 0; i < fenceCount; i++) {
                update_fence_tracking(pFences[i]);
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueWaitIdle(
    VkQueue queue)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, queue)->QueueWaitIdle(queue);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        retire_queue_fences(queue);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDeviceWaitIdle(
    VkDevice device)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DeviceWaitIdle(device);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        retire_device_fences(device);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateEvent(
    VkDevice                 device,
    const VkEventCreateInfo *pCreateInfo,
    VkEvent                 *pEvent)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateEvent(device, pCreateInfo, pEvent);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pEvent, pCreateInfo->sType, pCreateInfo, sizeof(VkEventCreateInfo), "event");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateQueryPool(
    VkDevice                     device,
    const VkQueryPoolCreateInfo *pCreateInfo,
    VkQueryPool                 *pQueryPool)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateQueryPool(device, pCreateInfo, pQueryPool);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pQueryPool, pCreateInfo->sType, pCreateInfo, sizeof(VkQueryPoolCreateInfo), "query_pool");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBuffer(
    VkDevice                  device,
    const VkBufferCreateInfo *pCreateInfo,
    VkBuffer                 *pBuffer)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateBuffer(device, pCreateInfo, pBuffer);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pBuffer, pCreateInfo->sType, pCreateInfo, sizeof(VkBufferCreateInfo), "buffer");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBufferView(
    VkDevice                      device,
    const VkBufferViewCreateInfo *pCreateInfo,
    VkBufferView                 *pView)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateBufferView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pView, pCreateInfo->sType, pCreateInfo, sizeof(VkBufferViewCreateInfo), "buffer_view");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImage(
    VkDevice                 device,
    const VkImageCreateInfo *pCreateInfo,
    VkImage                 *pImage)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateImage(device, pCreateInfo, pImage);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pImage, pCreateInfo->sType, pCreateInfo, sizeof(VkImageCreateInfo), "image");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImageView(
    VkDevice                     device,
    const VkImageViewCreateInfo *pCreateInfo,
    VkImageView                 *pView)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateImageView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pView, pCreateInfo->sType, pCreateInfo, sizeof(VkImageViewCreateInfo), "image_view");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateColorAttachmentView(
    VkDevice                               device,
    const VkColorAttachmentViewCreateInfo *pCreateInfo,
    VkColorAttachmentView                 *pView)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateColorAttachmentView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pView, pCreateInfo->sType, pCreateInfo, sizeof(VkColorAttachmentViewCreateInfo), "color_attachment_view");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDepthStencilView(
    VkDevice                            device,
    const VkDepthStencilViewCreateInfo *pCreateInfo,
    VkDepthStencilView                 *pView)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateDepthStencilView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pView, pCreateInfo->sType, pCreateInfo, sizeof(VkDepthStencilViewCreateInfo), "ds_view");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateShader(
    VkDevice                  device,
    const VkShaderCreateInfo *pCreateInfo,
    VkShader                 *pShader)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateShader(device, pCreateInfo, pShader);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipeline(
    VkDevice                            device,
    const VkGraphicsPipelineCreateInfo *pCreateInfo,
    VkPipeline                         *pPipeline)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pPipeline, pCreateInfo->sType, pCreateInfo, sizeof(VkGraphicsPipelineCreateInfo), "graphics_pipeline");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipelineDerivative(
    VkDevice                            device,
    const VkGraphicsPipelineCreateInfo *pCreateInfo,
    VkPipeline                          basePipeline,
    VkPipeline                         *pPipeline)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateGraphicsPipelineDerivative(
        device, pCreateInfo, basePipeline, pPipeline);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pPipeline, pCreateInfo->sType, pCreateInfo, sizeof(VkGraphicsPipelineCreateInfo), "graphics_pipeline");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateComputePipeline(
    VkDevice                           device,
    const VkComputePipelineCreateInfo *pCreateInfo,
    VkPipeline                        *pPipeline)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateComputePipeline(device, pCreateInfo, pPipeline);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pPipeline, pCreateInfo->sType, pCreateInfo, sizeof(VkComputePipelineCreateInfo), "compute_pipeline");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSampler(
    VkDevice                   device,
    const VkSamplerCreateInfo *pCreateInfo,
    VkSampler                 *pSampler)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateSampler(device, pCreateInfo, pSampler);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pSampler, pCreateInfo->sType, pCreateInfo, sizeof(VkSamplerCreateInfo), "sampler");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicViewportState(
    VkDevice                          device,
    const VkDynamicVpStateCreateInfo *pCreateInfo,
    VkDynamicVpState                 *pState)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateDynamicViewportState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pState, pCreateInfo->sType, pCreateInfo, sizeof(VkDynamicVpStateCreateInfo), "viewport_state");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicRasterState(
    VkDevice                          device,
    const VkDynamicRsStateCreateInfo *pCreateInfo,
    VkDynamicRsState                 *pState)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateDynamicRasterState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pState, pCreateInfo->sType, pCreateInfo, sizeof(VkDynamicRsStateCreateInfo), "raster_state");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicColorBlendState(
    VkDevice                          device,
    const VkDynamicCbStateCreateInfo *pCreateInfo,
    VkDynamicCbState                 *pState)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateDynamicColorBlendState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pState, pCreateInfo->sType, pCreateInfo, sizeof(VkDynamicCbStateCreateInfo), "cb_state");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicDepthStencilState(
    VkDevice                          device,
    const VkDynamicDsStateCreateInfo *pCreateInfo,
    VkDynamicDsState                 *pState)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateDynamicDepthStencilState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_info(*pState, pCreateInfo->sType, pCreateInfo, sizeof(VkDynamicDsStateCreateInfo), "ds_state");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateCommandBuffer(
    VkDevice                     device,
    const VkCmdBufferCreateInfo *pCreateInfo,
    VkCmdBuffer                 *pCmdBuffer)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    // At time of cmd buffer creation, create global cmd buffer info for the returned cmd buffer
    loader_platform_thread_lock_mutex(&globalLock);
    if (*pCmdBuffer)
        add_cmd_buf_info(*pCmdBuffer);
    printCBList(device);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBeginCommandBuffer(
    VkCmdBuffer                 cmdBuffer,
    const VkCmdBufferBeginInfo *pBeginInfo)
{
    loader_platform_thread_lock_mutex(&globalLock);
    // This implicitly resets the Cmd Buffer so make sure any fence is done and then clear memory references
    if (!checkCBCompleted(cmdBuffer)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM",
                "Calling vkBeginCommandBuffer() on active CB %p before it has completed. "
                     "You must check CB flag before this call.", cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->BeginCommandBuffer(cmdBuffer, pBeginInfo);
    loader_platform_thread_lock_mutex(&globalLock);
    clear_cmd_buf_and_mem_references(cmdBuffer);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkEndCommandBuffer(
    VkCmdBuffer cmdBuffer)
{
    // TODO : Anything to do here?
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->EndCommandBuffer(cmdBuffer);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetCommandBuffer(
    VkCmdBuffer cmdBuffer)
{
    loader_platform_thread_lock_mutex(&globalLock);
    // Verify that CB is complete (not in-flight)
    if (!checkCBCompleted(cmdBuffer)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM",
                "Resetting CB %p before it has completed. You must check CB flag before "
                     "calling vkResetCommandBuffer().", cmdBuffer);
    }
    // Clear memory references as this point.
    clear_cmd_buf_and_mem_references(cmdBuffer);
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->ResetCommandBuffer(cmdBuffer);
    return result;
}
// TODO : For any vkCmdBind* calls that include an object which has mem bound to it,
//    need to account for that mem now having binding to given cmdBuffer
VK_LAYER_EXPORT void VKAPI vkCmdBindPipeline(
    VkCmdBuffer         cmdBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline          pipeline)
{
#if 0
    // TODO : If memory bound to pipeline, then need to tie that mem to cmdBuffer
    if (getPipeline(pipeline)) {
        MT_CB_INFO *pCBInfo = get_cmd_buf_info(cmdBuffer);
        if (pCBInfo) {
            pCBInfo->pipelines[pipelineBindPoint] = pipeline;
        } else {
                    "Attempt to bind Pipeline %p to non-existant command buffer %p!", (void*)pipeline, cmdBuffer);
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_INVALID_CB, (char *) "DS", (char *) str);
        }
    }
    else {
                "Attempt to bind Pipeline %p that doesn't exist!", (void*)pipeline);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE, pipeline, 0, MEMTRACK_INVALID_OBJECT, (char *) "DS", (char *) str);
    }
#endif
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDynamicStateObject(
    VkCmdBuffer          cmdBuffer,
    VkStateBindPoint     stateBindPoint,
    VkDynamicStateObject state)
{
    MT_OBJ_INFO *pObjInfo;
    loader_platform_thread_lock_mutex(&globalLock);
    MT_CB_INFO *pCmdBuf = get_cmd_buf_info(cmdBuffer);
    if (!pCmdBuf) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_INVALID_CB, "MEM",
                "Unable to find command buffer object %p, was it ever created?", (void*)cmdBuffer);
    }
    pObjInfo = get_object_info(state);
    if (!pObjInfo) {
        /* TODO: put in real object type */
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, state, 0, MEMTRACK_INVALID_OBJECT, "MEM",
                "Unable to find dynamic state object %p, was it ever created?", (void*)state);
    }
    pCmdBuf->pDynamicState[stateBindPoint] = pObjInfo;
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindDynamicStateObject(cmdBuffer, stateBindPoint, state);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDescriptorSets(
    VkCmdBuffer            cmdBuffer,
    VkPipelineBindPoint    pipelineBindPoint,
    VkPipelineLayout       layout,
    uint32_t               firstSet,
    uint32_t               setCount,
    const VkDescriptorSet *pDescriptorSets,
    uint32_t               dynamicOffsetCount,
    const uint32_t        *pDynamicOffsets)
{
    // TODO : Somewhere need to verify that all textures referenced by shaders in DS are in some type of *SHADER_READ* state
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindDescriptorSets(
        cmdBuffer, pipelineBindPoint, layout, firstSet, setCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindVertexBuffers(
    VkCmdBuffer         cmdBuffer,
    uint32_t            startBinding,
    uint32_t            bindingCount,
    const VkBuffer     *pBuffers,
    const VkDeviceSize *pOffsets)
{
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindVertexBuffers(cmdBuffer, startBinding, bindingCount, pBuffers, pOffsets);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindIndexBuffer(
    VkCmdBuffer  cmdBuffer,
    VkBuffer     buffer,
    VkDeviceSize offset,
    VkIndexType  indexType)
{
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndirect(
    VkCmdBuffer   cmdBuffer,
     VkBuffer     buffer,
     VkDeviceSize offset,
     uint32_t     count,
     uint32_t     stride)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, buffer);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdDrawIndirect() call unable to update binding of buffer %p to cmdBuffer %p", buffer, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(
    VkCmdBuffer  cmdBuffer,
    VkBuffer     buffer,
    VkDeviceSize offset,
    uint32_t     count,
    uint32_t     stride)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, buffer);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdDrawIndexedIndirect() call unable to update binding of buffer %p to cmdBuffer %p", buffer, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatchIndirect(
    VkCmdBuffer  cmdBuffer,
    VkBuffer     buffer,
    VkDeviceSize offset)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, buffer);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdDispatchIndirect() call unable to update binding of buffer %p to cmdBuffer %p", buffer, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBuffer(
    VkCmdBuffer         cmdBuffer,
    VkBuffer            srcBuffer,
    VkBuffer            destBuffer,
    uint32_t            regionCount,
    const VkBufferCopy *pRegions)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, srcBuffer);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdCopyBuffer() call unable to update binding of srcBuffer %p to cmdBuffer %p", srcBuffer, cmdBuffer);
    }
    mem = get_mem_binding_from_object(cmdBuffer, destBuffer);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdCopyBuffer() call unable to update binding of destBuffer %p to cmdBuffer %p", destBuffer, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImage(
    VkCmdBuffer        cmdBuffer,
    VkImage            srcImage,
    VkImageLayout      srcImageLayout,
    VkImage            destImage,
    VkImageLayout      destImageLayout,
    uint32_t           regionCount,
    const VkImageCopy *pRegions)
{
    // TODO : Each image will have mem mapping so track them
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdCopyImage(
        cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdBlitImage(
    VkCmdBuffer        cmdBuffer,
    VkImage            srcImage,
    VkImageLayout      srcImageLayout,
    VkImage            destImage,
    VkImageLayout      destImageLayout,
    uint32_t           regionCount,
    const VkImageBlit *pRegions,
    VkTexFilter        filter)
{
    // TODO : Each image will have mem mapping so track them
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBlitImage(
        cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions, filter);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBufferToImage(
    VkCmdBuffer              cmdBuffer,
    VkBuffer                 srcBuffer,
    VkImage                  destImage,
    VkImageLayout            destImageLayout,
    uint32_t                 regionCount,
    const VkBufferImageCopy *pRegions)
{
    // TODO : Track this
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, destImage);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdCopyMemoryToImage() call unable to update binding of destImage buffer %p to cmdBuffer %p", destImage, cmdBuffer);
    }

    mem = get_mem_binding_from_object(cmdBuffer, srcBuffer);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdCopyMemoryToImage() call unable to update binding of srcBuffer %p to cmdBuffer %p", srcBuffer, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdCopyBufferToImage(
        cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImageToBuffer(
    VkCmdBuffer              cmdBuffer,
    VkImage                  srcImage,
    VkImageLayout            srcImageLayout,
    VkBuffer                 destBuffer,
    uint32_t                 regionCount,
    const VkBufferImageCopy *pRegions)
{
    // TODO : Track this
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, srcImage);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdCopyImageToMemory() call unable to update binding of srcImage buffer %p to cmdBuffer %p", srcImage, cmdBuffer);
    }
    mem = get_mem_binding_from_object(cmdBuffer, destBuffer);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdCopyImageToMemory() call unable to update binding of destBuffer %p to cmdBuffer %p", destBuffer, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdCopyImageToBuffer(
        cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdUpdateBuffer(
    VkCmdBuffer     cmdBuffer,
    VkBuffer        destBuffer,
    VkDeviceSize    destOffset,
    VkDeviceSize    dataSize,
    const uint32_t *pData)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, destBuffer);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdUpdateMemory() call unable to update binding of destBuffer %p to cmdBuffer %p", destBuffer, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdFillBuffer(
    VkCmdBuffer  cmdBuffer,
    VkBuffer     destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize fillSize,
    uint32_t     data)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, destBuffer);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdFillMemory() call unable to update binding of destBuffer %p to cmdBuffer %p", destBuffer, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorImage(
    VkCmdBuffer                    cmdBuffer,
    VkImage                        image,
    VkImageLayout                  imageLayout,
    const VkClearColor            *pColor,
    uint32_t                       rangeCount,
    const VkImageSubresourceRange *pRanges)
{
    // TODO : Verify memory is in VK_IMAGE_STATE_CLEAR state
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, image);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdClearColorImage() call unable to update binding of image buffer %p to cmdBuffer %p", image, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdClearColorImage(cmdBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencil(
    VkCmdBuffer                    cmdBuffer,
    VkImage                        image,
    VkImageLayout                  imageLayout,
    float                          depth,
    uint32_t                       stencil,
    uint32_t                       rangeCount,
    const VkImageSubresourceRange *pRanges)
{
    // TODO : Verify memory is in VK_IMAGE_STATE_CLEAR state
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, image);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdClearDepthStencil() call unable to update binding of image buffer %p to cmdBuffer %p", image, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdClearDepthStencil(
        cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdResolveImage(
    VkCmdBuffer           cmdBuffer,
    VkImage               srcImage,
    VkImageLayout         srcImageLayout,
    VkImage               destImage,
    VkImageLayout         destImageLayout,
    uint32_t              regionCount,
    const VkImageResolve *pRegions)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, srcImage);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdResolveImage() call unable to update binding of srcImage buffer %p to cmdBuffer %p", srcImage, cmdBuffer);
    }
    mem = get_mem_binding_from_object(cmdBuffer, destImage);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdResolveImage() call unable to update binding of destImage buffer %p to cmdBuffer %p", destImage, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdResolveImage(
        cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t    slot,
    VkFlags     flags)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, queryPool);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdBeginQuery() call unable to update binding of queryPool buffer %p to cmdBuffer %p", queryPool, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t    slot)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, queryPool);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdEndQuery() call unable to update binding of queryPool buffer %p to cmdBuffer %p", queryPool, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdEndQuery(cmdBuffer, queryPool, slot);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetQueryPool(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t    startQuery,
    uint32_t    queryCount)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, queryPool);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdResetQueryPool() call unable to update binding of queryPool buffer %p to cmdBuffer %p", queryPool, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgCreateMsgCallback(
        VkInstance instance,
        VkFlags msgFlags,
        const PFN_vkDbgMsgCallback pfnMsgCallback,
        void* pUserData,
        VkDbgMsgCallback* pMsgCallback)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(mem_tracker_instance_table_map, instance);
    VkResult res =  pTable->DbgCreateMsgCallback(instance, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    if (res == VK_SUCCESS) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

        res = layer_create_msg_callback(my_data->report_data, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    }
    return res;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgDestroyMsgCallback(
        VkInstance instance,
        VkDbgMsgCallback msgCallback)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(mem_tracker_instance_table_map, instance);
    VkResult res =  pTable->DbgDestroyMsgCallback(instance, msgCallback);

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    layer_destroy_msg_callback(my_data->report_data, msgCallback);

    return res;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSwapChainWSI(
    VkDevice                        device,
    const VkSwapChainCreateInfoWSI *pCreateInfo,
    VkSwapChainWSI                 *pSwapChain)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateSwapChainWSI(device, pCreateInfo, pSwapChain);

    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_swap_chain_info(*pSwapChain);
        loader_platform_thread_unlock_mutex(&globalLock);
    }

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroySwapChainWSI(
    VkSwapChainWSI swapChain)
{
    loader_platform_thread_lock_mutex(&globalLock);

    if (swapChainMap.find(swapChain) != swapChainMap.end()) {
        MT_SWAP_CHAIN_INFO* pInfo = swapChainMap[swapChain];

        if (pInfo->images.size() > 0) {
            for (std::vector<VkSwapChainImageInfoWSI>::const_iterator it = pInfo->images.begin();
                 it != pInfo->images.end(); it++) {
                clear_object_binding(it->image);
                freeMemObjInfo(swapChain, it->memory, true);

                objectMap.erase(it->image);
            }
        }

        delete pInfo;
        swapChainMap.erase(swapChain);
    }

    loader_platform_thread_unlock_mutex(&globalLock);

    return get_dispatch_table(mem_tracker_device_table_map, swapChain)->DestroySwapChainWSI(swapChain);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetSwapChainInfoWSI(
    VkSwapChainWSI          swapChain,
    VkSwapChainInfoTypeWSI  infoType,
    size_t                 *pDataSize,
    void                   *pData)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, swapChain)->GetSwapChainInfoWSI(swapChain, infoType, pDataSize, pData);

    if (infoType == VK_SWAP_CHAIN_INFO_TYPE_PERSISTENT_IMAGES_WSI && result == VK_SUCCESS) {
        const size_t count = *pDataSize / sizeof(VkSwapChainImageInfoWSI);
        MT_SWAP_CHAIN_INFO *pInfo = swapChainMap[swapChain];

        if (pInfo->images.empty()) {
            pInfo->images.resize(count);
            memcpy(&pInfo->images[0], pData, sizeof(pInfo->images[0]) * count);

            if (pInfo->images.size() > 0) {
                for (std::vector<VkSwapChainImageInfoWSI>::const_iterator it = pInfo->images.begin();
                     it != pInfo->images.end(); it++) {
                    // Add image object, then insert the new Mem Object and then bind it to created image
                    add_object_info(it->image, VK_STRUCTURE_TYPE_MAX_ENUM, &pInfo->createInfo, sizeof(pInfo->createInfo), "persistent_image");
                    add_mem_obj_info(swapChain, it->memory, NULL);
                    if (VK_FALSE == set_object_binding(swapChain, it->image, it->memory)) {
                        log_msg(mdd(swapChain), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE, it->image, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                                "In vkGetSwapChainInfoWSI(), unable to set image %p binding to mem obj %p", (void*)it->image, (void*)it->memory);
                    }
                }
            }
        } else {
            const bool mismatch = (pInfo->images.size() != count ||
                    memcmp(&pInfo->images[0], pData, sizeof(pInfo->images[0]) * count));

            if (mismatch) {
                log_msg(mdd(swapChain), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_SWAP_CHAIN_WSI, (VkObject) swapChain, 0, MEMTRACK_NONE, "SWAP_CHAIN",
                        "vkGetSwapChainInfoWSI(%p, VK_SWAP_CHAIN_INFO_TYPE_PERSISTENT_IMAGES_WSI) returned mismatching data", swapChain);
            }
        }
    }

    return result;
}

VK_LAYER_EXPORT void* VKAPI vkGetDeviceProcAddr(
    VkDevice         dev,
    const char       *funcName)
{
    void *fptr;

    if (dev == NULL) {
        return NULL;
    }

    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp(funcName, "vkGetDeviceProcAddr")) {
        initDeviceTable(mem_tracker_device_table_map, (const VkBaseLayerObject *) dev);
        return (void *) vkGetDeviceProcAddr;
    }
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (void*) vkDestroyDevice;
    if (!strcmp(funcName, "vkQueueSubmit"))
        return (void*) vkQueueSubmit;
    if (!strcmp(funcName, "vkAllocMemory"))
        return (void*) vkAllocMemory;
    if (!strcmp(funcName, "vkFreeMemory"))
        return (void*) vkFreeMemory;
    if (!strcmp(funcName, "vkMapMemory"))
        return (void*) vkMapMemory;
    if (!strcmp(funcName, "vkUnmapMemory"))
        return (void*) vkUnmapMemory;
    if (!strcmp(funcName, "vkOpenSharedMemory"))
        return (void*) vkOpenSharedMemory;
    if (!strcmp(funcName, "vkOpenPeerMemory"))
        return (void*) vkOpenPeerMemory;
    if (!strcmp(funcName, "vkOpenPeerImage"))
        return (void*) vkOpenPeerImage;
    if (!strcmp(funcName, "vkDestroyObject"))
        return (void*) vkDestroyObject;
    if (!strcmp(funcName, "vkGetObjectInfo"))
        return (void*) vkGetObjectInfo;
    if (!strcmp(funcName, "vkBindObjectMemory"))
        return (void*) vkBindObjectMemory;
    if (!strcmp(funcName, "vkQueueBindSparseBufferMemory"))
        return (void*) vkQueueBindSparseBufferMemory;
    if (!strcmp(funcName, "vkCreateFence"))
        return (void*) vkCreateFence;
    if (!strcmp(funcName, "vkGetFenceStatus"))
        return (void*) vkGetFenceStatus;
    if (!strcmp(funcName, "vkResetFences"))
        return (void*) vkResetFences;
    if (!strcmp(funcName, "vkWaitForFences"))
        return (void*) vkWaitForFences;
    if (!strcmp(funcName, "vkQueueWaitIdle"))
        return (void*) vkQueueWaitIdle;
    if (!strcmp(funcName, "vkDeviceWaitIdle"))
        return (void*) vkDeviceWaitIdle;
    if (!strcmp(funcName, "vkCreateEvent"))
        return (void*) vkCreateEvent;
    if (!strcmp(funcName, "vkCreateQueryPool"))
        return (void*) vkCreateQueryPool;
    if (!strcmp(funcName, "vkCreateBuffer"))
        return (void*) vkCreateBuffer;
    if (!strcmp(funcName, "vkCreateBufferView"))
        return (void*) vkCreateBufferView;
    if (!strcmp(funcName, "vkCreateImage"))
        return (void*) vkCreateImage;
    if (!strcmp(funcName, "vkCreateImageView"))
        return (void*) vkCreateImageView;
    if (!strcmp(funcName, "vkCreateColorAttachmentView"))
        return (void*) vkCreateColorAttachmentView;
    if (!strcmp(funcName, "vkCreateDepthStencilView"))
        return (void*) vkCreateDepthStencilView;
    if (!strcmp(funcName, "vkCreateShader"))
        return (void*) vkCreateShader;
    if (!strcmp(funcName, "vkCreateGraphicsPipeline"))
        return (void*) vkCreateGraphicsPipeline;
    if (!strcmp(funcName, "vkCreateGraphicsPipelineDerivative"))
        return (void*) vkCreateGraphicsPipelineDerivative;
    if (!strcmp(funcName, "vkCreateComputePipeline"))
        return (void*) vkCreateComputePipeline;
    if (!strcmp(funcName, "vkCreateSampler"))
        return (void*) vkCreateSampler;
    if (!strcmp(funcName, "vkCreateDynamicViewportState"))
        return (void*) vkCreateDynamicViewportState;
    if (!strcmp(funcName, "vkCreateDynamicRasterState"))
        return (void*) vkCreateDynamicRasterState;
    if (!strcmp(funcName, "vkCreateDynamicColorBlendState"))
        return (void*) vkCreateDynamicColorBlendState;
    if (!strcmp(funcName, "vkCreateDynamicDepthStencilState"))
        return (void*) vkCreateDynamicDepthStencilState;
    if (!strcmp(funcName, "vkCreateCommandBuffer"))
        return (void*) vkCreateCommandBuffer;
    if (!strcmp(funcName, "vkBeginCommandBuffer"))
        return (void*) vkBeginCommandBuffer;
    if (!strcmp(funcName, "vkEndCommandBuffer"))
        return (void*) vkEndCommandBuffer;
    if (!strcmp(funcName, "vkResetCommandBuffer"))
        return (void*) vkResetCommandBuffer;
    if (!strcmp(funcName, "vkCmdBindPipeline"))
        return (void*) vkCmdBindPipeline;
    if (!strcmp(funcName, "vkCmdBindDynamicStateObject"))
        return (void*) vkCmdBindDynamicStateObject;
    if (!strcmp(funcName, "vkCmdBindDescriptorSets"))
        return (void*) vkCmdBindDescriptorSets;
    if (!strcmp(funcName, "vkCmdBindVertexBuffers"))
        return (void*) vkCmdBindVertexBuffers;
    if (!strcmp(funcName, "vkCmdBindIndexBuffer"))
        return (void*) vkCmdBindIndexBuffer;
    if (!strcmp(funcName, "vkCmdDrawIndirect"))
        return (void*) vkCmdDrawIndirect;
    if (!strcmp(funcName, "vkCmdDrawIndexedIndirect"))
        return (void*) vkCmdDrawIndexedIndirect;
    if (!strcmp(funcName, "vkCmdDispatchIndirect"))
        return (void*) vkCmdDispatchIndirect;
    if (!strcmp(funcName, "vkCmdCopyBuffer"))
        return (void*) vkCmdCopyBuffer;
    if (!strcmp(funcName, "vkCmdCopyImage"))
        return (void*) vkCmdCopyImage;
    if (!strcmp(funcName, "vkCmdCopyBufferToImage"))
        return (void*) vkCmdCopyBufferToImage;
    if (!strcmp(funcName, "vkCmdCopyImageToBuffer"))
        return (void*) vkCmdCopyImageToBuffer;
    if (!strcmp(funcName, "vkCmdUpdateBuffer"))
        return (void*) vkCmdUpdateBuffer;
    if (!strcmp(funcName, "vkCmdFillBuffer"))
        return (void*) vkCmdFillBuffer;
    if (!strcmp(funcName, "vkCmdClearColorImage"))
        return (void*) vkCmdClearColorImage;
    if (!strcmp(funcName, "vkCmdClearDepthStencil"))
        return (void*) vkCmdClearDepthStencil;
    if (!strcmp(funcName, "vkCmdResolveImage"))
        return (void*) vkCmdResolveImage;
    if (!strcmp(funcName, "vkCmdBeginQuery"))
        return (void*) vkCmdBeginQuery;
    if (!strcmp(funcName, "vkCmdEndQuery"))
        return (void*) vkCmdEndQuery;
    if (!strcmp(funcName, "vkCmdResetQueryPool"))
        return (void*) vkCmdResetQueryPool;
    if (!strcmp(funcName, "vkGetDeviceQueue"))
        return (void*) vkGetDeviceQueue;

    VkLayerDispatchTable *pDisp =  get_dispatch_table(mem_tracker_device_table_map, dev);
    if (deviceExtMap.size() == 0 || deviceExtMap[pDisp].wsi_lunarg_enabled)
    {
        if (!strcmp(funcName, "vkCreateSwapChainWSI"))
            return (void*) vkCreateSwapChainWSI;
        if (!strcmp(funcName, "vkDestroySwapChainWSI"))
            return (void*) vkDestroySwapChainWSI;
        if (!strcmp(funcName, "vkGetSwapChainInfoWSI"))
            return (void*) vkGetSwapChainInfoWSI;
    }

    {
        if (pDisp->GetDeviceProcAddr == NULL)
            return NULL;
        return pDisp->GetDeviceProcAddr(dev, funcName);
    }
}

VK_LAYER_EXPORT void* VKAPI vkGetInstanceProcAddr(
    VkInstance       instance,
    const char       *funcName)
{
    void *fptr;
    if (instance == NULL) {
        return NULL;
    }

    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp(funcName, "vkGetInstanceProcAddr")) {
        initInstanceTable(mem_tracker_instance_table_map, (const VkBaseLayerObject *) instance);
        return (void *) vkGetInstanceProcAddr;
    }

    if (!strcmp(funcName, "vkDestroyInstance"))
        return (void *) vkDestroyInstance;
    if (!strcmp(funcName, "vkCreateInstance"))
        return (void*) vkCreateInstance;
    if (!strcmp(funcName, "vkCreateDevice"))
        return (void*) vkCreateDevice;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    fptr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (fptr)
        return fptr;

    {
        if (get_dispatch_table(mem_tracker_instance_table_map, instance)->GetInstanceProcAddr == NULL)
            return NULL;
        return get_dispatch_table(mem_tracker_instance_table_map, instance)->GetInstanceProcAddr(instance, funcName);
    }
}
