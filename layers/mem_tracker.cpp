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
#include "xgl_dispatch_table_helper.h"
#include "xgl_struct_string_helper_cpp.h"
#include "mem_tracker.h"
#include "layers_config.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"
#include "layers_msg.h"

static XGL_LAYER_DISPATCH_TABLE nextTable;
static XGL_BASE_LAYER_OBJECT *pCurObj;
static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(g_initOnce);
// TODO : This can be much smarter, using separate locks for separate global data
static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;

#define MAX_BINDING 0xFFFFFFFF

map<XGL_CMD_BUFFER, GLOBAL_CB_NODE*>      cbMap;
map<XGL_GPU_MEMORY, GLOBAL_MEM_OBJ_NODE*> memObjMap;
map<XGL_OBJECT,     GLOBAL_OBJECT_NODE*>  objectMap;
map<uint64_t,       GLOBAL_FENCE_NODE*>   fenceMap;    // Map fenceId to fence node

// TODO : Add support for per-queue and per-device fence completion
static uint64_t     g_currentFenceId  = 1;
static uint64_t     g_lastRetiredId   = 0;
static XGL_DEVICE   globalDevice      = NULL;

// Add new CB node for this cb to map container
static void insertGlobalCB(const XGL_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pTrav = new GLOBAL_CB_NODE;
    memset(pTrav, 0, (sizeof(GLOBAL_CB_NODE) - sizeof(list<XGL_GPU_MEMORY>)));
    pTrav->cmdBuffer = cb;
    cbMap[cb] = pTrav;
}

// Return ptr to node in CB map, or NULL if not found
static GLOBAL_CB_NODE* getGlobalCBNode(const XGL_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pCBNode = NULL;
    if (cbMap.find(cb) != cbMap.end()) {
        pCBNode = cbMap[cb];
    }
    return pCBNode;
}

// Add a fence, creating one if necessary to our list of fences/fenceIds
static uint64_t addFenceNode(XGL_FENCE fence)
{
    // Create fence node
    GLOBAL_FENCE_NODE* pFenceNode = new GLOBAL_FENCE_NODE;
    memset(pFenceNode, 0, sizeof(GLOBAL_FENCE_NODE));
    // If no fence, create an internal fence to track the submissions
    if (fence == NULL) {
        XGL_FENCE_CREATE_INFO fci;
        fci.sType = XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fci.pNext = NULL;
        fci.flags = 0;
        nextTable.CreateFence(globalDevice, &fci, &pFenceNode->fence);
        pFenceNode->localFence = XGL_TRUE;
    } else {
        pFenceNode->localFence = XGL_FALSE;
        pFenceNode->fence      = fence;
    }
    uint64_t fenceId  = g_currentFenceId++;
    fenceMap[fenceId] = pFenceNode;
    return fenceId;
}

// Remove a node from our list of fences/fenceIds
static void deleteFenceNode(uint64_t fenceId)
{
    if (fenceId != 0) {
        if (fenceMap.find(fenceId) != fenceMap.end()) {
            GLOBAL_FENCE_NODE* pDelNode = fenceMap[fenceId];
            if (pDelNode->localFence == XGL_TRUE) {
                nextTable.DestroyObject(pDelNode->fence);
            }
            delete pDelNode;
            fenceMap.erase(fenceId);
        }
    }
}

// Search through list for this fence, deleting all nodes before it (with lower IDs) and updating lastRetiredId
static void updateFenceTracking(XGL_FENCE fence)
{
    GLOBAL_FENCE_NODE *pCurFenceNode = NULL;
    uint64_t           fenceId       = 0;

    for (map<uint64_t, GLOBAL_FENCE_NODE*>::iterator ii=fenceMap.begin(); ii!=fenceMap.end(); ++ii) {
        if (fence == ((*ii).second)->fence) {
            g_lastRetiredId = (*ii).first;
        } else {
            deleteFenceNode((*ii).first);
        }
    }
}

// Utility function that determines if a fenceId has been retired yet
static bool32_t fenceRetired(uint64_t fenceId)
{
    bool32_t result = XGL_FALSE;
    if (fenceId <= g_lastRetiredId) {
        result = XGL_TRUE;
    }
    return result;
}

// Return the fence associated with a fenceId
static XGL_FENCE getFenceFromId(uint64_t fenceId)
{
    XGL_FENCE fence = NULL;
    if (fenceId != 0) {
        // Search for a node with this fenceId
        if (fenceId > g_lastRetiredId) {
            if (fenceMap.find(fenceId) != fenceMap.end()) {
                fence = (fenceMap[fenceId])->fence;
            }
        }
    }
    return fence;
}

// Helper routine that updates the fence list to all-retired, as for Queue/DeviceWaitIdle
static void retireAllFences(void)
{
    // In this case, we go throught the whole list, retiring each node and update the global retired ID until the list is empty
    GLOBAL_FENCE_NODE* pDelNode = NULL;
    for (map<uint64_t, GLOBAL_FENCE_NODE*>::iterator ii=fenceMap.begin(); ii!=fenceMap.end(); ++ii) {
        g_lastRetiredId = (*ii).first;
        deleteFenceNode((*ii).first);
    }
}

static bool32_t validateCBMemRef(const XGL_CMD_BUFFER cb, uint32_t memRefCount, const XGL_MEMORY_REF* pMemRefs)
{
    bool32_t result = XGL_TRUE;
    GLOBAL_CB_NODE* pTrav = getGlobalCBNode(cb);
    if (!pTrav) {
        char str[1024];
        sprintf(str, "Unable to find node for CB %p in order to check memory references", (void*)cb);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_CB, "MEM", str);
        result = XGL_FALSE;
    } else {
        // Validate that all actual references are accounted for in pMemRefs
        uint32_t i;
        uint8_t  found = 0;
        uint64_t foundCount = 0;

        for (list<XGL_GPU_MEMORY>::iterator it = pTrav->pMemObjList.begin(); it != pTrav->pMemObjList.end(); ++it) {
            for (i = 0; i < memRefCount; i++) {
                if ((*it) == pMemRefs[i].mem) {
                    char str[1024];
                    sprintf(str, "Found Mem Obj %p binding to CB %p", (*it), cb);
                    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_NONE, "MEM", str);
                    found = 1;
                    foundCount++;
                    break;
                }
            }
            if (!found) {
                char str[1024];
                sprintf(str, "Memory reference list for Command Buffer %p is missing ref to mem obj %p", cb, (*it));
                layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_CB_MISSING_MEM_REF, "MEM", str);
                result = XGL_FALSE;
            }
            found = 0;
        }

        if (result == XGL_TRUE) {
            char str[1024];
            sprintf(str, "Verified all %lu memory dependencies for CB %p are included in pMemRefs list", foundCount, cb);
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_NONE, "MEM", str);
            // TODO : Could report mem refs in pMemRefs that AREN'T in mem list, that would be primarily informational
            //   Currently just noting that there is a difference
            if (foundCount != memRefCount) {
                sprintf(str, "There are %u mem refs included in pMemRefs list, but only %lu are required", memRefCount, foundCount);
                layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_NONE, "MEM", str);
            }
        }
    }
    return result;
}
// Return ptr to node in map container containing mem, or NULL if not found
//  Calls to this function should be wrapped in mutex
static GLOBAL_MEM_OBJ_NODE* getGlobalMemNode(const XGL_GPU_MEMORY mem)
{
    GLOBAL_MEM_OBJ_NODE* pMemNode = NULL;

    if (memObjMap.find(mem) != memObjMap.end()) {
        pMemNode = memObjMap[mem];
    }
    return pMemNode;
}

static void insertGlobalMemObj(const XGL_GPU_MEMORY mem, const XGL_MEMORY_ALLOC_INFO* pAllocInfo)
{
    GLOBAL_MEM_OBJ_NODE* pTrav = new GLOBAL_MEM_OBJ_NODE;
    pTrav->refCount           = 0;
    memset(&pTrav->allocInfo, 0, sizeof(XGL_MEMORY_ALLOC_INFO));

    if (pAllocInfo) {  // MEM alloc created by xglWsiX11CreatePresentableImage() doesn't have alloc info struct
        memcpy(&pTrav->allocInfo, pAllocInfo, sizeof(XGL_MEMORY_ALLOC_INFO));
        // TODO:  Update for real hardware, actually process allocation info structures
        pTrav->allocInfo.pNext = NULL;
    }
    pTrav->mem = mem;
    memObjMap[mem] = pTrav;
}

// Find Global CB Node and add mem binding to list container
// Find Global Mem Obj Node and add CB binding to list container
static bool32_t updateCBBinding(const XGL_CMD_BUFFER cb, const XGL_GPU_MEMORY mem)
{
    bool32_t result = XGL_TRUE;
    // First update CB binding in MemObj mini CB list
    GLOBAL_MEM_OBJ_NODE* pMemTrav = getGlobalMemNode(mem);
    if (!pMemTrav) {
        char str[1024];
        sprintf(str, "Trying to bind mem obj %p to CB %p but no Node for that mem obj.\n    Was it correctly allocated? Did it already get freed?", mem, cb);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
        result = XGL_FALSE;
    } else {
        // Search for cmd buffer object in memory object's binding list
        bool32_t found  = XGL_FALSE;
        for (list<XGL_CMD_BUFFER>::iterator it = pMemTrav->pCmdBufferBindings.begin(); it != pMemTrav->pCmdBufferBindings.end(); ++it) {
            if ((*it) == cb) {
                found = XGL_TRUE;
                break;
            }
        }
        // If not present, add to list
        if (found == XGL_FALSE) {
            pMemTrav->pCmdBufferBindings.push_front(cb);
            pMemTrav->refCount++;
        }

        // Now update Global CB's Mem binding list
        GLOBAL_CB_NODE* pCBTrav = getGlobalCBNode(cb);
        if (!pCBTrav) {
            char str[1024];
            sprintf(str, "Trying to bind mem obj %p to CB %p but no Node for that CB.    Was it CB incorrectly destroyed?", mem, cb);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
            result = XGL_FALSE;
        } else {
            // Search for memory object in cmd buffer's binding list
            bool32_t found  = XGL_FALSE;
            for (list<XGL_GPU_MEMORY>::iterator it = pCBTrav->pMemObjList.begin(); it != pCBTrav->pMemObjList.end(); ++it) {
                if ((*it) == mem) {
                    found = XGL_TRUE;
                    break;
                }
            }
            // If not present, add to list
            if (found == XGL_FALSE) {
                pCBTrav->pMemObjList.push_front(mem);
            }
        }
    }
    return result;
}

// Clear the CB Binding for mem
//  Calls to this function should be wrapped in mutex
static void clearCBBinding(const XGL_CMD_BUFFER cb, const XGL_GPU_MEMORY mem)
{
    GLOBAL_MEM_OBJ_NODE* pTrav = getGlobalMemNode(mem);
    // TODO : Having this check is not ideal, really if mem node was deleted,
    //   its CB bindings should be cleared and then freeCBBindings wouldn't call
    //   us here with stale mem objs
    if (pTrav) {
        pTrav->pCmdBufferBindings.remove(cb);
        pTrav->refCount--;
    }
}

// Free bindings related to CB
static bool32_t freeCBBindings(const XGL_CMD_BUFFER cb)
{
    bool32_t result = XGL_TRUE;
    GLOBAL_CB_NODE* pCBTrav = getGlobalCBNode(cb);
    if (!pCBTrav) {
        char str[1024];
        sprintf(str, "Unable to find global CB node %p for deletion", cb);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_CB, "MEM", str);
        result = XGL_FALSE;
    } else {
        if (!fenceRetired(pCBTrav->fenceId)) {
            deleteFenceNode(pCBTrav->fenceId);
        }

        for (list<XGL_GPU_MEMORY>::iterator it=pCBTrav->pMemObjList.begin(); it!=pCBTrav->pMemObjList.end(); ++it) {
            clearCBBinding(cb, (*it));
        }
        pCBTrav->pMemObjList.clear();
    }
    return result;
}

// Delete Global CB Node from list along with all of it's mini mem obj node
//   and also clear Global mem references to CB
// TODO : When should this be called?  There's no Destroy of CBs that I see
static bool32_t deleteGlobalCBNode(const XGL_CMD_BUFFER cb)
{
    bool32_t result = XGL_TRUE;
    result = freeCBBindings(cb);
    // Delete the Global CB node
    if (result == XGL_TRUE) {
        if (cbMap.find(cb) != cbMap.end()) {
            GLOBAL_CB_NODE* pDelNode = cbMap[cb];
            delete pDelNode;
            cbMap.erase(cb);
        }
    }
    return result;
}

// Delete the entire CB list
static bool32_t deleteGlobalCBList()
{
    bool32_t result = XGL_TRUE;
    for (map<XGL_CMD_BUFFER, GLOBAL_CB_NODE*>::iterator ii=cbMap.begin(); ii!=cbMap.end(); ++ii) {
        freeCBBindings((*ii).first);
        delete (*ii).second;
    }
    return result;
}

// For given MemObj node, report Obj & CB bindings
static void reportMemReferences(const GLOBAL_MEM_OBJ_NODE* pMemObjTrav)
{
    uint32_t refCount = 0; // Count found references

    for (list<XGL_CMD_BUFFER>::const_iterator it = pMemObjTrav->pCmdBufferBindings.begin(); it != pMemObjTrav->pCmdBufferBindings.end(); ++it) {
        refCount++;
        char str[1024];
        sprintf(str, "Command Buffer %p has reference to mem obj %p", (*it), pMemObjTrav->mem);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, (*it), 0, MEMTRACK_NONE, "MEM", str);
    }
    for (list<XGL_OBJECT>::const_iterator it = pMemObjTrav->pObjBindings.begin(); it != pMemObjTrav->pObjBindings.end(); ++it) {
        char str[1024];
        sprintf(str, "XGL Object %p has reference to mem obj %p", (*it), pMemObjTrav->mem);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, (*it), 0, MEMTRACK_NONE, "MEM", str);
    }
    if (refCount != pMemObjTrav->refCount) {
        char str[1024];
        sprintf(str, "Refcount of %u for Mem Obj %p does't match reported refs of %u", pMemObjTrav->refCount, pMemObjTrav->mem, refCount);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pMemObjTrav->mem, 0, MEMTRACK_INTERNAL_ERROR, "MEM", str);
    }
}

static void deleteGlobalMemNode(XGL_GPU_MEMORY mem)
{
    GLOBAL_MEM_OBJ_NODE* pDelNode = memObjMap[mem];
    if (memObjMap.find(mem) != memObjMap.end()) {
        GLOBAL_MEM_OBJ_NODE* pDelNode = memObjMap[mem];
        delete pDelNode;
        memObjMap.erase(mem);
    }
}

// Check if fence for given CB is completed
static bool32_t checkCBCompleted(const XGL_CMD_BUFFER cb)
{
    bool32_t result = XGL_TRUE;
    GLOBAL_CB_NODE* pCBTrav = getGlobalCBNode(cb);
    if (!pCBTrav) {
        char str[1024];
        sprintf(str, "Unable to find global CB node %p to check for completion", cb);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_CB, "MEM", str);
        result = XGL_FALSE;
    } else {
        if (!fenceRetired(pCBTrav->fenceId)) {
            // Explicitly call the internal xglGetFenceStatus routine
            if (XGL_SUCCESS != xglGetFenceStatus(getFenceFromId(pCBTrav->fenceId))) {
                char str[1024];
                sprintf(str, "FenceId %" PRIx64", fence %p for CB %p has not completed", pCBTrav->fenceId, getFenceFromId(pCBTrav->fenceId), cb);
                layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_NONE, "MEM", str);
                result = XGL_FALSE;
            }
        }
    }
    return result;
}

static bool32_t freeMemNode(XGL_GPU_MEMORY mem)
{
    bool32_t result = XGL_TRUE;
    // Parse global list to find node w/ mem
    GLOBAL_MEM_OBJ_NODE* pTrav = getGlobalMemNode(mem);
    if (!pTrav) {
        char str[1024];
        sprintf(str, "Couldn't find mem node object for %p\n    Was %p never allocated or previously freed?", (void*)mem, (void*)mem);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
        result = XGL_FALSE;
    } else {
        if (pTrav->allocInfo.allocationSize == 0) {
            char str[1024];
            sprintf(str, "Attempting to free memory associated with a Presentable Image, %p, this should not be explicitly freed\n", (void*)mem);
            layerCbMsg(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
            result = XGL_FALSE;
        } else {
            // Clear any CB bindings for completed CBs
            //   TODO : Is there a better place to do this?

            list<XGL_CMD_BUFFER>::iterator it = pTrav->pCmdBufferBindings.begin();
            list<XGL_CMD_BUFFER>::iterator temp;
            while (it != pTrav->pCmdBufferBindings.end()) {
                if (XGL_TRUE == checkCBCompleted(*it)) {
                    temp = it;
                    ++temp;
                    freeCBBindings(*it);
                    it = temp;
                } else {
                    ++it;
                }
            }

            // Now verify that no references to this mem obj remain
            if (0 != pTrav->refCount) {
                // If references remain, report the error and can search CB list to find references
                char str[1024];
                sprintf(str, "Freeing mem obj %p while it still has references", (void*)mem);
                layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_FREED_MEM_REF, "MEM", str);
                reportMemReferences(pTrav);
                result = XGL_FALSE;
            }
            // Delete global node
            deleteGlobalMemNode(mem);
        }
    }
    return result;
}

// Return object node for 'object' or return NULL if no node exists
static GLOBAL_OBJECT_NODE* getGlobalObjectNode(const XGL_OBJECT object)
{
    GLOBAL_OBJECT_NODE* pObjNode = NULL;

    if (objectMap.find(object) != objectMap.end()) {
        pObjNode = objectMap[object];
    }
    return pObjNode;
}

static GLOBAL_OBJECT_NODE* insertGlobalObjectNode(XGL_OBJECT object, XGL_STRUCTURE_TYPE sType, const void *pCreateInfo, const int struct_size, const char *name_prefix)
{
    GLOBAL_OBJECT_NODE* pTrav = new GLOBAL_OBJECT_NODE;
    memset(pTrav, 0, sizeof(GLOBAL_OBJECT_NODE));
    memcpy(&pTrav->create_info, pCreateInfo, struct_size);
    sprintf(pTrav->object_name, "%s_%p", name_prefix, object);

    pTrav->object     = object;
    pTrav->ref_count  = 1;
    pTrav->sType      = sType;
    objectMap[object] = pTrav;

    return pTrav;
}

// Remove object binding performs 3 tasks:
// 1. Remove object node from Global Mem Obj list container of obj bindings & free it
// 2. Decrement refCount for Global Mem Obj
// 3. Clear Global Mem Obj ptr from Global Object Node
static bool32_t clearObjectBinding(XGL_OBJECT object)
{
    bool32_t result = XGL_FALSE;
    GLOBAL_OBJECT_NODE* pGlobalObjTrav = getGlobalObjectNode(object);
    if (!pGlobalObjTrav) {
        char str[1024];
        sprintf(str, "Attempting to clear mem binding for object %p: devices, queues, command buffers, shaders and memory objects do not have external memory requirements and it is unneccessary to call bind/unbindObjectMemory on them.", object);
        layerCbMsg(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_INVALID_OBJECT, "MEM", str);
    } else {
        if (!pGlobalObjTrav->pMemNode) {
            char str[1024];
            sprintf(str, "Attempting to clear mem binding on obj %p but it has no binding.", (void*)object);
            layerCbMsg(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_MEM_OBJ_CLEAR_EMPTY_BINDINGS, "MEM", str);
        } else {
            for (list<XGL_OBJECT>::iterator it = pGlobalObjTrav->pMemNode->pObjBindings.begin(); it != pGlobalObjTrav->pMemNode->pObjBindings.end(); ++it) {
                pGlobalObjTrav->pMemNode->refCount--;
                pGlobalObjTrav->pMemNode = NULL;
                it = pGlobalObjTrav->pMemNode->pObjBindings.erase(it);
                result = XGL_TRUE;
                break;
            }
            if (result == XGL_FALSE) {
                char str[1024];
                sprintf(str, "While trying to clear mem binding for object %p, unable to find that object referenced by mem obj %p",
                    object, pGlobalObjTrav->pMemNode->mem);
                layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_INTERNAL_ERROR, "MEM", str);
            }
        }
    }
    return result;
}

// For NULL mem case, clear any previous binding Else...
// Make sure given object is in global object map
//  IF a previous binding existed, clear it
//  Add reference from global object node to global memory node
//  Add reference off of global obj node
// Return XGL_TRUE if addition is successful, XGL_FALSE otherwise
static bool32_t updateObjectBinding(XGL_OBJECT object, XGL_GPU_MEMORY mem)
{
    bool32_t result = XGL_FALSE;
    // Handle NULL case separately, just clear previous binding & decrement reference
    if (mem == XGL_NULL_HANDLE) {
        clearObjectBinding(object);
        result = XGL_TRUE;
    } else {
        char str[1024];
        GLOBAL_OBJECT_NODE* pGlobalObjTrav = getGlobalObjectNode(object);
        if (!pGlobalObjTrav) {
            sprintf(str, "Attempting to update Binding of Obj(%p) that's not in global list()", (void*)object);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_INTERNAL_ERROR, "MEM", str);
            return XGL_FALSE;
        }
        // non-null case so should have real mem obj
        GLOBAL_MEM_OBJ_NODE* pTrav = getGlobalMemNode(mem);
        if (!pTrav) {
            sprintf(str, "While trying to bind mem for obj %p, couldn't find node for mem obj %p", (void*)object, (void*)mem);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
        } else {
            // Search for object in memory object's binding list
            bool32_t found  = XGL_FALSE;
            for (list<XGL_OBJECT>::iterator it = pTrav->pObjBindings.begin(); it != pTrav->pObjBindings.end(); ++it) {
                if ((*it) == object) {
                    found = XGL_TRUE;
                    break;
                }
            }
            // If not present, add to list
            if (found == XGL_FALSE) {
                pTrav->pObjBindings.push_front(object);
                pTrav->refCount++;
            }

            if (pGlobalObjTrav->pMemNode) {
                clearObjectBinding(object); // Need to clear the previous object binding before setting new binding
                sprintf(str, "Updating memory binding for object %p from mem obj %p to %p", object, pGlobalObjTrav->pMemNode->mem, mem);
                layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_NONE, "MEM", str);
            }
            // For image objects, make sure default memory state is correctly set
            // TODO : What's the best/correct way to handle this?
            if (XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO == pGlobalObjTrav->sType) {
                if (pGlobalObjTrav->create_info.image_create_info.usage & (XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT)) {
                    // TODO::  More memory state transition stuff.
                }
            }
            pGlobalObjTrav->pMemNode = pTrav;
        }
    }
    return XGL_TRUE;
}

// Print details of global Obj tracking list
static void printObjList()
{
    GLOBAL_OBJECT_NODE* pTrav = NULL;
    char str[1024];
    sprintf(str, "Details of Global Object list of size %lu elements", objectMap.size());
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
    for (map<XGL_OBJECT, GLOBAL_OBJECT_NODE*>::iterator ii=objectMap.begin(); ii!=objectMap.end(); ++ii) {
        pTrav = (*ii).second;
        sprintf(str, "    GlobObjNode %p has object %p, pMemNode %p", pTrav, pTrav->object, pTrav->pMemNode);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pTrav->object, 0, MEMTRACK_NONE, "MEM", str);
    }
}

// For given Object, get 'mem' obj that it's bound to or NULL if no binding
static XGL_GPU_MEMORY getMemBindingFromObject(const XGL_OBJECT object)
{
    XGL_GPU_MEMORY mem = NULL;
    GLOBAL_OBJECT_NODE* pObjNode = getGlobalObjectNode(object);
    if (pObjNode) {
        if (pObjNode->pMemNode) {
            mem = pObjNode->pMemNode->mem;
        }
        else {
            char str[1024];
            sprintf(str, "Trying to get mem binding for object %p but object has no mem binding", (void*)object);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_MISSING_MEM_BINDINGS, "MEM", str);
            printObjList();
        }
    }
    else {
        char str[1024];
        sprintf(str, "Trying to get mem binding for object %p but no such object in global list", (void*)object);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_INVALID_OBJECT, "MEM", str);
        printObjList();
    }
    return mem;
}

// Print details of global Mem Obj list
static void printMemList()
{
    GLOBAL_MEM_OBJ_NODE* pTrav = NULL;
    // Just printing each msg individually for now, may want to package these into single large print
    char str[1024];
    sprintf(str, "MEM INFO : Details of Global Memory Object list of size %lu elements", memObjMap.size());
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);

    for (map<XGL_GPU_MEMORY, GLOBAL_MEM_OBJ_NODE*>::iterator ii=memObjMap.begin(); ii!=memObjMap.end(); ++ii) {
        pTrav = (*ii).second;

        sprintf(str, "    ===MemObj Node at %p===", (void*)pTrav);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        sprintf(str, "    Mem object: %p", (void*)pTrav->mem);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        sprintf(str, "    Ref Count: %u", pTrav->refCount);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        if (0 != pTrav->allocInfo.allocationSize) {
            string pAllocInfoMsg = xgl_print_xgl_memory_alloc_info(&pTrav->allocInfo, "{MEM}INFO :       ");
            sprintf(str, "    Mem Alloc info:\n%s", pAllocInfoMsg.c_str());
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        } else {
            sprintf(str, "    Mem Alloc info is NULL (alloc done by xglWsiX11CreatePresentableImage())");
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        }

        sprintf(str, "    XGL OBJECT Binding list of size %lu elements:", pTrav->pObjBindings.size());
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        for (list<XGL_OBJECT>::iterator it = pTrav->pObjBindings.begin(); it != pTrav->pObjBindings.end(); ++it) {
            sprintf(str, "       XGL OBJECT %p", (*it));
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        }

        sprintf(str, "    XGL Command Buffer (CB) binding list of size %lu elements", pTrav->pCmdBufferBindings.size());
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        for (list<XGL_CMD_BUFFER>::iterator it = pTrav->pCmdBufferBindings.begin(); it != pTrav->pCmdBufferBindings.end(); ++it) {
            sprintf(str, "      XGL CB %p", (*it));
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        }
    }
}

static void printGlobalCB()
{
    char str[1024] = {0};
    GLOBAL_CB_NODE* pNode = NULL;
    sprintf(str, "Details of Global CB list of size %lu elements", cbMap.size());
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);

    for (map<XGL_CMD_BUFFER, GLOBAL_CB_NODE*>::iterator ii=cbMap.begin(); ii!=cbMap.end(); ++ii) {
        pNode = (*ii).second;

        sprintf(str, "    Global CB Node (%p) has CB %p, fenceId %" PRIx64", and fence %p",
            (void*)pNode, (void*)pNode->cmdBuffer, pNode->fenceId,
            (void*)getFenceFromId(pNode->fenceId));
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);

        for (list<XGL_GPU_MEMORY>::iterator it = pNode->pMemObjList.begin(); it != pNode->pMemObjList.end(); ++it) {
            sprintf(str, "      Mem obj %p", (*it));
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        }
    }
}

static void initMemTracker(void)
{
    const char *strOpt;
    // initialize MemTracker options
    getLayerOptionEnum("MemTrackerReportLevel", (uint32_t *) &g_reportingLevel);
    g_actionIsDefault = getLayerOptionEnum("MemTrackerDebugAction", (uint32_t *) &g_debugAction);

    if (g_debugAction & XGL_DBG_LAYER_ACTION_LOG_MSG)
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
    xglGetProcAddrType fpNextGPA;
    fpNextGPA = pCurObj->pGPA;
    assert(fpNextGPA);

    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (XGL_PHYSICAL_GPU) pCurObj->nextObject);

    xglGetProcAddrType fpGetProcAddr = (xglGetProcAddrType)fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (char *) "xglGetProcAddr");
    nextTable.GetProcAddr = fpGetProcAddr;

    if (!globalLockInitialized)
    {
        // TODO/TBD: Need to delete this mutex sometime.  How???  One
        // suggestion is to call this during xglCreateInstance(), and then we
        // can clean it up during xglDestroyInstance().  However, that requires
        // that the layer have per-instance locks.  We need to come back and
        // address this soon.
        loader_platform_thread_create_mutex(&globalLock);
        globalLockInitialized = 1;
    }
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo, XGL_DEVICE* pDevice)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initMemTracker);
    XGL_RESULT result = nextTable.CreateDevice((XGL_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    // Save off device in case we need it to create Fences
    globalDevice = *pDevice;
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyDevice(XGL_DEVICE device)
{
    char str[1024];
    sprintf(str, "Printing List details prior to xglDestroyDevice()");
    loader_platform_thread_lock_mutex(&globalLock);
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, device, 0, MEMTRACK_NONE, "MEM", str);
    printMemList();
    printGlobalCB();
    printObjList();
    if (XGL_FALSE == deleteGlobalCBList()) {
        sprintf(str, "Issue deleting global CB list in xglDestroyDevice()");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, device, 0, MEMTRACK_INTERNAL_ERROR, "MEM", str);
    }
    // Report any memory leaks
    GLOBAL_MEM_OBJ_NODE* pTrav = NULL;
    for (map<XGL_GPU_MEMORY, GLOBAL_MEM_OBJ_NODE*>::iterator ii=memObjMap.begin(); ii!=memObjMap.end(); ++ii) {
        pTrav = (*ii).second; 

        if (pTrav->allocInfo.allocationSize != 0) {
            sprintf(str, "Mem Object %p has not been freed. You should clean up this memory by calling xglFreeMemory(%p) prior to xglDestroyDevice().",
                pTrav->mem, pTrav->mem);
            layerCbMsg(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, pTrav->mem, 0, MEMTRACK_MEMORY_LEAK, "MEM", str);
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    XGL_RESULT result = nextTable.DestroyDevice(device);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(XGL_PHYSICAL_GPU gpu, size_t maxLayerCount,
    size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved)
{
        if (gpu != NULL)
    {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        pCurObj = gpuw;
        loader_platform_thread_once(&g_initOnce, initMemTracker);
        XGL_RESULT result = nextTable.EnumerateLayers((XGL_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount,
            maxStringSize, pOutLayerCount, pOutLayers, pReserved);
        return result;
    } else
    {
        if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)
            return XGL_ERROR_INVALID_POINTER;
        // This layer compatible with all GPUs
        *pOutLayerCount = 1;
        strncpy((char *) pOutLayers[0], "MemTracker", maxStringSize);
        return XGL_SUCCESS;
    }
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSubmit(XGL_QUEUE queue, uint32_t cmdBufferCount, const XGL_CMD_BUFFER* pCmdBuffers,
    uint32_t memRefCount, const XGL_MEMORY_REF* pMemRefs, XGL_FENCE fence)
{
    loader_platform_thread_lock_mutex(&globalLock);
    // TODO : Need to track fence and clear mem references when fence clears
    GLOBAL_CB_NODE* pCBNode = NULL;
    uint64_t        fenceId = addFenceNode(fence);
    char            str[1024];
    sprintf(str, "In xglQueueSubmit(), checking %u cmdBuffers with %u memRefs", cmdBufferCount, memRefCount);
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, queue, 0, MEMTRACK_NONE, "MEM", str);
    printMemList();
    printGlobalCB();
    for (uint32_t i = 0; i < cmdBufferCount; i++) {
        pCBNode = getGlobalCBNode(pCmdBuffers[i]);
        pCBNode->fenceId = fenceId;
        sprintf(str, "Verifying mem refs for CB %p", pCmdBuffers[i]);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pCmdBuffers[i], 0, MEMTRACK_NONE, "MEM", str);
        if (XGL_FALSE == validateCBMemRef(pCmdBuffers[i], memRefCount, pMemRefs)) {
            sprintf(str, "Unable to verify memory references for CB %p", (void*)pCmdBuffers[i]);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pCmdBuffers[i], 0, MEMTRACK_CB_MISSING_MEM_REF, "MEM", str);
        }
    }
    printGlobalCB();
    loader_platform_thread_unlock_mutex(&globalLock);
    XGL_RESULT result = nextTable.QueueSubmit(queue, cmdBufferCount, pCmdBuffers, memRefCount, pMemRefs, getFenceFromId(fenceId));
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSetGlobalMemReferences(XGL_QUEUE queue, uint32_t memRefCount, const XGL_MEMORY_REF* pMemRefs)
{
    // TODO : Use global mem references as part of list checked on QueueSubmit above
    XGL_RESULT result = nextTable.QueueSetGlobalMemReferences(queue, memRefCount, pMemRefs);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglAllocMemory(XGL_DEVICE device, const XGL_MEMORY_ALLOC_INFO* pAllocInfo, XGL_GPU_MEMORY* pMem)
{
    XGL_RESULT result = nextTable.AllocMemory(device, pAllocInfo, pMem);
    // TODO : Track allocations and overall size here
    loader_platform_thread_lock_mutex(&globalLock);
    insertGlobalMemObj(*pMem, pAllocInfo);
    printMemList();
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglFreeMemory(XGL_GPU_MEMORY mem)
{
    /* From spec : A memory object is freed by calling xglFreeMemory() when it is no longer needed. Before
     * freeing a memory object, an application must ensure the memory object is unbound from
     * all API objects referencing it and that it is not referenced by any queued command buffers
     */
    loader_platform_thread_lock_mutex(&globalLock);
    if (XGL_FALSE == freeMemNode(mem)) {
        char str[1024];
        sprintf(str, "Issue while freeing mem obj %p", (void*)mem);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_FREE_MEM_ERROR, "MEM", str);
    }
    printMemList();
    printObjList();
    printGlobalCB();
    loader_platform_thread_unlock_mutex(&globalLock);
    XGL_RESULT result = nextTable.FreeMemory(mem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetMemoryPriority(XGL_GPU_MEMORY mem, XGL_MEMORY_PRIORITY priority)
{
    // TODO : Update tracking for this alloc
    //  Make sure memory is not pinned, which can't have priority set
    XGL_RESULT result = nextTable.SetMemoryPriority(mem, priority);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglMapMemory(XGL_GPU_MEMORY mem, XGL_FLAGS flags, void** ppData)
{
    // TODO : Track when memory is mapped
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_MEM_OBJ_NODE *pMemObj = getGlobalMemNode(mem);
    if ((pMemObj->allocInfo.memProps & XGL_MEMORY_PROPERTY_CPU_VISIBLE_BIT) == 0) {
        char str[1024];
        sprintf(str, "Mapping Memory (%p) without XGL_MEMORY_PROPERTY_CPU_VISIBLE_BIT set", (void*)mem);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_INVALID_STATE, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    XGL_RESULT result = nextTable.MapMemory(mem, flags, ppData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglUnmapMemory(XGL_GPU_MEMORY mem)
{
    // TODO : Track as memory gets unmapped, do we want to check what changed following map?
    //   Make sure that memory was ever mapped to begin with
    XGL_RESULT result = nextTable.UnmapMemory(mem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglPinSystemMemory(XGL_DEVICE device, const void* pSysMem, size_t memSize, XGL_GPU_MEMORY* pMem)
{
    // TODO : Track this
    //  Verify that memory is actually pinnable
    XGL_RESULT result = nextTable.PinSystemMemory(device, pSysMem, memSize, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenSharedMemory(XGL_DEVICE device, const XGL_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem)
{
    // TODO : Track this
    XGL_RESULT result = nextTable.OpenSharedMemory(device, pOpenInfo, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenPeerMemory(XGL_DEVICE device, const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem)
{
    // TODO : Track this
    XGL_RESULT result = nextTable.OpenPeerMemory(device, pOpenInfo, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenPeerImage(XGL_DEVICE device, const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo, XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem)
{
    // TODO : Track this
    XGL_RESULT result = nextTable.OpenPeerImage(device, pOpenInfo, pImage, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyObject(XGL_OBJECT object)
{
    loader_platform_thread_lock_mutex(&globalLock);

    // First check if this is a CmdBuffer
    if (NULL != getGlobalCBNode((XGL_CMD_BUFFER)object)) {
        deleteGlobalCBNode((XGL_CMD_BUFFER)object);
    }

    if (objectMap.find(object) != objectMap.end()) {
        GLOBAL_OBJECT_NODE* pDelNode = objectMap[object];
        if (pDelNode->pMemNode) {
            // Wsi allocated Memory is tied to image object so clear the binding and free that memory automatically
            if (0 == pDelNode->pMemNode->allocInfo.allocationSize) { // Wsi allocated memory has NULL allocInfo w/ 0 size
                XGL_GPU_MEMORY memToFree = pDelNode->pMemNode->mem;
                clearObjectBinding(object);
                freeMemNode(memToFree);
            } else {
                char str[1024];
                sprintf(str, "Destroying obj %p that is still bound to memory object %p\nYou should first clear binding by calling xglBindObjectMemory(%p, 0, XGL_NULL_HANDLE)", object, (void*)pDelNode->pMemNode->mem, object);
                layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_DESTROY_OBJECT_ERROR, "MEM", str);
                // From the spec : If an object has previous memory binding, it is required to unbind memory from an API object before it is destroyed.
                clearObjectBinding(object);
            }
        }
        delete pDelNode;
        objectMap.erase(object);
    }

    loader_platform_thread_unlock_mutex(&globalLock);
    XGL_RESULT result = nextTable.DestroyObject(object);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetObjectInfo(XGL_BASE_OBJECT object, XGL_OBJECT_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    // TODO : What to track here?
    //   Could potentially save returned mem requirements and validate values passed into BindObjectMemory for this object
    // From spec : The only objects that are guaranteed to have no external memory requirements are devices, queues, command buffers, shaders and memory objects.
    XGL_RESULT result = nextTable.GetObjectInfo(object, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindObjectMemory(XGL_OBJECT object, uint32_t allocationIdx, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset)
{
    XGL_RESULT result = nextTable.BindObjectMemory(object, allocationIdx, mem, offset);
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    if (XGL_FALSE == updateObjectBinding(object, mem)) {
        char str[1024];
        sprintf(str, "Unable to set object %p binding to mem obj %p", (void*)object, (void*)mem);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    printObjList();
    printMemList();
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateFence(XGL_DEVICE device, const XGL_FENCE_CREATE_INFO* pCreateInfo, XGL_FENCE* pFence)
{
    XGL_RESULT result = nextTable.CreateFence(device, pCreateInfo, pFence);
    if (XGL_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pFence, pCreateInfo->sType, pCreateInfo, sizeof(XGL_FENCE_CREATE_INFO), "fence");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFenceStatus(XGL_FENCE fence)
{
    XGL_RESULT result = nextTable.GetFenceStatus(fence);
    if (XGL_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        updateFenceTracking(fence);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWaitForFences(XGL_DEVICE device, uint32_t fenceCount, const XGL_FENCE* pFences, bool32_t waitAll, uint64_t timeout)
{
    XGL_RESULT result = nextTable.WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    loader_platform_thread_lock_mutex(&globalLock);
    if (XGL_SUCCESS == result) {
        if (waitAll) { // Clear all the fences
            for(uint32_t i = 0; i < fenceCount; i++) {
                updateFenceTracking(pFences[i]);
            }
        }
        else { // Clear only completed fences
            for(uint32_t i = 0; i < fenceCount; i++) {
                if (XGL_SUCCESS == nextTable.GetFenceStatus(pFences[i])) {
                    updateFenceTracking(pFences[i]);
                }
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueWaitIdle(XGL_QUEUE queue)
{
    XGL_RESULT result = nextTable.QueueWaitIdle(queue);
    if (XGL_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        retireAllFences();
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDeviceWaitIdle(XGL_DEVICE device)
{
    XGL_RESULT result = nextTable.DeviceWaitIdle(device);
    if (XGL_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        retireAllFences();
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateEvent(XGL_DEVICE device, const XGL_EVENT_CREATE_INFO* pCreateInfo, XGL_EVENT* pEvent)
{
    XGL_RESULT result = nextTable.CreateEvent(device, pCreateInfo, pEvent);
    if (XGL_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pEvent, pCreateInfo->sType, pCreateInfo, sizeof(XGL_EVENT_CREATE_INFO), "event");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateQueryPool(XGL_DEVICE device, const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo, XGL_QUERY_POOL* pQueryPool)
{
    XGL_RESULT result = nextTable.CreateQueryPool(device, pCreateInfo, pQueryPool);
    if (XGL_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pQueryPool, pCreateInfo->sType, pCreateInfo, sizeof(XGL_QUERY_POOL_CREATE_INFO), "query_pool");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateBuffer(XGL_DEVICE device, const XGL_BUFFER_CREATE_INFO* pCreateInfo, XGL_BUFFER* pBuffer)
{
    XGL_RESULT result = nextTable.CreateBuffer(device, pCreateInfo, pBuffer);
    if (XGL_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pBuffer, pCreateInfo->sType, pCreateInfo, sizeof(XGL_BUFFER_CREATE_INFO), "buffer");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateBufferView(XGL_DEVICE device, const XGL_BUFFER_VIEW_CREATE_INFO* pCreateInfo, XGL_BUFFER_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateBufferView(device, pCreateInfo, pView);
    if (result == XGL_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pView, pCreateInfo->sType, pCreateInfo, sizeof(XGL_BUFFER_VIEW_CREATE_INFO), "buffer_view");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImage(XGL_DEVICE device, const XGL_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage)
{
    XGL_RESULT result = nextTable.CreateImage(device, pCreateInfo, pImage);
    if (XGL_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pImage, pCreateInfo->sType, pCreateInfo, sizeof(XGL_IMAGE_CREATE_INFO), "image");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImageView(XGL_DEVICE device, const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo, XGL_IMAGE_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateImageView(device, pCreateInfo, pView);
    if (result == XGL_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pView, pCreateInfo->sType, pCreateInfo, sizeof(XGL_IMAGE_VIEW_CREATE_INFO), "image_view");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateColorAttachmentView(XGL_DEVICE device, const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
    XGL_COLOR_ATTACHMENT_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateColorAttachmentView(device, pCreateInfo, pView);
    if (result == XGL_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pView, pCreateInfo->sType, pCreateInfo, sizeof(XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO), "color_attachment_view");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDepthStencilView(XGL_DEVICE device, const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo, XGL_DEPTH_STENCIL_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateDepthStencilView(device, pCreateInfo, pView);
    if (result == XGL_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pView, pCreateInfo->sType, pCreateInfo, sizeof(XGL_DEPTH_STENCIL_VIEW_CREATE_INFO), "ds_view");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateShader(XGL_DEVICE device, const XGL_SHADER_CREATE_INFO* pCreateInfo, XGL_SHADER* pShader)
{
    XGL_RESULT result = nextTable.CreateShader(device, pCreateInfo, pShader);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateGraphicsPipeline(XGL_DEVICE device, const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline)
{
    XGL_RESULT result = nextTable.CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    if (result == XGL_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pPipeline, pCreateInfo->sType, pCreateInfo, sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO), "graphics_pipeline");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateComputePipeline(XGL_DEVICE device, const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline)
{
    XGL_RESULT result = nextTable.CreateComputePipeline(device, pCreateInfo, pPipeline);
    if (result == XGL_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pPipeline, pCreateInfo->sType, pCreateInfo, sizeof(XGL_COMPUTE_PIPELINE_CREATE_INFO), "compute_pipeline");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateSampler(XGL_DEVICE device, const XGL_SAMPLER_CREATE_INFO* pCreateInfo, XGL_SAMPLER* pSampler)
{
    XGL_RESULT result = nextTable.CreateSampler(device, pCreateInfo, pSampler);
    if (result == XGL_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pSampler, pCreateInfo->sType, pCreateInfo, sizeof(XGL_SAMPLER_CREATE_INFO), "sampler");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicViewportState(XGL_DEVICE device, const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pCreateInfo,
    XGL_DYNAMIC_VP_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateDynamicViewportState(device, pCreateInfo, pState);
    if (result == XGL_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pState, pCreateInfo->sType, pCreateInfo, sizeof(XGL_DYNAMIC_VP_STATE_CREATE_INFO), "viewport_state");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicRasterState(XGL_DEVICE device, const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pCreateInfo,
    XGL_DYNAMIC_RS_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateDynamicRasterState(device, pCreateInfo, pState);
    if (result == XGL_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pState, pCreateInfo->sType, pCreateInfo, sizeof(XGL_DYNAMIC_RS_STATE_CREATE_INFO), "raster_state");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicColorBlendState(XGL_DEVICE device, const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pCreateInfo,
    XGL_DYNAMIC_CB_STATE_OBJECT*  pState)
{
    XGL_RESULT result = nextTable.CreateDynamicColorBlendState(device, pCreateInfo, pState);
    if (result == XGL_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pState, pCreateInfo->sType, pCreateInfo, sizeof(XGL_DYNAMIC_CB_STATE_CREATE_INFO), "cb_state");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicDepthStencilState(XGL_DEVICE device, const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pCreateInfo,
    XGL_DYNAMIC_DS_STATE_OBJECT*    pState)
{
    XGL_RESULT result = nextTable.CreateDynamicDepthStencilState(device, pCreateInfo, pState);
    if (result == XGL_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        insertGlobalObjectNode(*pState, pCreateInfo->sType, pCreateInfo, sizeof(XGL_DYNAMIC_DS_STATE_CREATE_INFO), "ds_state");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateCommandBuffer(XGL_DEVICE device, const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo, XGL_CMD_BUFFER* pCmdBuffer)
{
    XGL_RESULT result = nextTable.CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    // At time of cmd buffer creation, create global cmd buffer node for the returned cmd buffer
    loader_platform_thread_lock_mutex(&globalLock);
    if (*pCmdBuffer)
        insertGlobalCB(*pCmdBuffer);
    printGlobalCB();
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBeginCommandBuffer(XGL_CMD_BUFFER cmdBuffer, const XGL_CMD_BUFFER_BEGIN_INFO* pBeginInfo)
{
    // This implicitly resets the Cmd Buffer so make sure any fence is done and then clear memory references
    GLOBAL_CB_NODE* pCBTrav = getGlobalCBNode(cmdBuffer);
    if (pCBTrav && (!fenceRetired(pCBTrav->fenceId))) {
        bool32_t cbDone = checkCBCompleted(cmdBuffer);
        if (XGL_FALSE == cbDone) {
            char str[1024];
            sprintf(str, "Calling xglBeginCommandBuffer() on active CB %p before it has completed. You must check CB flag before this call.", cmdBuffer);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM", str);
        }
    }
    XGL_RESULT result = nextTable.BeginCommandBuffer(cmdBuffer, pBeginInfo);
    loader_platform_thread_lock_mutex(&globalLock);
    freeCBBindings(cmdBuffer);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEndCommandBuffer(XGL_CMD_BUFFER cmdBuffer)
{
    // TODO : Anything to do here?
    XGL_RESULT result = nextTable.EndCommandBuffer(cmdBuffer);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglResetCommandBuffer(XGL_CMD_BUFFER cmdBuffer)
{
    // Verify that CB is complete (not in-flight)
    GLOBAL_CB_NODE* pCBTrav = getGlobalCBNode(cmdBuffer);
    if (pCBTrav && (!fenceRetired(pCBTrav->fenceId))) {
        bool32_t cbDone = checkCBCompleted(cmdBuffer);
        if (XGL_FALSE == cbDone) {
            char str[1024];
            sprintf(str, "Resetting CB %p before it has completed. You must check CB flag before calling xglResetCommandBuffer().", cmdBuffer);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM", str);
        }
    }
    // Clear memory references as this point.
    loader_platform_thread_lock_mutex(&globalLock);
    freeCBBindings(cmdBuffer);
    loader_platform_thread_unlock_mutex(&globalLock);
    XGL_RESULT result = nextTable.ResetCommandBuffer(cmdBuffer);
    return result;
}
// TODO : For any xglCmdBind* calls that include an object which has mem bound to it,
//    need to account for that mem now having binding to given cmdBuffer
XGL_LAYER_EXPORT void XGLAPI xglCmdBindPipeline(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE pipeline)
{
#if 0
    // TODO : If memory bound to pipeline, then need to tie that mem to cmdBuffer
    if (getPipeline(pipeline)) {
        GLOBAL_CB_NODE *pCBTrav = getGlobalCBNode(cmdBuffer);
        if (pCBTrav) {
            pCBTrav->pipelines[pipelineBindPoint] = pipeline;
        } else {
            char str[1024];
            sprintf(str, "Attempt to bind Pipeline %p to non-existant command buffer %p!", (void*)pipeline, cmdBuffer);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_INVALID_CB, (char *) "DS", (char *) str);
        }
    }
    else {
        char str[1024];
        sprintf(str, "Attempt to bind Pipeline %p that doesn't exist!", (void*)pipeline);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pipeline, 0, MEMTRACK_INVALID_OBJECT, (char *) "DS", (char *) str);
    }
#endif
    nextTable.CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindDynamicStateObject(XGL_CMD_BUFFER cmdBuffer, XGL_STATE_BIND_POINT stateBindPoint, XGL_DYNAMIC_STATE_OBJECT state)
{
    GLOBAL_OBJECT_NODE *pNode;
    loader_platform_thread_lock_mutex(&globalLock);
    GLOBAL_CB_NODE *pCmdBuf = getGlobalCBNode(cmdBuffer);
    if (!pCmdBuf) {
        char str[1024];
        sprintf(str, "Unable to find command buffer object %p, was it ever created?", (void*)cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_INVALID_CB, "DD", str);
    }
    pNode = getGlobalObjectNode(state);
    if (!pNode) {
        char str[1024];
        sprintf(str, "Unable to find dynamic state object %p, was it ever created?", (void*)state);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, state, 0, MEMTRACK_INVALID_OBJECT, "DD", str);
    }
    pCmdBuf->pDynamicState[stateBindPoint] = pNode;
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdBindDynamicStateObject(cmdBuffer, stateBindPoint, state);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindDescriptorSet(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint,
    XGL_DESCRIPTOR_SET descriptorSet, const uint32_t* pUserData)
{
    // TODO : Somewhere need to verify that all textures referenced by shaders in DS are in some type of *SHADER_READ* state
    nextTable.CmdBindDescriptorSet(cmdBuffer, pipelineBindPoint, descriptorSet, pUserData);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindVertexBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t binding)
{
    nextTable.CmdBindVertexBuffer(cmdBuffer, buffer, offset, binding);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindIndexBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, XGL_INDEX_TYPE indexType)
{
    nextTable.CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDrawIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t count, uint32_t stride)
{
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(buffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdDrawIndirect() call unable to update binding of buffer %p to cmdBuffer %p", buffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDrawIndexedIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t count, uint32_t stride)
{
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(buffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdDrawIndexedIndirect() call unable to update binding of buffer %p to cmdBuffer %p", buffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDispatchIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset)
{
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(buffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdDispatchIndirect() call unable to update binding of buffer %p to cmdBuffer %p", buffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER srcBuffer, XGL_BUFFER destBuffer,
    uint32_t regionCount, const XGL_BUFFER_COPY* pRegions)
{
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(srcBuffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdCopyBuffer() call unable to update binding of srcBuffer %p to cmdBuffer %p", srcBuffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    mem = getMemBindingFromObject(destBuffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdCopyBuffer() call unable to update binding of destBuffer %p to cmdBuffer %p", destBuffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage,
    uint32_t regionCount, const XGL_IMAGE_COPY* pRegions)
{
    // TODO : Each image will have mem mapping so track them
    nextTable.CmdCopyImage(cmdBuffer, srcImage, destImage, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyBufferToImage(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER srcBuffer, XGL_IMAGE destImage,
    uint32_t regionCount, const XGL_BUFFER_IMAGE_COPY* pRegions)
{
    // TODO : Track this
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(destImage);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdCopyMemoryToImage() call unable to update binding of destImage buffer %p to cmdBuffer %p", destImage, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }

    mem = getMemBindingFromObject(srcBuffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdCopyMemoryToImage() call unable to update binding of srcBuffer %p to cmdBuffer %p", srcBuffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyImageToBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_BUFFER destBuffer,
    uint32_t regionCount, const XGL_BUFFER_IMAGE_COPY* pRegions)
{
    // TODO : Track this
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(srcImage);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdCopyImageToMemory() call unable to update binding of srcImage buffer %p to cmdBuffer %p", srcImage, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    mem = getMemBindingFromObject(destBuffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdCopyImageToMemory() call unable to update binding of destBuffer %p to cmdBuffer %p", destBuffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdCopyImageToBuffer(cmdBuffer, srcImage, destBuffer, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCloneImageData(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE_LAYOUT srcImageLayout,
    XGL_IMAGE destImage, XGL_IMAGE_LAYOUT destImageLayout)
{
    // TODO : Each image will have mem mapping so track them
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(srcImage);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdCloneImageData() call unable to update binding of srcImage buffer %p to cmdBuffer %p", srcImage, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    mem = getMemBindingFromObject(destImage);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdCloneImageData() call unable to update binding of destImage buffer %p to cmdBuffer %p", destImage, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdCloneImageData(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdUpdateBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE dataSize, const uint32_t* pData)
{
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(destBuffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdUpdateMemory() call unable to update binding of destBuffer %p to cmdBuffer %p", destBuffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdFillBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE fillSize, uint32_t data)
{
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(destBuffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdFillMemory() call unable to update binding of destBuffer %p to cmdBuffer %p", destBuffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdClearColorImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const float color[4],
    uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    // TODO : Verify memory is in XGL_IMAGE_STATE_CLEAR state
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(image);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdClearColorImage() call unable to update binding of image buffer %p to cmdBuffer %p", image, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdClearColorImage(cmdBuffer, image, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdClearColorImageRaw(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const uint32_t color[4],
    uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    // TODO : Verify memory is in XGL_IMAGE_STATE_CLEAR state
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(image);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdClearColorImageRaw() call unable to update binding of image buffer %p to cmdBuffer %p", image, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdClearColorImageRaw(cmdBuffer, image, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdClearDepthStencil(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, float depth,
   uint32_t stencil, uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    // TODO : Verify memory is in XGL_IMAGE_STATE_CLEAR state
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(image);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdClearDepthStencil() call unable to update binding of image buffer %p to cmdBuffer %p", image, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdClearDepthStencil(cmdBuffer, image, depth, stencil, rangeCount, pRanges);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdResolveImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage,
    uint32_t rectCount, const XGL_IMAGE_RESOLVE* pRects)
{
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(srcImage);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdResolveImage() call unable to update binding of srcImage buffer %p to cmdBuffer %p", srcImage, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    mem = getMemBindingFromObject(destImage);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdResolveImage() call unable to update binding of destImage buffer %p to cmdBuffer %p", destImage, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdResolveImage(cmdBuffer, srcImage, destImage, rectCount, pRects);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBeginQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t slot, XGL_FLAGS flags)
{
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(queryPool);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdBeginQuery() call unable to update binding of queryPool buffer %p to cmdBuffer %p", queryPool, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdEndQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t slot)
{
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(queryPool);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdEndQuery() call unable to update binding of queryPool buffer %p to cmdBuffer %p", queryPool, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdEndQuery(cmdBuffer, queryPool, slot);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdResetQueryPool(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount)
{
    loader_platform_thread_lock_mutex(&globalLock);
    XGL_GPU_MEMORY mem = getMemBindingFromObject(queryPool);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdResetQueryPool() call unable to update binding of queryPool buffer %p to cmdBuffer %p", queryPool, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    nextTable.CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData)
{
    // This layer intercepts callbacks
    XGL_LAYER_DBG_FUNCTION_NODE *pNewDbgFuncNode = (XGL_LAYER_DBG_FUNCTION_NODE*)malloc(sizeof(XGL_LAYER_DBG_FUNCTION_NODE));
    if (!pNewDbgFuncNode)
        return XGL_ERROR_OUT_OF_MEMORY;
    pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;
    pNewDbgFuncNode->pUserData = pUserData;
    pNewDbgFuncNode->pNext = g_pDbgFunctionHead;
    g_pDbgFunctionHead = pNewDbgFuncNode;
    // force callbacks if DebugAction hasn't been set already other than initial value
    if (g_actionIsDefault) {
        g_debugAction = XGL_DBG_LAYER_ACTION_CALLBACK;
    }
    XGL_RESULT result = nextTable.DbgRegisterMsgCallback(pfnMsgCallback, pUserData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgUnregisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
{
    XGL_LAYER_DBG_FUNCTION_NODE *pTrav = g_pDbgFunctionHead;
    XGL_LAYER_DBG_FUNCTION_NODE *pPrev = pTrav;
    while (pTrav) {
        if (pTrav->pfnMsgCallback == pfnMsgCallback) {
            pPrev->pNext = pTrav->pNext;
            if (g_pDbgFunctionHead == pTrav)
                g_pDbgFunctionHead = pTrav->pNext;
            free(pTrav);
            break;
        }
        pPrev = pTrav;
        pTrav = pTrav->pNext;
    }
    if (g_pDbgFunctionHead == NULL)
    {
        if (g_actionIsDefault) {
            g_debugAction = XGL_DBG_LAYER_ACTION_LOG_MSG;
        } else {
            g_debugAction = (XGL_LAYER_DBG_ACTION)(g_debugAction & ~((uint32_t)XGL_DBG_LAYER_ACTION_CALLBACK));
        }
    }
    XGL_RESULT result = nextTable.DbgUnregisterMsgCallback(pfnMsgCallback);
    return result;
}

#if !defined(WIN32)
XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11CreatePresentableImage(XGL_DEVICE device, const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
    XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem)
{
    XGL_RESULT result = nextTable.WsiX11CreatePresentableImage(device, pCreateInfo, pImage, pMem);
    loader_platform_thread_lock_mutex(&globalLock);
    if (XGL_SUCCESS == result) {
        // Add image object, then insert the new Mem Object and then bind it to created image
        insertGlobalObjectNode(*pImage, _XGL_STRUCTURE_TYPE_MAX_ENUM, pCreateInfo, sizeof(XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO), "wsi_x11_image");
        insertGlobalMemObj(*pMem, NULL);
        if (XGL_FALSE == updateObjectBinding(*pImage, *pMem)) {
            char str[1024];
            sprintf(str, "In xglWsiX11CreatePresentableImage(), unable to set image %p binding to mem obj %p", (void*)*pImage, (void*)*pMem);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, *pImage, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
        }
    }
    printObjList();
    printMemList();
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11QueuePresent(XGL_QUEUE queue, const XGL_WSI_X11_PRESENT_INFO*  pPresentInfo, XGL_FENCE fence)
{
    loader_platform_thread_lock_mutex(&globalLock);
    addFenceNode(fence);
    char            str[1024];
    sprintf(str, "In xglWsiX11QueuePresent(), checking queue %p for fence %p", queue, fence);
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, queue, 0, MEMTRACK_NONE, "MEM", str);
    loader_platform_thread_unlock_mutex(&globalLock);
    XGL_RESULT result = nextTable.WsiX11QueuePresent(queue, pPresentInfo, fence);
    return result;
}
#endif // WIN32

XGL_LAYER_EXPORT void* XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const char* funcName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;

    if (gpu == NULL)
        return NULL;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initMemTracker);

    if (!strcmp(funcName, "xglGetProcAddr"))
        return (void *) xglGetProcAddr;
    if (!strcmp(funcName, "xglCreateDevice"))
        return (void*) xglCreateDevice;
    if (!strcmp(funcName, "xglDestroyDevice"))
        return (void*) xglDestroyDevice;
    if (!strcmp(funcName, "xglEnumerateLayers"))
        return (void*) xglEnumerateLayers;
    if (!strcmp(funcName, "xglQueueSubmit"))
        return (void*) xglQueueSubmit;
    if (!strcmp(funcName, "xglQueueSetGlobalMemReferences"))
        return (void*) xglQueueSetGlobalMemReferences;
    if (!strcmp(funcName, "xglAllocMemory"))
        return (void*) xglAllocMemory;
    if (!strcmp(funcName, "xglFreeMemory"))
        return (void*) xglFreeMemory;
    if (!strcmp(funcName, "xglSetMemoryPriority"))
        return (void*) xglSetMemoryPriority;
    if (!strcmp(funcName, "xglMapMemory"))
        return (void*) xglMapMemory;
    if (!strcmp(funcName, "xglUnmapMemory"))
        return (void*) xglUnmapMemory;
    if (!strcmp(funcName, "xglPinSystemMemory"))
        return (void*) xglPinSystemMemory;
    if (!strcmp(funcName, "xglOpenSharedMemory"))
        return (void*) xglOpenSharedMemory;
    if (!strcmp(funcName, "xglOpenPeerMemory"))
        return (void*) xglOpenPeerMemory;
    if (!strcmp(funcName, "xglOpenPeerImage"))
        return (void*) xglOpenPeerImage;
    if (!strcmp(funcName, "xglDestroyObject"))
        return (void*) xglDestroyObject;
    if (!strcmp(funcName, "xglGetObjectInfo"))
        return (void*) xglGetObjectInfo;
    if (!strcmp(funcName, "xglBindObjectMemory"))
        return (void*) xglBindObjectMemory;
    if (!strcmp(funcName, "xglCreateFence"))
        return (void*) xglCreateFence;
    if (!strcmp(funcName, "xglGetFenceStatus"))
        return (void*) xglGetFenceStatus;
    if (!strcmp(funcName, "xglWaitForFences"))
        return (void*) xglWaitForFences;
    if (!strcmp(funcName, "xglQueueWaitIdle"))
        return (void*) xglQueueWaitIdle;
    if (!strcmp(funcName, "xglDeviceWaitIdle"))
        return (void*) xglDeviceWaitIdle;
    if (!strcmp(funcName, "xglCreateEvent"))
        return (void*) xglCreateEvent;
    if (!strcmp(funcName, "xglCreateQueryPool"))
        return (void*) xglCreateQueryPool;
    if (!strcmp(funcName, "xglCreateBuffer"))
        return (void*) xglCreateBuffer;
    if (!strcmp(funcName, "xglCreateBufferView"))
        return (void*) xglCreateBufferView;
    if (!strcmp(funcName, "xglCreateImage"))
        return (void*) xglCreateImage;
    if (!strcmp(funcName, "xglCreateImageView"))
        return (void*) xglCreateImageView;
    if (!strcmp(funcName, "xglCreateColorAttachmentView"))
        return (void*) xglCreateColorAttachmentView;
    if (!strcmp(funcName, "xglCreateDepthStencilView"))
        return (void*) xglCreateDepthStencilView;
    if (!strcmp(funcName, "xglCreateShader"))
        return (void*) xglCreateShader;
    if (!strcmp(funcName, "xglCreateGraphicsPipeline"))
        return (void*) xglCreateGraphicsPipeline;
    if (!strcmp(funcName, "xglCreateComputePipeline"))
        return (void*) xglCreateComputePipeline;
    if (!strcmp(funcName, "xglCreateSampler"))
        return (void*) xglCreateSampler;
    if (!strcmp(funcName, "xglCreateDynamicViewportState"))
        return (void*) xglCreateDynamicViewportState;
    if (!strcmp(funcName, "xglCreateDynamicRasterState"))
        return (void*) xglCreateDynamicRasterState;
    if (!strcmp(funcName, "xglCreateDynamicColorBlendState"))
        return (void*) xglCreateDynamicColorBlendState;
    if (!strcmp(funcName, "xglCreateDynamicDepthStencilState"))
        return (void*) xglCreateDynamicDepthStencilState;
    if (!strcmp(funcName, "xglCreateCommandBuffer"))
        return (void*) xglCreateCommandBuffer;
    if (!strcmp(funcName, "xglBeginCommandBuffer"))
        return (void*) xglBeginCommandBuffer;
    if (!strcmp(funcName, "xglEndCommandBuffer"))
        return (void*) xglEndCommandBuffer;
    if (!strcmp(funcName, "xglResetCommandBuffer"))
        return (void*) xglResetCommandBuffer;
    if (!strcmp(funcName, "xglCmdBindPipeline"))
        return (void*) xglCmdBindPipeline;
    if (!strcmp(funcName, "xglCmdBindDynamicStateObject"))
        return (void*) xglCmdBindDynamicStateObject;
    if (!strcmp(funcName, "xglCmdBindDescriptorSet"))
        return (void*) xglCmdBindDescriptorSet;
    if (!strcmp(funcName, "xglCmdBindVertexBuffer"))
        return (void*) xglCmdBindVertexBuffer;
    if (!strcmp(funcName, "xglCmdBindIndexBuffer"))
        return (void*) xglCmdBindIndexBuffer;
    if (!strcmp(funcName, "xglCmdDrawIndirect"))
        return (void*) xglCmdDrawIndirect;
    if (!strcmp(funcName, "xglCmdDrawIndexedIndirect"))
        return (void*) xglCmdDrawIndexedIndirect;
    if (!strcmp(funcName, "xglCmdDispatchIndirect"))
        return (void*) xglCmdDispatchIndirect;
    if (!strcmp(funcName, "xglCmdCopyBuffer"))
        return (void*) xglCmdCopyBuffer;
    if (!strcmp(funcName, "xglCmdCopyImage"))
        return (void*) xglCmdCopyImage;
    if (!strcmp(funcName, "xglCmdCopyBufferToImage"))
        return (void*) xglCmdCopyBufferToImage;
    if (!strcmp(funcName, "xglCmdCopyImageToBuffer"))
        return (void*) xglCmdCopyImageToBuffer;
    if (!strcmp(funcName, "xglCmdCloneImageData"))
        return (void*) xglCmdCloneImageData;
    if (!strcmp(funcName, "xglCmdUpdateBuffer"))
        return (void*) xglCmdUpdateBuffer;
    if (!strcmp(funcName, "xglCmdFillBuffer"))
        return (void*) xglCmdFillBuffer;
    if (!strcmp(funcName, "xglCmdClearColorImage"))
        return (void*) xglCmdClearColorImage;
    if (!strcmp(funcName, "xglCmdClearColorImageRaw"))
        return (void*) xglCmdClearColorImageRaw;
    if (!strcmp(funcName, "xglCmdClearDepthStencil"))
        return (void*) xglCmdClearDepthStencil;
    if (!strcmp(funcName, "xglCmdResolveImage"))
        return (void*) xglCmdResolveImage;
    if (!strcmp(funcName, "xglCmdBeginQuery"))
        return (void*) xglCmdBeginQuery;
    if (!strcmp(funcName, "xglCmdEndQuery"))
        return (void*) xglCmdEndQuery;
    if (!strcmp(funcName, "xglCmdResetQueryPool"))
        return (void*) xglCmdResetQueryPool;
    if (!strcmp(funcName, "xglDbgRegisterMsgCallback"))
        return (void*) xglDbgRegisterMsgCallback;
    if (!strcmp(funcName, "xglDbgUnregisterMsgCallback"))
        return (void*) xglDbgUnregisterMsgCallback;
#if !defined(WIN32)
    if (!strcmp(funcName, "xglWsiX11CreatePresentableImage"))
        return (void*) xglWsiX11CreatePresentableImage;
    if (!strcmp(funcName, "xglWsiX11QueuePresent"))
        return (void*) xglWsiX11QueuePresent;
#endif
    else {
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((XGL_PHYSICAL_GPU)gpuw->nextObject, funcName);
    }
}
