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
using namespace std;

#include "loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_struct_string_helper_cpp.h"
#include "mem_tracker.h"
#include "layers_config.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"
#include "layers_msg.h"

static VkLayerDispatchTable nextTable;
static VkBaseLayerObject *pCurObj;
static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(g_initOnce);
// TODO : This can be much smarter, using separate locks for separate global data
static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;

#define MAX_BINDING 0xFFFFFFFF

map<VkCmdBuffer, MT_CB_INFO*>      cbMap;
map<VkDeviceMemory, MT_MEM_OBJ_INFO*> memObjMap;
map<VkObject,     MT_OBJ_INFO*>     objectMap;
map<uint64_t,       MT_FENCE_INFO*>   fenceMap;    // Map fenceId to fence info
map<VkQueue,      MT_QUEUE_INFO*>   queueMap;

// TODO : Add per-device fence completion
static uint64_t     g_currentFenceId  = 1;
static VkDevice   globalDevice      = NULL;

// Add new queue for this device to map container
static void addQueueInfo(const VkQueue queue)
{
    MT_QUEUE_INFO* pInfo   = new MT_QUEUE_INFO;
    pInfo->lastRetiredId   = 0;
    pInfo->lastSubmittedId = 0;
    queueMap[queue]        = pInfo;
}

static void deleteQueueInfoList(void)
{
    // Process queue list, cleaning up each entry before deleting
    for (map<VkQueue, MT_QUEUE_INFO*>::iterator ii=queueMap.begin(); ii!=queueMap.end(); ++ii) {
        (*ii).second->pQueueCmdBuffers.clear();
    }
    queueMap.clear();
}

// Add new CBInfo for this cb to map container
static void addCBInfo(const VkCmdBuffer cb)
{
    MT_CB_INFO* pInfo = new MT_CB_INFO;
    memset(pInfo, 0, (sizeof(MT_CB_INFO) - sizeof(list<VkDeviceMemory>)));
    pInfo->cmdBuffer = cb;
    cbMap[cb]        = pInfo;
}

// Return ptr to Info in CB map, or NULL if not found
static MT_CB_INFO* getCBInfo(const VkCmdBuffer cb)
{
    MT_CB_INFO* pCBInfo = NULL;
    if (cbMap.find(cb) != cbMap.end()) {
        pCBInfo = cbMap[cb];
    }
    return pCBInfo;
}

// Return object info for 'object' or return NULL if no info exists
static MT_OBJ_INFO* getObjectInfo(const VkObject object)
{
    MT_OBJ_INFO* pObjInfo = NULL;

    if (objectMap.find(object) != objectMap.end()) {
        pObjInfo = objectMap[object];
    }
    return pObjInfo;
}

static MT_OBJ_INFO* addObjectInfo(VkObject object, VkStructureType sType, const void *pCreateInfo, const int struct_size, const char *name_prefix)
{
    MT_OBJ_INFO* pInfo = new MT_OBJ_INFO;
    memset(pInfo, 0, sizeof(MT_OBJ_INFO));
    memcpy(&pInfo->create_info, pCreateInfo, struct_size);
    sprintf(pInfo->object_name, "%s_%p", name_prefix, object);

    pInfo->object     = object;
    pInfo->ref_count  = 1;
    pInfo->sType      = sType;
    objectMap[object] = pInfo;

    return pInfo;
}

// Add a fence, creating one if necessary to our list of fences/fenceIds
static uint64_t addFenceInfo(VkFence fence, VkQueue queue)
{
    // Create fence object
    MT_FENCE_INFO* pFenceInfo = new MT_FENCE_INFO;
    MT_QUEUE_INFO* pQueueInfo = queueMap[queue];
    uint64_t       fenceId    = g_currentFenceId++;
    memset(pFenceInfo, 0, sizeof(MT_FENCE_INFO));
    // If no fence, create an internal fence to track the submissions
    if (fence == NULL) {
        VkFenceCreateInfo fci;
        fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fci.pNext = NULL;
        fci.flags = static_cast<VkFenceCreateFlags>(0);
        nextTable.CreateFence(globalDevice, &fci, &pFenceInfo->fence);
        addObjectInfo(pFenceInfo->fence, fci.sType, &fci, sizeof(VkFenceCreateInfo), "internalFence");
        pFenceInfo->localFence = VK_TRUE;
    } else {
        // Validate that fence is in UNSIGNALED state
        MT_OBJ_INFO* pObjectInfo = getObjectInfo(fence);
        if (pObjectInfo != NULL) {
            if (pObjectInfo->create_info.fence_create_info.flags & VK_FENCE_CREATE_SIGNALED_BIT) {
                char str[1024];
                sprintf(str, "Fence %p submitted in SIGNALED state.  Fences must be reset before being submitted", fence);
                layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, fence, 0, MEMTRACK_INVALID_FENCE_STATE, "MEM", str);
            }
        }
        pFenceInfo->localFence = VK_FALSE;
        pFenceInfo->fence      = fence;
    }
    pFenceInfo->queue = queue;
    fenceMap[fenceId] = pFenceInfo;
    // Update most recently submitted fenceId for Queue
    pQueueInfo->lastSubmittedId = fenceId;
    return fenceId;
}

// Remove a fenceInfo from our list of fences/fenceIds
static void deleteFenceInfo(uint64_t fenceId)
{
    if (fenceId != 0) {
        if (fenceMap.find(fenceId) != fenceMap.end()) {
            map<uint64_t, MT_FENCE_INFO*>::iterator item;
            MT_FENCE_INFO* pDelInfo = fenceMap[fenceId];
            if (pDelInfo != NULL) {
                if (pDelInfo->localFence == VK_TRUE) {
                    nextTable.DestroyObject(pDelInfo->fence);
                }
                delete pDelInfo;
            }
            item = fenceMap.find(fenceId);
            fenceMap.erase(item);
        }
    }
}

// Search through list for this fence, deleting all items before it (with lower IDs) and updating lastRetiredId
static void updateFenceTracking(VkFence fence)
{
    MT_FENCE_INFO *pCurFenceInfo = NULL;
    uint64_t       fenceId       = 0;
    VkQueue      queue         = NULL;
    bool found = false;

    for (map<uint64_t, MT_FENCE_INFO*>::iterator ii=fenceMap.begin(); !found && ii!=fenceMap.end(); ++ii) {
        if ((*ii).second != NULL) {
            if (fence == ((*ii).second)->fence) {
                queue = ((*ii).second)->queue;
                MT_QUEUE_INFO *pQueueInfo = queueMap[queue];
                pQueueInfo->lastRetiredId = (*ii).first;
                found = true;
            } else {
                deleteFenceInfo((*ii).first);
            }
            // Update fence state in fenceCreateInfo structure
            MT_OBJ_INFO* pObjectInfo = getObjectInfo(fence);
            if (pObjectInfo != NULL) {
                pObjectInfo->create_info.fence_create_info.flags =
                    static_cast<VkFenceCreateFlags>(pObjectInfo->create_info.fence_create_info.flags | VK_FENCE_CREATE_SIGNALED_BIT);
            }
        }
    }
}

// Utility function that determines if a fenceId has been retired yet
static bool32_t fenceRetired(uint64_t fenceId)
{
    bool32_t result = VK_FALSE;
    if (fenceMap.find(fenceId) != fenceMap.end()) {
        MT_FENCE_INFO* pFenceInfo = fenceMap[fenceId];
        MT_QUEUE_INFO* pQueueInfo = queueMap[pFenceInfo->queue];
        if (fenceId <= pQueueInfo->lastRetiredId)
        {
             result = VK_TRUE;
        }
    } else {                 // If not in list, fence has been retired and deleted
       result = VK_TRUE;
    }
    return result;
}

// Return the fence associated with a fenceId
static VkFence getFenceFromId(uint64_t fenceId)
{
    VkFence fence = NULL;
    if (fenceId != 0) {
        // Search for an item with this fenceId
        if (fenceMap.find(fenceId) != fenceMap.end()) {
            MT_FENCE_INFO* pFenceInfo = fenceMap[fenceId];
            if (pFenceInfo != NULL) {
                MT_QUEUE_INFO* pQueueInfo = queueMap[pFenceInfo->queue];
                if (fenceId > pQueueInfo->lastRetiredId) {
                    fence = pFenceInfo->fence;
                }
            }
        }
    }
    return fence;
}

// Helper routine that updates the fence list for a specific queue to all-retired
static void retireQueueFences(VkQueue queue)
{
    MT_QUEUE_INFO *pQueueInfo = queueMap[queue];
    pQueueInfo->lastRetiredId = pQueueInfo->lastSubmittedId;
    // Set Queue's lastRetired to lastSubmitted, free items in queue's fence list
    map<uint64_t, MT_FENCE_INFO*>::iterator it = fenceMap.begin();
    map<uint64_t, MT_FENCE_INFO*>::iterator temp;
    while (it != fenceMap.end()) {
        if ((((*it).second) != NULL) && ((*it).second)->queue == queue) {
            temp = it;
            ++temp;
            deleteFenceInfo((*it).first);
            it = temp;
        } else {
            ++it;
        }
    }
}

// Helper routine that updates fence list for all queues to all-retired
static void retireDeviceFences(VkDevice device)
{
    // Process each queue for device
    // TODO: Add multiple device support
    for (map<VkQueue, MT_QUEUE_INFO*>::iterator ii=queueMap.begin(); ii!=queueMap.end(); ++ii) {
        retireQueueFences((*ii).first);
    }
}

// Returns True if a memory reference is present in a Queue's memory reference list
// Queue is validated by caller
static bool32_t checkMemRef(
    VkQueue      queue,
    VkDeviceMemory mem)
{
    bool32_t result = VK_FALSE;
    list<VkDeviceMemory>::iterator it;
    MT_QUEUE_INFO *pQueueInfo = queueMap[queue];
    for (it = pQueueInfo->pMemRefList.begin(); it != pQueueInfo->pMemRefList.end(); ++it) {
        if ((*it) == mem) {
            result = VK_TRUE;
            break;
        }
    }
    return result;
}

static bool32_t validateQueueMemRefs(
    VkQueue             queue,
    uint32_t              cmdBufferCount,
    const VkCmdBuffer *pCmdBuffers)
{
    bool32_t result = VK_TRUE;

    //  Verify Queue
    MT_QUEUE_INFO *pQueueInfo = queueMap[queue];
    if (pQueueInfo == NULL) {
        char str[1024];
        sprintf(str, "Unknown Queue %p specified in vkQueueSubmit", queue);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, queue, 0, MEMTRACK_INVALID_QUEUE, "MEM", str);
    }
    else {
        //  Iterate through all CBs in pCmdBuffers
        for (uint32_t i = 0; i < cmdBufferCount; i++) {
            MT_CB_INFO* pCBInfo = getCBInfo(pCmdBuffers[i]);
            if (!pCBInfo) {
                char str[1024];
                sprintf(str, "Unable to find info for CB %p in order to check memory references in vkQueueSubmit for queue %p", (void*)pCmdBuffers[i], queue);
                layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pCmdBuffers[i], 0, MEMTRACK_INVALID_CB, "MEM", str);
                result = VK_FALSE;
            } else {
                // Validate that all actual references are accounted for in pMemRefs
                for (list<VkDeviceMemory>::iterator it = pCBInfo->pMemObjList.begin(); it != pCBInfo->pMemObjList.end(); ++it) {
                    // Search for each memref in queues memreflist.
                    if (checkMemRef(queue, *it)) {
                        char str[1024];
                        sprintf(str, "Found Mem Obj %p binding to CB %p for queue %p", (*it), pCmdBuffers[i], queue);
                        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, pCmdBuffers[i], 0, MEMTRACK_NONE, "MEM", str);
                    }
                    else {
                        char str[1024];
                        sprintf(str, "Queue %p Memory reference list for Command Buffer %p is missing ref to mem obj %p", queue, pCmdBuffers[i], (*it));
                        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pCmdBuffers[i], 0, MEMTRACK_INVALID_MEM_REF, "MEM", str);
                        result = VK_FALSE;
                    }
                }
            }
        }
        if (result == VK_TRUE) {
            char str[1024];
            sprintf(str, "Verified all memory dependencies for Queue %p are included in pMemRefs list", queue);
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, queue, 0, MEMTRACK_NONE, "MEM", str);
            // TODO : Could report mem refs in pMemRefs that AREN'T in mem list, that would be primarily informational
            //   Currently just noting that there is a difference
        }
    }

    return result;
}

// Return ptr to info in map container containing mem, or NULL if not found
//  Calls to this function should be wrapped in mutex
static MT_MEM_OBJ_INFO* getMemObjInfo(const VkDeviceMemory mem)
{
    MT_MEM_OBJ_INFO* pMemObjInfo = NULL;

    if (memObjMap.find(mem) != memObjMap.end()) {
        pMemObjInfo = memObjMap[mem];
    }
    return pMemObjInfo;
}

static void addMemObjInfo(const VkDeviceMemory mem, const VkMemoryAllocInfo* pAllocInfo)
{
    MT_MEM_OBJ_INFO* pInfo = new MT_MEM_OBJ_INFO;
    pInfo->refCount           = 0;
    memset(&pInfo->allocInfo, 0, sizeof(VkMemoryAllocInfo));

    if (pAllocInfo) {  // MEM alloc created by vkWsiX11CreatePresentableImage() doesn't have alloc info struct
        memcpy(&pInfo->allocInfo, pAllocInfo, sizeof(VkMemoryAllocInfo));
        // TODO:  Update for real hardware, actually process allocation info structures
        pInfo->allocInfo.pNext = NULL;
    }
    pInfo->mem = mem;
    memObjMap[mem] = pInfo;
}

// Find CB Info and add mem binding to list container
// Find Mem Obj Info and add CB binding to list container
static bool32_t updateCBBinding(const VkCmdBuffer cb, const VkDeviceMemory mem)
{
    bool32_t result = VK_TRUE;
    // First update CB binding in MemObj mini CB list
    MT_MEM_OBJ_INFO* pMemInfo = getMemObjInfo(mem);
    if (!pMemInfo) {
        char str[1024];
        sprintf(str, "Trying to bind mem obj %p to CB %p but no info for that mem obj.\n    Was it correctly allocated? Did it already get freed?", mem, cb);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
        result = VK_FALSE;
    } else {
        // Search for cmd buffer object in memory object's binding list
        bool32_t found  = VK_FALSE;
        for (list<VkCmdBuffer>::iterator it = pMemInfo->pCmdBufferBindings.begin(); it != pMemInfo->pCmdBufferBindings.end(); ++it) {
            if ((*it) == cb) {
                found = VK_TRUE;
                break;
            }
        }
        // If not present, add to list
        if (found == VK_FALSE) {
            pMemInfo->pCmdBufferBindings.push_front(cb);
            pMemInfo->refCount++;
        }

        // Now update CBInfo's Mem binding list
        MT_CB_INFO* pCBInfo = getCBInfo(cb);
        if (!pCBInfo) {
            char str[1024];
            sprintf(str, "Trying to bind mem obj %p to CB %p but no info for that CB.    Was it CB incorrectly destroyed?", mem, cb);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
            result = VK_FALSE;
        } else {
            // Search for memory object in cmd buffer's binding list
            bool32_t found  = VK_FALSE;
            for (list<VkDeviceMemory>::iterator it = pCBInfo->pMemObjList.begin(); it != pCBInfo->pMemObjList.end(); ++it) {
                if ((*it) == mem) {
                    found = VK_TRUE;
                    break;
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
//  Calls to this function should be wrapped in mutex
static void clearCBBinding(const VkCmdBuffer cb, const VkDeviceMemory mem)
{
    MT_MEM_OBJ_INFO* pInfo = getMemObjInfo(mem);
    // TODO : Having this check is not ideal, really if memInfo was deleted,
    //   its CB bindings should be cleared and then freeCBBindings wouldn't call
    //   us here with stale mem objs
    if (pInfo) {
        pInfo->pCmdBufferBindings.remove(cb);
        pInfo->refCount--;
    }
}

// Free bindings related to CB
static bool32_t freeCBBindings(const VkCmdBuffer cb)
{
    bool32_t result = VK_TRUE;
    MT_CB_INFO* pCBInfo = getCBInfo(cb);
    if (!pCBInfo) {
        char str[1024];
        sprintf(str, "Unable to find global CB info %p for deletion", cb);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_CB, "MEM", str);
        result = VK_FALSE;
    } else {
        if (!fenceRetired(pCBInfo->fenceId)) {
            deleteFenceInfo(pCBInfo->fenceId);
        }

        for (list<VkDeviceMemory>::iterator it=pCBInfo->pMemObjList.begin(); it!=pCBInfo->pMemObjList.end(); ++it) {
            clearCBBinding(cb, (*it));
        }
        pCBInfo->pMemObjList.clear();
    }
    return result;
}

// Delete CBInfo from list along with all of it's mini MemObjInfo
//   and also clear mem references to CB
// TODO : When should this be called?  There's no Destroy of CBs that I see
static bool32_t deleteCBInfo(const VkCmdBuffer cb)
{
    bool32_t result = VK_TRUE;
    result = freeCBBindings(cb);
    // Delete the CBInfo info
    if (result == VK_TRUE) {
        if (cbMap.find(cb) != cbMap.end()) {
            MT_CB_INFO* pDelInfo = cbMap[cb];
            delete pDelInfo;
            cbMap.erase(cb);
        }
    }
    return result;
}

// Delete the entire CB list
static bool32_t deleteCBInfoList()
{
    bool32_t result = VK_TRUE;
    for (map<VkCmdBuffer, MT_CB_INFO*>::iterator ii=cbMap.begin(); ii!=cbMap.end(); ++ii) {
        freeCBBindings((*ii).first);
        delete (*ii).second;
    }
    return result;
}

// For given MemObjInfo, report Obj & CB bindings
static void reportMemReferencesAndCleanUp(MT_MEM_OBJ_INFO* pMemObjInfo)
{
    uint32_t cmdBufRefCount = pMemObjInfo->pCmdBufferBindings.size();
    uint32_t objRefCount    = pMemObjInfo->pObjBindings.size();

    if ((pMemObjInfo->pCmdBufferBindings.size() + pMemObjInfo->pObjBindings.size()) != 0) {
        char str[1024];
        sprintf(str, "Attempting to free memory object %p which still contains %d references", pMemObjInfo->mem, (cmdBufRefCount + objRefCount));
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pMemObjInfo->mem, 0, MEMTRACK_INTERNAL_ERROR, "MEM", str);
    }

    if (cmdBufRefCount > 0) {
        for (list<VkCmdBuffer>::const_iterator it = pMemObjInfo->pCmdBufferBindings.begin(); it != pMemObjInfo->pCmdBufferBindings.end(); ++it) {
            char str[1024];
            sprintf(str, "Command Buffer %p still has a reference to mem obj %p", (*it), pMemObjInfo->mem);
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, (*it), 0, MEMTRACK_NONE, "MEM", str);
        }
        // Clear the list of hanging references
        pMemObjInfo->pCmdBufferBindings.clear();
    }

    if (objRefCount > 0) {
        for (list<VkObject>::const_iterator it = pMemObjInfo->pObjBindings.begin(); it != pMemObjInfo->pObjBindings.end(); ++it) {
            char str[1024];
            sprintf(str, "VK Object %p still has a reference to mem obj %p", (*it), pMemObjInfo->mem);
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, (*it), 0, MEMTRACK_NONE, "MEM", str);
        }
        // Clear the list of hanging references
        pMemObjInfo->pObjBindings.clear();
    }
}

static void deleteMemObjInfo(VkDeviceMemory mem)
{
    if (memObjMap.find(mem) != memObjMap.end()) {
        MT_MEM_OBJ_INFO* pDelInfo = memObjMap[mem];
        delete pDelInfo;
        memObjMap.erase(mem);
    }
    else {
        char str[1024];
        sprintf(str, "Request to delete memory object %p not present in memory Object Map", mem);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
    }
}

// Check if fence for given CB is completed
static bool32_t checkCBCompleted(const VkCmdBuffer cb)
{
    bool32_t result = VK_TRUE;
    MT_CB_INFO* pCBInfo = getCBInfo(cb);
    if (!pCBInfo) {
        char str[1024];
        sprintf(str, "Unable to find global CB info %p to check for completion", cb);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_CB, "MEM", str);
        result = VK_FALSE;
    } else {
        if (!fenceRetired(pCBInfo->fenceId)) {
            char str[1024];
            sprintf(str, "FenceId %" PRIx64", fence %p for CB %p has not been checked for completion", pCBInfo->fenceId, getFenceFromId(pCBInfo->fenceId), cb);
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_NONE, "MEM", str);
            result = VK_FALSE;
        }
    }
    return result;
}

static bool32_t freeMemObjInfo(VkDeviceMemory mem, bool internal)
{
    bool32_t result = VK_TRUE;
    // Parse global list to find info w/ mem
    MT_MEM_OBJ_INFO* pInfo = getMemObjInfo(mem);
    if (!pInfo) {
        char str[1024];
        sprintf(str, "Couldn't find mem info object for %p\n    Was %p never allocated or previously freed?", (void*)mem, (void*)mem);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
        result = VK_FALSE;
    } else {
        if (pInfo->allocInfo.allocationSize == 0 && !internal) {
            char str[1024];
            sprintf(str, "Attempting to free memory associated with a Presentable Image, %p, this should not be explicitly freed\n", (void*)mem);
            layerCbMsg(VK_DBG_MSG_WARNING, VK_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
            result = VK_FALSE;
        } else {
            // Clear any CB bindings for completed CBs
            //   TODO : Is there a better place to do this?

            list<VkCmdBuffer>::iterator it = pInfo->pCmdBufferBindings.begin();
            list<VkCmdBuffer>::iterator temp;
            while (it != pInfo->pCmdBufferBindings.end()) {
                if (VK_TRUE == checkCBCompleted(*it)) {
                    temp = it;
                    ++temp;
                    freeCBBindings(*it);
                    it = temp;
                } else {
                    ++it;
                }
            }

            // Now verify that no references to this mem obj remain
            if (0 != pInfo->refCount) {
                // If references remain, report the error and can search CB list to find references
                char str[1024];
                sprintf(str, "Freeing mem obj %p while it still has references", (void*)mem);
                layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_FREED_MEM_REF, "MEM", str);
                reportMemReferencesAndCleanUp(pInfo);
                result = VK_FALSE;
            }
            // Delete mem obj info
            deleteMemObjInfo(mem);
        }
    }
    return result;
}

// Remove object binding performs 3 tasks:
// 1. Remove ObjectInfo from MemObjInfo list container of obj bindings & free it
// 2. Decrement refCount for MemObjInfo
// 3. Clear MemObjInfo ptr from ObjectInfo
static bool32_t clearObjectBinding(VkObject object)
{
    bool32_t result = VK_FALSE;
    MT_OBJ_INFO* pObjInfo = getObjectInfo(object);
    if (!pObjInfo) {
        char str[1024];
        sprintf(str, "Attempting to clear mem binding for object %p: devices, queues, command buffers, shaders and memory objects do not have external memory requirements and it is unneccessary to call bind/unbindObjectMemory on them.", object);
        layerCbMsg(VK_DBG_MSG_WARNING, VK_VALIDATION_LEVEL_0, object, 0, MEMTRACK_INVALID_OBJECT, "MEM", str);
    } else {
        if (!pObjInfo->pMemObjInfo) {
            char str[1024];
            sprintf(str, "Attempting to clear mem binding on obj %p but it has no binding.", (void*)object);
            layerCbMsg(VK_DBG_MSG_WARNING, VK_VALIDATION_LEVEL_0, object, 0, MEMTRACK_MEM_OBJ_CLEAR_EMPTY_BINDINGS, "MEM", str);
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
                char str[1024];
                sprintf(str, "While trying to clear mem binding for object %p, unable to find that object referenced by mem obj %p",
                    object, pObjInfo->pMemObjInfo->mem);
                layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, object, 0, MEMTRACK_INTERNAL_ERROR, "MEM", str);
            }
        }
    }
    return result;
}

// For NULL mem case, clear any previous binding Else...
// Make sure given object is in global object map
//  IF a previous binding existed, clear it
//  Add reference from objectInfo to memoryInfo
//  Add reference off of objInfo
// Return VK_TRUE if addition is successful, VK_FALSE otherwise
static bool32_t updateObjectBinding(VkObject object, VkDeviceMemory mem)
{
    bool32_t result = VK_FALSE;
    // Handle NULL case separately, just clear previous binding & decrement reference
    if (mem == VK_NULL_HANDLE) {
        clearObjectBinding(object);
        result = VK_TRUE;
    } else {
        char str[1024];
        MT_OBJ_INFO* pObjInfo = getObjectInfo(object);
        if (!pObjInfo) {
            sprintf(str, "Attempting to update Binding of Obj(%p) that's not in global list()", (void*)object);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, object, 0, MEMTRACK_INTERNAL_ERROR, "MEM", str);
            return VK_FALSE;
        }
        // non-null case so should have real mem obj
        MT_MEM_OBJ_INFO* pInfo = getMemObjInfo(mem);
        if (!pInfo) {
            sprintf(str, "While trying to bind mem for obj %p, couldn't find info for mem obj %p", (void*)object, (void*)mem);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
        } else {
            // Search for object in memory object's binding list
            bool32_t found  = VK_FALSE;
            for (list<VkObject>::iterator it = pInfo->pObjBindings.begin(); it != pInfo->pObjBindings.end(); ++it) {
                if ((*it) == object) {
                    found = VK_TRUE;
                    break;
                }
            }
            // If not present, add to list
            if (found == VK_FALSE) {
                pInfo->pObjBindings.push_front(object);
                pInfo->refCount++;
            }

            if (pObjInfo->pMemObjInfo) {
                clearObjectBinding(object); // Need to clear the previous object binding before setting new binding
                sprintf(str, "Updating memory binding for object %p from mem obj %p to %p", object, pObjInfo->pMemObjInfo->mem, mem);
                layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, object, 0, MEMTRACK_NONE, "MEM", str);
            }
            // For image objects, make sure default memory state is correctly set
            // TODO : What's the best/correct way to handle this?
            if (VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO == pObjInfo->sType) {
                if (pObjInfo->create_info.image_create_info.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_BIT)) {
                    // TODO::  More memory state transition stuff.
                }
            }
            pObjInfo->pMemObjInfo = pInfo;
        }
    }
    return VK_TRUE;
}

// Print details of global Obj tracking list
static void printObjList()
{
    MT_OBJ_INFO* pInfo = NULL;
    char str[1024];
    sprintf(str, "Details of Object list of size %lu elements", objectMap.size());
    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
    for (map<VkObject, MT_OBJ_INFO*>::iterator ii=objectMap.begin(); ii!=objectMap.end(); ++ii) {
        pInfo = (*ii).second;
        sprintf(str, "    ObjInfo %p has object %p, pMemObjInfo %p", pInfo, pInfo->object, pInfo->pMemObjInfo);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, pInfo->object, 0, MEMTRACK_NONE, "MEM", str);
    }
}

// For given Object, get 'mem' obj that it's bound to or NULL if no binding
static VkDeviceMemory getMemBindingFromObject(const VkObject object)
{
    VkDeviceMemory mem = NULL;
    MT_OBJ_INFO* pObjInfo = getObjectInfo(object);
    if (pObjInfo) {
        if (pObjInfo->pMemObjInfo) {
            mem = pObjInfo->pMemObjInfo->mem;
        }
        else {
            char str[1024];
            sprintf(str, "Trying to get mem binding for object %p but object has no mem binding", (void*)object);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, object, 0, MEMTRACK_MISSING_MEM_BINDINGS, "MEM", str);
            printObjList();
        }
    }
    else {
        char str[1024];
        sprintf(str, "Trying to get mem binding for object %p but no such object in global list", (void*)object);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, object, 0, MEMTRACK_INVALID_OBJECT, "MEM", str);
        printObjList();
    }
    return mem;
}

// Print details of MemObjInfo list
static void printMemList()
{
    MT_MEM_OBJ_INFO* pInfo = NULL;
    // Just printing each msg individually for now, may want to package these into single large print
    char str[1024];
    sprintf(str, "MEM INFO : Details of Memory Object list of size %lu elements", memObjMap.size());
    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);

    for (map<VkDeviceMemory, MT_MEM_OBJ_INFO*>::iterator ii=memObjMap.begin(); ii!=memObjMap.end(); ++ii) {
        pInfo = (*ii).second;

        sprintf(str, "    ===MemObjInfo at %p===", (void*)pInfo);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        sprintf(str, "    Mem object: %p", (void*)pInfo->mem);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        sprintf(str, "    Ref Count: %u", pInfo->refCount);
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        if (0 != pInfo->allocInfo.allocationSize) {
            string pAllocInfoMsg = vk_print_vkmemoryallocinfo(&pInfo->allocInfo, "{MEM}INFO :       ");
            sprintf(str, "    Mem Alloc info:\n%s", pAllocInfoMsg.c_str());
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        } else {
            sprintf(str, "    Mem Alloc info is NULL (alloc done by vkWsiX11CreatePresentableImage())");
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        }

        sprintf(str, "    VK OBJECT Binding list of size %lu elements:", pInfo->pObjBindings.size());
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        for (list<VkObject>::iterator it = pInfo->pObjBindings.begin(); it != pInfo->pObjBindings.end(); ++it) {
            sprintf(str, "       VK OBJECT %p", (*it));
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        }

        sprintf(str, "    VK Command Buffer (CB) binding list of size %lu elements", pInfo->pCmdBufferBindings.size());
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        for (list<VkCmdBuffer>::iterator it = pInfo->pCmdBufferBindings.begin(); it != pInfo->pCmdBufferBindings.end(); ++it) {
            sprintf(str, "      VK CB %p", (*it));
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        }
    }
}

static void printCBList()
{
    char str[1024] = {0};
    MT_CB_INFO* pCBInfo = NULL;
    sprintf(str, "Details of CB list of size %lu elements", cbMap.size());
    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);

    for (map<VkCmdBuffer, MT_CB_INFO*>::iterator ii=cbMap.begin(); ii!=cbMap.end(); ++ii) {
        pCBInfo = (*ii).second;

        sprintf(str, "    CB Info (%p) has CB %p, fenceId %" PRIx64", and fence %p",
            (void*)pCBInfo, (void*)pCBInfo->cmdBuffer, pCBInfo->fenceId,
            (void*)getFenceFromId(pCBInfo->fenceId));
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);

        for (list<VkDeviceMemory>::iterator it = pCBInfo->pMemObjList.begin(); it != pCBInfo->pMemObjList.end(); ++it) {
            sprintf(str, "      Mem obj %p", (*it));
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        }
    }
}

static void initMemTracker(void)
{
    const char *strOpt;
    // initialize MemTracker options
    getLayerOptionEnum("MemTrackerReportLevel", (uint32_t *) &g_reportingLevel);
    g_actionIsDefault = getLayerOptionEnum("MemTrackerDebugAction", (uint32_t *) &g_debugAction);

    if (g_debugAction & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        strOpt = getLayerOption("MemTrackerLogFilename");
        if (strOpt)
        {
            g_logFile = fopen(strOpt, "w");
        }
        if (g_logFile == NULL)
            g_logFile = stdout;
    }

    // initialize Layer dispatch table
    // TODO handle multiple GPUs
    PFN_vkGetProcAddr fpNextGPA;
    fpNextGPA = pCurObj->pGPA;
    assert(fpNextGPA);

    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (VkPhysicalDevice) pCurObj->nextObject);

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

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    pCurObj = (VkBaseLayerObject *) gpu;
    loader_platform_thread_once(&g_initOnce, initMemTracker);
    VkResult result = nextTable.CreateDevice(gpu, pCreateInfo, pDevice);
    // Save off device in case we need it to create Fences
    globalDevice = *pDevice;
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyDevice(VkDevice device)
{
    char str[1024];
    sprintf(str, "Printing List details prior to vkDestroyDevice()");
    loader_platform_thread_lock_mutex(&globalLock);
    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, device, 0, MEMTRACK_NONE, "MEM", str);
    printMemList();
    printCBList();
    printObjList();
    if (VK_FALSE == deleteCBInfoList()) {
        sprintf(str, "Issue deleting global CB list in vkDestroyDevice()");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, device, 0, MEMTRACK_INTERNAL_ERROR, "MEM", str);
    }
    // Report any memory leaks
    MT_MEM_OBJ_INFO* pInfo = NULL;
    for (map<VkDeviceMemory, MT_MEM_OBJ_INFO*>::iterator ii=memObjMap.begin(); ii!=memObjMap.end(); ++ii) {
        pInfo = (*ii).second;

        if (pInfo->allocInfo.allocationSize != 0) {
            sprintf(str, "Mem Object %p has not been freed. You should clean up this memory by calling vkFreeMemory(%p) prior to vkDestroyDevice().",
                pInfo->mem, pInfo->mem);
            layerCbMsg(VK_DBG_MSG_WARNING, VK_VALIDATION_LEVEL_0, pInfo->mem, 0, MEMTRACK_MEMORY_LEAK, "MEM", str);
        }
    }

    // Queues persist until device is destroyed
    deleteQueueInfoList();

    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = nextTable.DestroyDevice(device);
    return result;
}

struct extProps {
    uint32_t version;
    const char * const name;
};
#define MEM_TRACKER_LAYER_EXT_ARRAY_SIZE 2
static const struct extProps mtExts[MEM_TRACKER_LAYER_EXT_ARRAY_SIZE] = {
    // TODO what is the version?
    0x10, "MemTracker",
    0x10, "Validation"
};

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionInfo(
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData)
{
    VkResult result;

    /* This entrypoint is NOT going to init it's own dispatch table since loader calls here early */
    VkExtensionProperties *ext_props;
    uint32_t *count;

    if (pDataSize == NULL)
        return VK_ERROR_INVALID_POINTER;

    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL)
                return VK_SUCCESS;
            count = (uint32_t *) pData;
            *count = MEM_TRACKER_LAYER_EXT_ARRAY_SIZE;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            if (extensionIndex >= MEM_TRACKER_LAYER_EXT_ARRAY_SIZE)
                return VK_ERROR_INVALID_VALUE;
            ext_props = (VkExtensionProperties *) pData;
            ext_props->version = mtExts[extensionIndex].version;
            strncpy(ext_props->extName, mtExts[extensionIndex].name,
                                        VK_MAX_EXTENSION_NAME);
            ext_props->extName[VK_MAX_EXTENSION_NAME - 1] = '\0';
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateLayers(VkPhysicalDevice gpu, size_t maxLayerCount,
    size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved)
{
        if (gpu != NULL)
    {
        pCurObj = (VkBaseLayerObject *)  gpu;
        loader_platform_thread_once(&g_initOnce, initMemTracker);
        VkResult result = nextTable.EnumerateLayers(gpu, maxLayerCount,
            maxStringSize, pOutLayerCount, pOutLayers, pReserved);
        return result;
    } else
    {
        if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)
            return VK_ERROR_INVALID_POINTER;
        // This layer compatible with all GPUs
        *pOutLayerCount = 1;
        strncpy((char *) pOutLayers[0], "MemTracker", maxStringSize);
        return VK_SUCCESS;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetDeviceQueue(VkDevice device, uint32_t queueNodeIndex, uint32_t queueIndex, VkQueue* pQueue)
{
    VkResult result = nextTable.GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        addQueueInfo(*pQueue);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueAddMemReferences(VkQueue queue, uint32_t count, const VkDeviceMemory* pMems)
{
    VkResult result = nextTable.QueueAddMemReferences(queue, count, pMems);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);

        MT_QUEUE_INFO *pQueueInfo = queueMap[queue];
        if (pQueueInfo == NULL) {
            char str[1024];
            sprintf(str, "Unknown Queue %p", queue);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, queue, 0, MEMTRACK_INVALID_QUEUE, "MEM", str);
        } else {
            for (int i = 0; i < count; i++) {
                if (checkMemRef(queue, pMems[i]) == VK_TRUE) {
                    // Alread in list, just warn
                    char str[1024];
                    sprintf(str, "Request to add a memory reference (%p) to Queue %p -- ref is already present in the queue's reference list", pMems[i], queue);
                    layerCbMsg(VK_DBG_MSG_WARNING, VK_VALIDATION_LEVEL_0, pMems[i], 0, MEMTRACK_INVALID_MEM_REF, "MEM", str);
                } else {
                    // Add to queue's memory reference list
                    pQueueInfo->pMemRefList.push_front(pMems[i]);
                }
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueRemoveMemReferences(VkQueue queue, uint32_t count, const VkDeviceMemory* pMems)
{
    // TODO : Decrement ref count for this memory reference on this queue. Remove if ref count is zero.
    VkResult result = nextTable.QueueRemoveMemReferences(queue, count, pMems);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);

        MT_QUEUE_INFO *pQueueInfo = queueMap[queue];
        if (pQueueInfo == NULL) {
            char str[1024];
            sprintf(str, "Unknown Queue %p", queue);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, queue, 0, MEMTRACK_INVALID_QUEUE, "MEM", str);
        } else {
            for (int i = 0; i < count; i++) {
                for (list<VkDeviceMemory>::iterator it = pQueueInfo->pMemRefList.begin(); it != pQueueInfo->pMemRefList.end(); ++it) {
                    if ((*it) == pMems[i]) {
                        it = pQueueInfo->pMemRefList.erase(it);
                    }
                }
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueSubmit(
    VkQueue             queue,
    uint32_t              cmdBufferCount,
    const VkCmdBuffer *pCmdBuffers,
    VkFence             fence)
{
    loader_platform_thread_lock_mutex(&globalLock);
    // TODO : Need to track fence and clear mem references when fence clears
    MT_CB_INFO* pCBInfo = NULL;
    uint64_t    fenceId = addFenceInfo(fence, queue);

    printMemList();
    printCBList();
    for (uint32_t i = 0; i < cmdBufferCount; i++) {
        pCBInfo = getCBInfo(pCmdBuffers[i]);
        pCBInfo->fenceId = fenceId;
    }

    if (VK_FALSE == validateQueueMemRefs(queue, cmdBufferCount, pCmdBuffers)) {
        char str[1024];
        sprintf(str, "Unable to verify memory references for Queue %p", queue);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, queue, 0, MEMTRACK_INVALID_MEM_REF, "MEM", str);
    }

    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = nextTable.QueueSubmit(queue, cmdBufferCount, pCmdBuffers, getFenceFromId(fenceId));
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocMemory(VkDevice device, const VkMemoryAllocInfo* pAllocInfo, VkDeviceMemory* pMem)
{
    VkResult result = nextTable.AllocMemory(device, pAllocInfo, pMem);
    // TODO : Track allocations and overall size here
    loader_platform_thread_lock_mutex(&globalLock);
    addMemObjInfo(*pMem, pAllocInfo);
    printMemList();
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkFreeMemory(VkDeviceMemory mem)
{
    /* From spec : A memory object is freed by calling vkFreeMemory() when it is no longer needed. Before
     * freeing a memory object, an application must ensure the memory object is unbound from
     * all API objects referencing it and that it is not referenced by any queued command buffers
     */
    loader_platform_thread_lock_mutex(&globalLock);
    if (VK_FALSE == freeMemObjInfo(mem, false)) {
        char str[1024];
        sprintf(str, "Issue while freeing mem obj %p", (void*)mem);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_FREE_MEM_ERROR, "MEM", str);
    }
    printMemList();
    printObjList();
    printCBList();
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = nextTable.FreeMemory(mem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkSetMemoryPriority(VkDeviceMemory mem, VkMemoryPriority priority)
{
    // TODO : Update tracking for this alloc
    //  Make sure memory is not pinned, which can't have priority set
    VkResult result = nextTable.SetMemoryPriority(mem, priority);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkMapMemory(VkDeviceMemory mem, VkFlags flags, void** ppData)
{
    // TODO : Track when memory is mapped
    loader_platform_thread_lock_mutex(&globalLock);
    MT_MEM_OBJ_INFO *pMemObj = getMemObjInfo(mem);
    if ((pMemObj->allocInfo.memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
        char str[1024];
        sprintf(str, "Mapping Memory (%p) without VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT set", (void*)mem);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_INVALID_STATE, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = nextTable.MapMemory(mem, flags, ppData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkUnmapMemory(VkDeviceMemory mem)
{
    // TODO : Track as memory gets unmapped, do we want to check what changed following map?
    //   Make sure that memory was ever mapped to begin with
    VkResult result = nextTable.UnmapMemory(mem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkPinSystemMemory(VkDevice device, const void* pSysMem, size_t memSize, VkDeviceMemory* pMem)
{
    // TODO : Track this
    //  Verify that memory is actually pinnable
    VkResult result = nextTable.PinSystemMemory(device, pSysMem, memSize, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenSharedMemory(VkDevice device, const VkMemoryOpenInfo* pOpenInfo, VkDeviceMemory* pMem)
{
    // TODO : Track this
    VkResult result = nextTable.OpenSharedMemory(device, pOpenInfo, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenPeerMemory(VkDevice device, const VkPeerMemoryOpenInfo* pOpenInfo, VkDeviceMemory* pMem)
{
    // TODO : Track this
    VkResult result = nextTable.OpenPeerMemory(device, pOpenInfo, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenPeerImage(VkDevice device, const VkPeerImageOpenInfo* pOpenInfo, VkImage* pImage, VkDeviceMemory* pMem)
{
    // TODO : Track this
    VkResult result = nextTable.OpenPeerImage(device, pOpenInfo, pImage, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyObject(VkObject object)
{
    loader_platform_thread_lock_mutex(&globalLock);

    // First check if this is a CmdBuffer
    if (NULL != getCBInfo((VkCmdBuffer)object)) {
        deleteCBInfo((VkCmdBuffer)object);
    }

    if (objectMap.find(object) != objectMap.end()) {
        MT_OBJ_INFO* pDelInfo = objectMap[object];
        if (pDelInfo->pMemObjInfo) {
            // Wsi allocated Memory is tied to image object so clear the binding and free that memory automatically
            if (0 == pDelInfo->pMemObjInfo->allocInfo.allocationSize) { // Wsi allocated memory has NULL allocInfo w/ 0 size
                VkDeviceMemory memToFree = pDelInfo->pMemObjInfo->mem;
                clearObjectBinding(object);
                freeMemObjInfo(memToFree, true);
            }
            else {
                char str[1024];
                sprintf(str, "Destroying obj %p that is still bound to memory object %p\nYou should first clear binding by calling vkQueueBindObjectMemory(queue, %p, 0, VK_NULL_HANDLE, 0)", object, (void*)pDelInfo->pMemObjInfo->mem, object);
                layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, object, 0, MEMTRACK_DESTROY_OBJECT_ERROR, "MEM", str);
                // From the spec : If an object has previous memory binding, it is required to unbind memory from an API object before it is destroyed.
                clearObjectBinding(object);
            }
        }
        delete pDelInfo;
        objectMap.erase(object);
    }

    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = nextTable.DestroyObject(object);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetObjectInfo(VkBaseObject object, VkObjectInfoType infoType, size_t* pDataSize, void* pData)
{
    // TODO : What to track here?
    //   Could potentially save returned mem requirements and validate values passed into QueueBindObjectMemory for this object
    // From spec : The only objects that are guaranteed to have no external memory requirements are devices, queues, command buffers, shaders and memory objects.
    VkResult result = nextTable.GetObjectInfo(object, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindObjectMemory(VkQueue queue, VkObject object, uint32_t allocationIdx, VkDeviceMemory mem, VkDeviceSize offset)
{
    VkResult result = nextTable.QueueBindObjectMemory(queue, object, allocationIdx, mem, offset);
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    if (VK_FALSE == updateObjectBinding(object, mem)) {
        char str[1024];
        sprintf(str, "Unable to set object %p binding to mem obj %p", (void*)object, (void*)mem);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, object, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    printObjList();
    printMemList();
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, VkFence* pFence)
{
    VkResult result = nextTable.CreateFence(device, pCreateInfo, pFence);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pFence, pCreateInfo->sType, pCreateInfo, sizeof(VkFenceCreateInfo), "fence");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetFences(VkDevice device, uint32_t fenceCount, VkFence* pFences)
{
    VkResult result = nextTable.ResetFences(device, fenceCount, pFences);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        // Reset fence state in fenceCreateInfo structure
        for (uint32_t i = 0; i < fenceCount; i++) {
            MT_OBJ_INFO* pObjectInfo = getObjectInfo(pFences[i]);
            if (pObjectInfo != NULL) {
                // Validate fences in SIGNALED state
                if (!(pObjectInfo->create_info.fence_create_info.flags & VK_FENCE_CREATE_SIGNALED_BIT)) {
                    char str[1024];
                    sprintf(str, "Fence %p submitted to VkResetFences in UNSIGNALED STATE", pFences[i]);
                    layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pFences[i], 0, MEMTRACK_INVALID_FENCE_STATE, "MEM", str);
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

VK_LAYER_EXPORT VkResult VKAPI vkGetFenceStatus(VkFence fence)
{
    VkResult result = nextTable.GetFenceStatus(fence);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        updateFenceTracking(fence);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, bool32_t waitAll, uint64_t timeout)
{
    // Verify fence status of submitted fences
    for(uint32_t i = 0; i < fenceCount; i++) {
        MT_OBJ_INFO* pObjectInfo = getObjectInfo(pFences[i]);
        if (pObjectInfo != NULL) {
            if (pObjectInfo->create_info.fence_create_info.flags & VK_FENCE_CREATE_SIGNALED_BIT) {
                char str[1024];
                sprintf(str, "VkWaitForFences specified fence %p already in SIGNALED state.", pFences[i]);
                layerCbMsg(VK_DBG_MSG_WARNING, VK_VALIDATION_LEVEL_0, pFences[i], 0, MEMTRACK_INVALID_FENCE_STATE, "MEM", str);
            }
        }
    }

    VkResult result = nextTable.WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    loader_platform_thread_lock_mutex(&globalLock);

    if (VK_SUCCESS == result) {
        if (waitAll || fenceCount == 1) { // Clear all the fences
            for(uint32_t i = 0; i < fenceCount; i++) {
                updateFenceTracking(pFences[i]);
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueWaitIdle(VkQueue queue)
{
    VkResult result = nextTable.QueueWaitIdle(queue);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        retireQueueFences(queue);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDeviceWaitIdle(VkDevice device)
{
    VkResult result = nextTable.DeviceWaitIdle(device);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        retireDeviceFences(device);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, VkEvent* pEvent)
{
    VkResult result = nextTable.CreateEvent(device, pCreateInfo, pEvent);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pEvent, pCreateInfo->sType, pCreateInfo, sizeof(VkEventCreateInfo), "event");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, VkQueryPool* pQueryPool)
{
    VkResult result = nextTable.CreateQueryPool(device, pCreateInfo, pQueryPool);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pQueryPool, pCreateInfo->sType, pCreateInfo, sizeof(VkQueryPoolCreateInfo), "query_pool");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, VkBuffer* pBuffer)
{
    VkResult result = nextTable.CreateBuffer(device, pCreateInfo, pBuffer);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pBuffer, pCreateInfo->sType, pCreateInfo, sizeof(VkBufferCreateInfo), "buffer");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, VkBufferView* pView)
{
    VkResult result = nextTable.CreateBufferView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pView, pCreateInfo->sType, pCreateInfo, sizeof(VkBufferViewCreateInfo), "buffer_view");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, VkImage* pImage)
{
    VkResult result = nextTable.CreateImage(device, pCreateInfo, pImage);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pImage, pCreateInfo->sType, pCreateInfo, sizeof(VkImageCreateInfo), "image");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, VkImageView* pView)
{
    VkResult result = nextTable.CreateImageView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pView, pCreateInfo->sType, pCreateInfo, sizeof(VkImageViewCreateInfo), "image_view");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateColorAttachmentView(VkDevice device, const VkColorAttachmentViewCreateInfo* pCreateInfo,
    VkColorAttachmentView* pView)
{
    VkResult result = nextTable.CreateColorAttachmentView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pView, pCreateInfo->sType, pCreateInfo, sizeof(VkColorAttachmentViewCreateInfo), "color_attachment_view");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDepthStencilView(VkDevice device, const VkDepthStencilViewCreateInfo* pCreateInfo, VkDepthStencilView* pView)
{
    VkResult result = nextTable.CreateDepthStencilView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pView, pCreateInfo->sType, pCreateInfo, sizeof(VkDepthStencilViewCreateInfo), "ds_view");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateShader(VkDevice device, const VkShaderCreateInfo* pCreateInfo, VkShader* pShader)
{
    VkResult result = nextTable.CreateShader(device, pCreateInfo, pShader);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipeline(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline)
{
    VkResult result = nextTable.CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pPipeline, pCreateInfo->sType, pCreateInfo, sizeof(VkGraphicsPipelineCreateInfo), "graphics_pipeline");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipelineDerivative(
        VkDevice device,
        const VkGraphicsPipelineCreateInfo* pCreateInfo,
        VkPipeline basePipeline,
        VkPipeline* pPipeline)
{
    VkResult result = nextTable.CreateGraphicsPipelineDerivative(device, pCreateInfo, basePipeline, pPipeline);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pPipeline, pCreateInfo->sType, pCreateInfo, sizeof(VkGraphicsPipelineCreateInfo), "graphics_pipeline");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateComputePipeline(VkDevice device, const VkComputePipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline)
{
    VkResult result = nextTable.CreateComputePipeline(device, pCreateInfo, pPipeline);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pPipeline, pCreateInfo->sType, pCreateInfo, sizeof(VkComputePipelineCreateInfo), "compute_pipeline");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, VkSampler* pSampler)
{
    VkResult result = nextTable.CreateSampler(device, pCreateInfo, pSampler);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pSampler, pCreateInfo->sType, pCreateInfo, sizeof(VkSamplerCreateInfo), "sampler");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicViewportState(VkDevice device, const VkDynamicVpStateCreateInfo* pCreateInfo,
    VkDynamicVpState* pState)
{
    VkResult result = nextTable.CreateDynamicViewportState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pState, pCreateInfo->sType, pCreateInfo, sizeof(VkDynamicVpStateCreateInfo), "viewport_state");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicRasterState(VkDevice device, const VkDynamicRsStateCreateInfo* pCreateInfo,
    VkDynamicRsState* pState)
{
    VkResult result = nextTable.CreateDynamicRasterState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pState, pCreateInfo->sType, pCreateInfo, sizeof(VkDynamicRsStateCreateInfo), "raster_state");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicColorBlendState(VkDevice device, const VkDynamicCbStateCreateInfo* pCreateInfo,
    VkDynamicCbState*  pState)
{
    VkResult result = nextTable.CreateDynamicColorBlendState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pState, pCreateInfo->sType, pCreateInfo, sizeof(VkDynamicCbStateCreateInfo), "cb_state");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicDepthStencilState(VkDevice device, const VkDynamicDsStateCreateInfo* pCreateInfo,
    VkDynamicDsState*    pState)
{
    VkResult result = nextTable.CreateDynamicDepthStencilState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        addObjectInfo(*pState, pCreateInfo->sType, pCreateInfo, sizeof(VkDynamicDsStateCreateInfo), "ds_state");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateCommandBuffer(VkDevice device, const VkCmdBufferCreateInfo* pCreateInfo, VkCmdBuffer* pCmdBuffer)
{
    VkResult result = nextTable.CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    // At time of cmd buffer creation, create global cmd buffer info for the returned cmd buffer
    loader_platform_thread_lock_mutex(&globalLock);
    if (*pCmdBuffer)
        addCBInfo(*pCmdBuffer);
    printCBList();
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBeginCommandBuffer(VkCmdBuffer cmdBuffer, const VkCmdBufferBeginInfo* pBeginInfo)
{
    // This implicitly resets the Cmd Buffer so make sure any fence is done and then clear memory references
    MT_CB_INFO* pCBInfo = getCBInfo(cmdBuffer);
    if (pCBInfo && (!fenceRetired(pCBInfo->fenceId))) {
        bool32_t cbDone = checkCBCompleted(cmdBuffer);
        if (VK_FALSE == cbDone) {
            char str[1024];
            sprintf(str, "Calling vkBeginCommandBuffer() on active CB %p before it has completed. You must check CB flag before this call.", cmdBuffer);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM", str);
        }
    }
    VkResult result = nextTable.BeginCommandBuffer(cmdBuffer, pBeginInfo);
    loader_platform_thread_lock_mutex(&globalLock);
    freeCBBindings(cmdBuffer);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkEndCommandBuffer(VkCmdBuffer cmdBuffer)
{
    // TODO : Anything to do here?
    VkResult result = nextTable.EndCommandBuffer(cmdBuffer);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetCommandBuffer(VkCmdBuffer cmdBuffer)
{
    // Verify that CB is complete (not in-flight)
    MT_CB_INFO* pCBInfo = getCBInfo(cmdBuffer);
    if (pCBInfo && (!fenceRetired(pCBInfo->fenceId))) {
        bool32_t cbDone = checkCBCompleted(cmdBuffer);
        if (VK_FALSE == cbDone) {
            char str[1024];
            sprintf(str, "Resetting CB %p before it has completed. You must check CB flag before calling vkResetCommandBuffer().", cmdBuffer);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM", str);
        }
    }
    // Clear memory references as this point.
    loader_platform_thread_lock_mutex(&globalLock);
    freeCBBindings(cmdBuffer);
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = nextTable.ResetCommandBuffer(cmdBuffer);
    return result;
}
// TODO : For any vkCmdBind* calls that include an object which has mem bound to it,
//    need to account for that mem now having binding to given cmdBuffer
VK_LAYER_EXPORT void VKAPI vkCmdBindPipeline(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
#if 0
    // TODO : If memory bound to pipeline, then need to tie that mem to cmdBuffer
    if (getPipeline(pipeline)) {
        MT_CB_INFO *pCBInfo = getCBInfo(cmdBuffer);
        if (pCBInfo) {
            pCBInfo->pipelines[pipelineBindPoint] = pipeline;
        } else {
            char str[1024];
            sprintf(str, "Attempt to bind Pipeline %p to non-existant command buffer %p!", (void*)pipeline, cmdBuffer);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_INVALID_CB, (char *) "DS", (char *) str);
        }
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to bind Pipeline %p that doesn't exist!", (void*)pipeline);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, pipeline, 0, MEMTRACK_INVALID_OBJECT, (char *) "DS", (char *) str);
    }
#endif
    nextTable.CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDynamicStateObject(VkCmdBuffer cmdBuffer, VkStateBindPoint stateBindPoint, VkDynamicStateObject state)
{
    MT_OBJ_INFO *pObjInfo;
    loader_platform_thread_lock_mutex(&globalLock);
    MT_CB_INFO *pCmdBuf = getCBInfo(cmdBuffer);
    if (!pCmdBuf) {
        char str[1024];
        sprintf(str, "Unable to find command buffer object %p, was it ever created?", (void*)cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_INVALID_CB, "DD", str);
    }
    pObjInfo = getObjectInfo(state);
    if (!pObjInfo) {
        char str[1024];
        sprintf(str, "Unable to find dynamic state object %p, was it ever created?", (void*)state);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, state, 0, MEMTRACK_INVALID_OBJECT, "DD", str);
    }
    pCmdBuf->pDynamicState[stateBindPoint] = pObjInfo;
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdBindDynamicStateObject(cmdBuffer, stateBindPoint, state);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDescriptorSets(
    VkCmdBuffer                              cmdBuffer,
    VkPipelineBindPoint                     pipelineBindPoint,
    VkDescriptorSetLayoutChain             layoutChain,
    uint32_t                                    layoutChainSlot,
    uint32_t                                    count,
    const VkDescriptorSet*                   pDescriptorSets,
    const uint32_t*                             pUserData)
{
    // TODO : Somewhere need to verify that all textures referenced by shaders in DS are in some type of *SHADER_READ* state
    nextTable.CmdBindDescriptorSets(cmdBuffer, pipelineBindPoint, layoutChain, layoutChainSlot, count, pDescriptorSets, pUserData);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindVertexBuffers(
    VkCmdBuffer                                 cmdBuffer,
    uint32_t                                    startBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                         pOffsets)
{
    nextTable.CmdBindVertexBuffers(cmdBuffer, startBinding, bindingCount, pBuffers, pOffsets);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindIndexBuffer(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    nextTable.CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(buffer);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdDrawIndirect() call unable to update binding of buffer %p to cmdBuffer %p", buffer, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(buffer);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdDrawIndexedIndirect() call unable to update binding of buffer %p to cmdBuffer %p", buffer, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatchIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(buffer);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdDispatchIndirect() call unable to update binding of buffer %p to cmdBuffer %p", buffer, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBuffer(VkCmdBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer destBuffer,
    uint32_t regionCount, const VkBufferCopy* pRegions)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(srcBuffer);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdCopyBuffer() call unable to update binding of srcBuffer %p to cmdBuffer %p", srcBuffer, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    mem = getMemBindingFromObject(destBuffer);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdCopyBuffer() call unable to update binding of destBuffer %p to cmdBuffer %p", destBuffer, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImage(VkCmdBuffer cmdBuffer,
                                             VkImage srcImage, VkImageLayout srcImageLayout,
                                             VkImage destImage, VkImageLayout destImageLayout,
                                             uint32_t regionCount, const VkImageCopy* pRegions)
{
    // TODO : Each image will have mem mapping so track them
    nextTable.CmdCopyImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdBlitImage(VkCmdBuffer cmdBuffer,
                                             VkImage srcImage, VkImageLayout srcImageLayout,
                                             VkImage destImage, VkImageLayout destImageLayout,
                                             uint32_t regionCount, const VkImageBlit* pRegions)
{
    // TODO : Each image will have mem mapping so track them
    nextTable.CmdBlitImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBufferToImage(VkCmdBuffer cmdBuffer,
                                                     VkBuffer srcBuffer,
                                                     VkImage destImage, VkImageLayout destImageLayout,
                                                     uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    // TODO : Track this
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(destImage);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdCopyMemoryToImage() call unable to update binding of destImage buffer %p to cmdBuffer %p", destImage, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }

    mem = getMemBindingFromObject(srcBuffer);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdCopyMemoryToImage() call unable to update binding of srcBuffer %p to cmdBuffer %p", srcBuffer, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImageToBuffer(VkCmdBuffer cmdBuffer,
                                                     VkImage srcImage, VkImageLayout srcImageLayout,
                                                     VkBuffer destBuffer,
                                                     uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    // TODO : Track this
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(srcImage);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdCopyImageToMemory() call unable to update binding of srcImage buffer %p to cmdBuffer %p", srcImage, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    mem = getMemBindingFromObject(destBuffer);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdCopyImageToMemory() call unable to update binding of destBuffer %p to cmdBuffer %p", destBuffer, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdCopyImageToBuffer(cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCloneImageData(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
    VkImage destImage, VkImageLayout destImageLayout)
{
    // TODO : Each image will have mem mapping so track them
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(srcImage);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdCloneImageData() call unable to update binding of srcImage buffer %p to cmdBuffer %p", srcImage, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    mem = getMemBindingFromObject(destImage);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdCloneImageData() call unable to update binding of destImage buffer %p to cmdBuffer %p", destImage, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdCloneImageData(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout);
}

VK_LAYER_EXPORT void VKAPI vkCmdUpdateBuffer(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize dataSize, const uint32_t* pData)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(destBuffer);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdUpdateMemory() call unable to update binding of destBuffer %p to cmdBuffer %p", destBuffer, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdFillBuffer(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize fillSize, uint32_t data)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(destBuffer);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdFillMemory() call unable to update binding of destBuffer %p to cmdBuffer %p", destBuffer, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorImage(VkCmdBuffer cmdBuffer,
                                                   VkImage image, VkImageLayout imageLayout,
                                                   VkClearColor color,
                                                   uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    // TODO : Verify memory is in VK_IMAGE_STATE_CLEAR state
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(image);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdClearColorImage() call unable to update binding of image buffer %p to cmdBuffer %p", image, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdClearColorImage(cmdBuffer, image, imageLayout, color, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencil(VkCmdBuffer cmdBuffer,
                                                     VkImage image, VkImageLayout imageLayout,
                                                     float depth, uint32_t stencil,
                                                     uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    // TODO : Verify memory is in VK_IMAGE_STATE_CLEAR state
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(image);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdClearDepthStencil() call unable to update binding of image buffer %p to cmdBuffer %p", image, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdClearDepthStencil(cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdResolveImage(VkCmdBuffer cmdBuffer,
                                                VkImage srcImage, VkImageLayout srcImageLayout,
                                                VkImage destImage, VkImageLayout destImageLayout,
                                                uint32_t regionCount, const VkImageResolve* pRegions)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(srcImage);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdResolveImage() call unable to update binding of srcImage buffer %p to cmdBuffer %p", srcImage, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    mem = getMemBindingFromObject(destImage);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdResolveImage() call unable to update binding of destImage buffer %p to cmdBuffer %p", destImage, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdResolveImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginQuery(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot, VkFlags flags)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(queryPool);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdBeginQuery() call unable to update binding of queryPool buffer %p to cmdBuffer %p", queryPool, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndQuery(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(queryPool);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdEndQuery() call unable to update binding of queryPool buffer %p to cmdBuffer %p", queryPool, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdEndQuery(cmdBuffer, queryPool, slot);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetQueryPool(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = getMemBindingFromObject(queryPool);
    if (VK_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In vkCmdResetQueryPool() call unable to update binding of queryPool buffer %p to cmdBuffer %p", queryPool, cmdBuffer);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgRegisterMsgCallback(VkInstance instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData)
{
    // This layer intercepts callbacks
    VK_LAYER_DBG_FUNCTION_NODE *pNewDbgFuncNode = (VK_LAYER_DBG_FUNCTION_NODE*)malloc(sizeof(VK_LAYER_DBG_FUNCTION_NODE));
    if (!pNewDbgFuncNode)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;
    pNewDbgFuncNode->pUserData = pUserData;
    pNewDbgFuncNode->pNext = g_pDbgFunctionHead;
    g_pDbgFunctionHead = pNewDbgFuncNode;
    // force callbacks if DebugAction hasn't been set already other than initial value
    if (g_actionIsDefault) {
        g_debugAction = VK_DBG_LAYER_ACTION_CALLBACK;
    }
    VkResult result = nextTable.DbgRegisterMsgCallback(instance, pfnMsgCallback, pUserData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgUnregisterMsgCallback(VkInstance instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
{
    VK_LAYER_DBG_FUNCTION_NODE *pInfo = g_pDbgFunctionHead;
    VK_LAYER_DBG_FUNCTION_NODE *pPrev = pInfo;
    while (pInfo) {
        if (pInfo->pfnMsgCallback == pfnMsgCallback) {
            pPrev->pNext = pInfo->pNext;
            if (g_pDbgFunctionHead == pInfo)
                g_pDbgFunctionHead = pInfo->pNext;
            free(pInfo);
            break;
        }
        pPrev = pInfo;
        pInfo = pInfo->pNext;
    }
    if (g_pDbgFunctionHead == NULL)
    {
        if (g_actionIsDefault) {
            g_debugAction = VK_DBG_LAYER_ACTION_LOG_MSG;
        } else {
            g_debugAction = (VK_LAYER_DBG_ACTION)(g_debugAction & ~((uint32_t)VK_DBG_LAYER_ACTION_CALLBACK));
        }
    }
    VkResult result = nextTable.DbgUnregisterMsgCallback(instance, pfnMsgCallback);
    return result;
}

#if !defined(WIN32)
VK_LAYER_EXPORT VkResult VKAPI vkWsiX11CreatePresentableImage(VkDevice device, const VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
    VkImage* pImage, VkDeviceMemory* pMem)
{
    VkResult result = nextTable.WsiX11CreatePresentableImage(device, pCreateInfo, pImage, pMem);
    loader_platform_thread_lock_mutex(&globalLock);
    if (VK_SUCCESS == result) {
        // Add image object, then insert the new Mem Object and then bind it to created image
        addObjectInfo(*pImage, VK_STRUCTURE_TYPE_MAX_ENUM, pCreateInfo, sizeof(VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO), "wsi_x11_image");
        addMemObjInfo(*pMem, NULL);
        if (VK_FALSE == updateObjectBinding(*pImage, *pMem)) {
            char str[1024];
            sprintf(str, "In vkWsiX11CreatePresentableImage(), unable to set image %p binding to mem obj %p", (void*)*pImage, (void*)*pMem);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, *pImage, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
        }
    }
    printObjList();
    printMemList();
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkWsiX11QueuePresent(VkQueue queue, const VK_WSI_X11_PRESENT_INFO*  pPresentInfo, VkFence fence)
{
    loader_platform_thread_lock_mutex(&globalLock);
    addFenceInfo(fence, queue);
    char            str[1024];
    sprintf(str, "In vkWsiX11QueuePresent(), checking queue %p for fence %p", queue, fence);
    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, queue, 0, MEMTRACK_NONE, "MEM", str);
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = nextTable.WsiX11QueuePresent(queue, pPresentInfo, fence);
    return result;
}
#endif // WIN32

VK_LAYER_EXPORT void* VKAPI vkGetProcAddr(VkPhysicalDevice gpu, const char* funcName)
{
    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;

    if (gpu == NULL)
        return NULL;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initMemTracker);

    if (!strcmp(funcName, "vkGetProcAddr"))
        return (void *) vkGetProcAddr;
    if (!strcmp(funcName, "vkCreateDevice"))
        return (void*) vkCreateDevice;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (void*) vkDestroyDevice;
    if (!strcmp(funcName, "vkEnumerateLayers"))
        return (void*) vkEnumerateLayers;
    if (!strcmp(funcName, "vkQueueSubmit"))
        return (void*) vkQueueSubmit;
    if (!strcmp(funcName, "vkAllocMemory"))
        return (void*) vkAllocMemory;
    if (!strcmp(funcName, "vkFreeMemory"))
        return (void*) vkFreeMemory;
    if (!strcmp(funcName, "vkSetMemoryPriority"))
        return (void*) vkSetMemoryPriority;
    if (!strcmp(funcName, "vkMapMemory"))
        return (void*) vkMapMemory;
    if (!strcmp(funcName, "vkUnmapMemory"))
        return (void*) vkUnmapMemory;
    if (!strcmp(funcName, "vkPinSystemMemory"))
        return (void*) vkPinSystemMemory;
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
    if (!strcmp(funcName, "vkQueueBindObjectMemory"))
        return (void*) vkQueueBindObjectMemory;
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
    if (!strcmp(funcName, "vkCmdCloneImageData"))
        return (void*) vkCmdCloneImageData;
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
    if (!strcmp(funcName, "vkDbgRegisterMsgCallback"))
        return (void*) vkDbgRegisterMsgCallback;
    if (!strcmp(funcName, "vkDbgUnregisterMsgCallback"))
        return (void*) vkDbgUnregisterMsgCallback;
    if (!strcmp(funcName, "vkGetDeviceQueue"))
        return (void*) vkGetDeviceQueue;
    if (!strcmp(funcName, "vkQueueAddMemReferences"))
        return (void*) vkQueueAddMemReferences;
    if (!strcmp(funcName, "vkQueueRemoveMemReferences"))
        return (void*) vkQueueRemoveMemReferences;
#if !defined(WIN32)
    if (!strcmp(funcName, "vkWsiX11CreatePresentableImage"))
        return (void*) vkWsiX11CreatePresentableImage;
    if (!strcmp(funcName, "vkWsiX11QueuePresent"))
        return (void*) vkWsiX11QueuePresent;
#endif
    else {
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((VkPhysicalDevice)gpuw->nextObject, funcName);
    }
}
