/*
 * XGL
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "loader_platform.h"
#include "xgl_dispatch_table_helper.h"
#include "xgl_generic_intercept_proc_helper.h"
#include "xgl_struct_string_helper.h"
#include "mem_tracker.h"
#include "layers_config.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"
#include "layers_msg.h"

static XGL_LAYER_DISPATCH_TABLE nextTable;
static XGL_BASE_LAYER_OBJECT *pCurObj;
static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(g_initOnce);


#define MAX_BINDING 0xFFFFFFFF
static uint32_t lastVtxBinding = MAX_BINDING;

static GLOBAL_CB_NODE* pGlobalCBHead = NULL;
static GLOBAL_MEM_OBJ_NODE* pGlobalMemObjHead = NULL;
static GLOBAL_OBJECT_NODE* pGlobalObjectHead = NULL;
static XGL_DEVICE globalDevice = NULL;
static uint64_t numCBNodes = 0;
static uint64_t numMemObjNodes = 0;
static uint64_t numObjectNodes = 0;

// Check list for data and if it's not included insert new node
//    into HEAD of list pointed to by pHEAD & update pHEAD
// Increment 'insert' if new node was inserted
// return XGL_SUCCESS if no errors occur
static XGL_RESULT insertMiniNode(MINI_NODE** pHEAD, const XGL_BASE_OBJECT data, uint32_t* insert)
{
    MINI_NODE* pTrav = *pHEAD;
    while (pTrav && (pTrav->data != data)) {
        pTrav = pTrav->pNext;
    }
    if (!pTrav) { // Add node to front of LL
        pTrav = (MINI_NODE*)malloc(sizeof(MINI_NODE));
        if (!pTrav)
            return XGL_ERROR_OUT_OF_MEMORY;
        memset(pTrav, 0, sizeof(MINI_NODE));
        if (*pHEAD)
            pTrav->pNext = *pHEAD;
        *pHEAD = pTrav;
        *insert += 1;
        //pMemTrav->refCount++;
        //sprintf(str, "MEM INFO : Incremented refCount for mem obj %p to %u", (void*)mem, pMemTrav->refCount);
    }
    if (pTrav->data) { // This is just FYI
        assert(data == pTrav->data);
        char str[1024];
        sprintf(str, "Data %p is already in data LL w/ HEAD at %p", data, *pHEAD);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, data, 0, MEMTRACK_NONE, "MEM", str);
    }
    pTrav->data = data;
    return XGL_SUCCESS;
}

// Add new CB node for this cb at end of global CB LL
static void insertGlobalCB(const XGL_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pTrav = pGlobalCBHead;
    if (!pTrav) {
        pTrav = (GLOBAL_CB_NODE*)malloc(sizeof(GLOBAL_CB_NODE));
        pGlobalCBHead = pTrav;
    }
    else {
        while (NULL != pTrav->pNextGlobalCBNode)
            pTrav = pTrav->pNextGlobalCBNode;
        pTrav->pNextGlobalCBNode = (GLOBAL_CB_NODE*)malloc(sizeof(GLOBAL_CB_NODE));
        pTrav = pTrav->pNextGlobalCBNode;
    }
    if (!pTrav) {
        char str[1024];
        sprintf(str, "Malloc failed to alloc node for Cmd Buffer %p", (void*)cb);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_OUT_OF_MEMORY_ERROR, "MEM", str);
    }
    else {
        numCBNodes++;
        memset(pTrav, 0, sizeof(GLOBAL_CB_NODE));
        pTrav->cmdBuffer = cb;
    }
}

// Return ptr to node in global LL containing cb, or NULL if not found
static GLOBAL_CB_NODE* getGlobalCBNode(const XGL_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pTrav = pGlobalCBHead;
    while (pTrav && (pTrav->cmdBuffer != cb))
        pTrav = pTrav->pNextGlobalCBNode;
    return pTrav;
}
// Set fence for given cb in global cb node
static bool32_t setCBFence(const XGL_CMD_BUFFER cb, const XGL_FENCE fence)
{
    GLOBAL_CB_NODE* pTrav = getGlobalCBNode(cb);
    if (!pTrav) {
        char str[1024];
        sprintf(str, "Unable to find node for CB %p in order to set fence to %p", (void*)cb, (void*)fence);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_CB, "MEM", str);
        return XGL_FALSE;
    }
    pTrav->fence = fence;
    return XGL_TRUE;
}

static bool32_t validateCBMemRef(const XGL_CMD_BUFFER cb, uint32_t memRefCount, const XGL_MEMORY_REF* pMemRefs)
{
    GLOBAL_CB_NODE* pTrav = getGlobalCBNode(cb);
    if (!pTrav) {
        char str[1024];
        sprintf(str, "Unable to find node for CB %p in order to check memory references", (void*)cb);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_CB, "MEM", str);
        return XGL_FALSE;
    }
    // Validate that all actual references are accounted for in pMemRefs
    MINI_NODE* pMemNode = pTrav->pMemObjList;
    uint32_t i;
    uint8_t found = 0;
    uint64_t foundCount = 0;
    while (pMemNode) {
        // TODO : Improve this algorithm
        for (i = 0; i < memRefCount; i++) {
            if (pMemNode->mem == pMemRefs[i].mem) {
                char str[1024];
                sprintf(str, "Found Mem Obj %p binding to CB %p", pMemNode->mem, cb);
                layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_NONE, "MEM", str);
                found = 1;
                foundCount++;
                break;
            }
        }
        if (!found) {
            char str[1024];
            sprintf(str, "Memory reference list for Command Buffer %p is missing ref to mem obj %p", cb, pMemNode->mem);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_CB_MISSING_MEM_REF, "MEM", str);
            return XGL_FALSE;
        }
        found = 0;
        pMemNode = pMemNode->pNext;
    }
    char str[1024];
    sprintf(str, "Verified all %lu memory dependencies for CB %p are included in pMemRefs list", foundCount, cb);
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_NONE, "MEM", str);
    // TODO : Could report mem refs in pMemRefs that AREN'T in mem LL, that would be primarily informational
    //   Currently just noting that there is a difference
    if (foundCount != memRefCount) {
        sprintf(str, "Note that %u mem refs included in pMemRefs list, but only %lu appear to be required", memRefCount, foundCount);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_NONE, "MEM", str);
    }
    return XGL_TRUE;
}
// Return ptr to node in global LL containing mem, or NULL if not found
static GLOBAL_MEM_OBJ_NODE* getGlobalMemNode(const XGL_GPU_MEMORY mem)
{
    GLOBAL_MEM_OBJ_NODE* pTrav = pGlobalMemObjHead;
    while (pTrav && (pTrav->mem != mem))
        pTrav = pTrav->pNextGlobalNode;
    return pTrav;
}

static void insertGlobalMemObj(const XGL_GPU_MEMORY mem, const XGL_MEMORY_ALLOC_INFO* pAllocInfo)
{
    GLOBAL_MEM_OBJ_NODE* pTrav = pGlobalMemObjHead;
    if (!pTrav) {
        pTrav = (GLOBAL_MEM_OBJ_NODE*)malloc(sizeof(GLOBAL_MEM_OBJ_NODE));
        pGlobalMemObjHead = pTrav;
    }
    else {
        while (NULL != pTrav->pNextGlobalNode)
            pTrav = pTrav->pNextGlobalNode;
        pTrav->pNextGlobalNode = (GLOBAL_MEM_OBJ_NODE*)malloc(sizeof(GLOBAL_MEM_OBJ_NODE));
        pTrav = pTrav->pNextGlobalNode;
    }
    if (!pTrav) {
        char str[1024];
        sprintf(str, "Malloc failed to alloc node for Mem Object %p", (void*)mem);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_OUT_OF_MEMORY_ERROR, "MEM", str);
    }
    else {
        numMemObjNodes++;
        memset(pTrav, 0, sizeof(GLOBAL_MEM_OBJ_NODE));
        if (pAllocInfo) {  // MEM alloc created by xglWsiX11CreatePresentableImage() doesn't have alloc info struct
            memcpy(&pTrav->allocInfo, pAllocInfo, sizeof(XGL_MEMORY_ALLOC_INFO));
            // TODO:  Update for real hardware, actually process allocation info structures
            pTrav->allocInfo.pNext = NULL;
        }
        pTrav->mem = mem;
    }
}

// Find Global CB Node and add mem binding to mini LL
// Find Global Mem Obj Node and add CB binding to mini LL
static bool32_t updateCBBinding(const XGL_CMD_BUFFER cb, const XGL_GPU_MEMORY mem)
{
    // First update CB binding in MemObj mini CB list
    GLOBAL_MEM_OBJ_NODE* pMemTrav = getGlobalMemNode(mem);
    if (!pMemTrav) {
        char str[1024];
        sprintf(str, "Trying to bind mem obj %p to CB %p but no Node for that mem obj.\n    Was it correctly allocated? Did it already get freed?", mem, cb);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
        return XGL_FALSE;
    }

    XGL_RESULT result = insertMiniNode(&pMemTrav->pCmdBufferBindings, cb, &pMemTrav->refCount);
    if (XGL_SUCCESS != result) {
        return result;
    }

    // Now update Global CB's Mini Mem binding list
    GLOBAL_CB_NODE* pCBTrav = getGlobalCBNode(cb);
    if (!pCBTrav) {
        char str[1024];
        sprintf(str, "Trying to bind mem obj %p to CB %p but no Node for that CB.    Was it CB incorrectly destroyed?", mem, cb);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
        return XGL_FALSE;
    }
    uint32_t dontCare;
    result = insertMiniNode(&pCBTrav->pMemObjList, mem, &dontCare);
    if (XGL_SUCCESS != result)
        return result;

    return XGL_TRUE;
}
// Clear the CB Binding for mem
static void clearCBBinding(const XGL_CMD_BUFFER cb, const XGL_GPU_MEMORY mem)
{
    GLOBAL_MEM_OBJ_NODE* pTrav = getGlobalMemNode(mem);
    MINI_NODE* pMiniCB = pTrav->pCmdBufferBindings;
    MINI_NODE* pPrev = pMiniCB;
    while (pMiniCB && (cb != pMiniCB->cmdBuffer)) {
        pPrev = pMiniCB;
        pMiniCB = pMiniCB->pNext;
    }
    if (!pMiniCB) {
        char str[1024];
        sprintf(str, "Trying to clear CB binding but CB %p not in binding list for mem obj %p", cb, mem);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INTERNAL_ERROR, "MEM", str);
    }
    else { // remove node from list & decrement refCount
        pPrev->pNext = pMiniCB->pNext;
        if (pMiniCB == pTrav->pCmdBufferBindings)
            pTrav->pCmdBufferBindings = NULL;
        free(pMiniCB);
        pTrav->refCount--;
    }
}
// Free bindings related to CB
static bool32_t freeCBBindings(const XGL_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pCBTrav = getGlobalCBNode(cb);
    if (!pCBTrav) {
        char str[1024];
        sprintf(str, "Unable to find global CB node %p for deletion", cb);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_CB, "MEM", str);
        return XGL_FALSE;
    }
    MINI_NODE* pMemTrav = pCBTrav->pMemObjList;
    MINI_NODE* pDeleteMe = NULL;
    // We traverse LL in order and free nodes as they're cleared
    while (pMemTrav) {
        pDeleteMe = pMemTrav;
        if (pMemTrav->mem)
            clearCBBinding(cb, pMemTrav->mem);
        pMemTrav = pMemTrav->pNext;
        free(pDeleteMe);
    }
    pCBTrav->pMemObjList = NULL;
    return XGL_TRUE;
}
// Delete Global CB Node from list along with all of it's mini mem obj node
//   and also clear Global mem references to CB
// TODO : When should this be called?  There's no Destroy of CBs that I see
static bool32_t deleteGlobalCBNode(const XGL_CMD_BUFFER cb)
{
    if (XGL_FALSE == freeCBBindings(cb)) {
        return XGL_FALSE;
    }
    // Delete the Global CB node
    GLOBAL_CB_NODE* pCBTrav = getGlobalCBNode(cb);
    pCBTrav = pGlobalCBHead;
    GLOBAL_CB_NODE* pPrev = pCBTrav;
    while (pCBTrav && (cb != pCBTrav->cmdBuffer)) {
        pPrev = pCBTrav;
        pCBTrav = pCBTrav->pNextGlobalCBNode;
    }
    assert(cb); // We found node at start of function so it should still be here
    pPrev->pNextGlobalCBNode = pCBTrav->pNextGlobalCBNode;
    if (pCBTrav == pGlobalCBHead)
        pGlobalCBHead = pCBTrav->pNextGlobalCBNode;
    free(pCBTrav);
    return XGL_TRUE;
}
// Delete the entire CB list
static bool32_t deleteGlobalCBList()
{
    bool32_t result = XGL_TRUE;
    GLOBAL_CB_NODE* pCBTrav = pGlobalCBHead;
    while (pCBTrav) {
        XGL_CMD_BUFFER cbToDelete = pCBTrav->cmdBuffer;
        pCBTrav = pCBTrav->pNextGlobalCBNode;
        bool32_t tmpResult = deleteGlobalCBNode(cbToDelete);
        // If any result is FALSE, final result should be FALSE
        if ((XGL_FALSE ==  tmpResult) || (XGL_FALSE == result))
            result = XGL_FALSE;
    }
    return result;
}

// For given MemObj node, report Obj & CB bindings
static void reportMemReferences(const GLOBAL_MEM_OBJ_NODE* pMemObjTrav)
{
    uint32_t refCount = 0; // Count found references
    MINI_NODE* pObjTrav = pMemObjTrav->pObjBindings;
    MINI_NODE* pCBTrav = pMemObjTrav->pCmdBufferBindings;
    while (pCBTrav) {
        refCount++;
        char str[1024];
        sprintf(str, "Command Buffer %p has reference to mem obj %p", pCBTrav->cmdBuffer, pMemObjTrav->mem);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pCBTrav->cmdBuffer, 0, MEMTRACK_NONE, "MEM", str);
        pCBTrav = pCBTrav->pNext;
    }
    while (pObjTrav) {
        refCount++;
        char str[1024];
        sprintf(str, "XGL Object %p has reference to mem obj %p", pObjTrav->object, pMemObjTrav->mem);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pObjTrav->object, 0, MEMTRACK_NONE, "MEM", str);
        pObjTrav = pObjTrav->pNext;
    }
    if (refCount != pMemObjTrav->refCount) {
        char str[1024];
        sprintf(str, "Refcount of %u for Mem Obj %p does't match reported refs of %u", pMemObjTrav->refCount, pMemObjTrav->mem, refCount);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pObjTrav->object, 0, MEMTRACK_INTERNAL_ERROR, "MEM", str);
    }
}

static void deleteGlobalMemNode(XGL_GPU_MEMORY mem)
{
    GLOBAL_MEM_OBJ_NODE* pTrav = pGlobalMemObjHead;
    GLOBAL_MEM_OBJ_NODE* pPrev = pTrav;
    while (pTrav && (pTrav->mem != mem)) {
        pPrev = pTrav;
        pTrav = pTrav->pNextGlobalNode;
    }
    if (pTrav) {
        pPrev->pNextGlobalNode = pTrav->pNextGlobalNode;
        if (pGlobalMemObjHead == pTrav)
            pGlobalMemObjHead = pTrav->pNextGlobalNode;
        free(pTrav);
    }
    else {
        char str[1024];
        sprintf(str, "Could not find global mem obj node for %p to delete!", mem);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
    }
}
// Check if fence for given CB is completed
static bool32_t checkCBCompleted(const XGL_CMD_BUFFER cb)
{
    GLOBAL_CB_NODE* pCBTrav = getGlobalCBNode(cb);
    if (!pCBTrav) {
        char str[1024];
        sprintf(str, "Unable to find global CB node %p to check for completion", cb);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_CB, "MEM", str);
        return XGL_FALSE;
    }
    if (!pCBTrav->fence) {
        char str[1024];
        sprintf(str, "No fence found for CB %p to check for completion", cb);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_CB_MISSING_FENCE, "MEM", str);
        return XGL_FALSE;
    }
    if (XGL_SUCCESS != nextTable.GetFenceStatus(pCBTrav->fence)) {
        char str[1024];
        sprintf(str, "Fence %p for CB %p has not completed", pCBTrav->fence, cb);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_NONE, "MEM", str);
        return XGL_FALSE;
    }
    return XGL_TRUE;
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
        return XGL_FALSE;
    }
    else {
        // First clear any CB bindings for completed CBs
        //   TODO : Is there a better place to do this?
        MINI_NODE* pMiniCB = pTrav->pCmdBufferBindings;
        while (pMiniCB) {
            XGL_CMD_BUFFER curCB = pMiniCB->cmdBuffer;
            pMiniCB = pMiniCB->pNext;
            if (XGL_TRUE == checkCBCompleted(curCB)) {
                freeCBBindings(curCB);
            }
        }
        // Now verify that no references to this mem obj remain
        if (0 != pTrav->refCount) {
            // If references remain, report the error and can search down CB LL to find references
            result = XGL_FALSE;
            char str[1024];
            sprintf(str, "Freeing mem obj %p while it still has references", (void*)mem);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_FREED_MEM_REF, "MEM", str);
            reportMemReferences(pTrav);
        }
        // Delete global node
        deleteGlobalMemNode(mem);
    }
    return result;
}

// Return object node for 'object' or return NULL if no node exists
static GLOBAL_OBJECT_NODE* getGlobalObjectNode(const XGL_OBJECT object)
{
    GLOBAL_OBJECT_NODE* pTrav = pGlobalObjectHead;
    while (pTrav && (object != pTrav->object)) {
        pTrav = pTrav->pNext;
    }
    return pTrav;
}

static GLOBAL_OBJECT_NODE* insertGlobalObjectNode(XGL_OBJECT object, XGL_STRUCTURE_TYPE sType, const void *pCreateInfo, const int struct_size, char *name_prefix)
{
    GLOBAL_OBJECT_NODE* pTrav = pGlobalObjectHead;
    if (!pTrav) {
        pTrav = (GLOBAL_OBJECT_NODE*)malloc(sizeof(GLOBAL_OBJECT_NODE));
        memset(pTrav, 0, sizeof(GLOBAL_OBJECT_NODE));
        pGlobalObjectHead = pTrav;
    }
    else {
        GLOBAL_OBJECT_NODE* pPrev = pTrav;
        while (pTrav) {
            pPrev = pTrav;
            pTrav = pTrav->pNext;
        }
        pTrav = (GLOBAL_OBJECT_NODE*)malloc(sizeof(GLOBAL_OBJECT_NODE));
        memset(pTrav, 0, sizeof(GLOBAL_OBJECT_NODE));
        pPrev->pNext = pTrav;
    }
    if (!pTrav) {
        char str[1024];
        sprintf(str, "Malloc failed to alloc node for XGL Object %p", (void*)object);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_OUT_OF_MEMORY_ERROR, "MEM", str);
        return NULL;
    }
    else {
        numObjectNodes++;
        pTrav->object = object;
        pTrav->ref_count = 1;
        pTrav->sType = sType;
        memcpy(&pTrav->create_info, pCreateInfo, struct_size);
        sprintf(pTrav->object_name, "%s_%p", name_prefix, object);
        return pTrav;
    }
}

// Remove object binding performs 3 tasks:
// 1. Remove object node from Global Mem Obj mini LL of obj bindings & free it
// 2. Decrement refCount for Global Mem Obj
// 3. Clear Global Mem Obj ptr from Global Object Node
static bool32_t clearObjectBinding(XGL_OBJECT object)
{
    GLOBAL_OBJECT_NODE* pGlobalObjTrav = getGlobalObjectNode(object);
    if (!pGlobalObjTrav) {
        char str[1024];
        sprintf(str, "Attempting to clear mem binding for object %p: devices, queues, command buffers, shaders and memory objects do not have external memory requirements and it is unneccessary to call bind/unbindObjectMemory on them.", object);
        layerCbMsg(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_INVALID_OBJECT, "MEM", str);
        return XGL_FALSE;
    }
    if (!pGlobalObjTrav->pMemNode) {
        char str[1024];
        sprintf(str, "Attempting to clear mem binding on obj %p but it has no binding.", (void*)object);
        layerCbMsg(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_MEM_OBJ_CLEAR_EMPTY_BINDINGS, "MEM", str);
        return XGL_FALSE;
    }
    MINI_NODE* pObjTrav = pGlobalObjTrav->pMemNode->pObjBindings;
    MINI_NODE* pPrevObj = pObjTrav;
    while (pObjTrav) {
        if (object == pObjTrav->object) {
            pPrevObj->pNext = pObjTrav->pNext;
            // check if HEAD needs to be updated
            if (pGlobalObjTrav->pMemNode->pObjBindings == pObjTrav)
                pGlobalObjTrav->pMemNode->pObjBindings = pObjTrav->pNext;
            free(pObjTrav);
            pGlobalObjTrav->pMemNode->refCount--;
            pGlobalObjTrav->pMemNode = NULL;
            return XGL_TRUE;
        }
        pPrevObj = pObjTrav;
        pObjTrav = pObjTrav->pNext;
    }
    char str[1024];
    sprintf(str, "While trying to clear mem binding for object %p, unable to find that object referenced by mem obj %p", object, pGlobalObjTrav->pMemNode->mem);
    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_INTERNAL_ERROR, "MEM", str);
    return XGL_FALSE;
}

// For NULL mem case, clear any previous binding Else...
// Make sure given object is in global object LL
//  IF a previous binding existed, clear it
//  Add link from global object node to global memory node
//  Add mini-object node & reference off of global obj node
// Return XGL_TRUE if addition is successful, XGL_FALSE otherwise
static bool32_t updateObjectBinding(XGL_OBJECT object, XGL_GPU_MEMORY mem)
{
    // Handle NULL case separately, just clear previous binding & decrement reference
    if (mem == XGL_NULL_HANDLE) {
        clearObjectBinding(object);
        return XGL_TRUE;
    }
    char str[1024];
    GLOBAL_OBJECT_NODE* pGlobalObjTrav = getGlobalObjectNode(object);
    assert(pGlobalObjTrav);
    if (!pGlobalObjTrav) {
        sprintf(str, "Attempting to update Binding of Obj(%p) that's not in global list()", (void*)object);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_INTERNAL_ERROR, "MEM", str);
    }
    // non-null case so should have real mem obj
    GLOBAL_MEM_OBJ_NODE* pTrav = getGlobalMemNode(mem);
    if (!pTrav) {
        sprintf(str, "While trying to bind mem for obj %p, couldn't find node for mem obj %p", (void*)object, (void*)mem);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
        return XGL_FALSE;
    }
    XGL_RESULT result = insertMiniNode(&pTrav->pObjBindings, object, &pTrav->refCount);
    if (XGL_SUCCESS != result)
        return result;

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
    return XGL_TRUE;
}
// Print details of global Obj tracking list
static void printObjList()
{
    GLOBAL_OBJECT_NODE* pGlobalObjTrav = pGlobalObjectHead;
    if (!pGlobalObjTrav) {
        char str[1024];
        sprintf(str, "Global Object list is empty :(\n");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
    }
    else {
        char str[1024];
        sprintf(str, "Details of Global Object list w/ HEAD at %p", (void*)pGlobalObjTrav);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        while (pGlobalObjTrav) {
            sprintf(str, "    GlobObjNode %p has object %p, pNext %p, pMemNode %p", pGlobalObjTrav, pGlobalObjTrav->object, pGlobalObjTrav->pNext, pGlobalObjTrav->pMemNode);
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pGlobalObjTrav->object, 0, MEMTRACK_NONE, "MEM", str);
            pGlobalObjTrav = pGlobalObjTrav->pNext;
        }
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
    GLOBAL_MEM_OBJ_NODE* pTrav = pGlobalMemObjHead;
    // Just printing each msg individually for now, may want to package these into single large print
    char str[1024];
    if (!pTrav) {
        sprintf(str, "MEM INFO : Global Memory Object list is empty :(\n");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
    }
    else {
        sprintf(str, "MEM INFO : Details of Global Memory Object list w/ HEAD at %p", (void*)pTrav);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        while (pTrav) {
            sprintf(str, "    ===MemObj Node at %p===", (void*)pTrav);
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
            sprintf(str, "    Mem object: %p", (void*)pTrav->mem);
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
            sprintf(str, "    Ref Count: %u", pTrav->refCount);
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
            sprintf(str, "    pNext Mem Obj Node: %p", (void*)pTrav->pNextGlobalNode);
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
            if (0 != pTrav->allocInfo.allocationSize)
                sprintf(str, "    Mem Alloc info:\n%s", xgl_print_xgl_memory_alloc_info(&pTrav->allocInfo, "{MEM}INFO :       "));
            else
                sprintf(str, "    Mem Alloc info is NULL (alloc done by xglWsiX11CreatePresentableImage())");
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
            MINI_NODE* pObjTrav = pTrav->pObjBindings;
            if (!pObjTrav) {
                sprintf(str, "    No XGL Object bindings");
                layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
            }
            else {
                sprintf(str, "    XGL OBJECT Binding list w/ HEAD at %p:", pObjTrav);
                layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
                while (pObjTrav) {
                    sprintf(str, "        OBJ_NODE(%p): XGL OBJECT %p, pNext %p", pObjTrav, pObjTrav->object, pObjTrav->pNext);
                    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
                    pObjTrav = pObjTrav->pNext;
                }
            }
            MINI_NODE* pCBTrav = pTrav->pCmdBufferBindings;
            if (!pCBTrav) {
                sprintf(str, "    No Command Buffer bindings");
                layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
            }
            else {
                sprintf(str, "    XGL Command Buffer (CB) binding list w/ HEAD at %p:", pCBTrav);
                layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
                while (pCBTrav) {
                    sprintf(str, "      CB_NODE(%p): XGL CB %p, pNext %p", pCBTrav, pCBTrav->cmdBuffer, pCBTrav->pNext);
                    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
                    pCBTrav = pCBTrav->pNext;
                }
            }
            pTrav = pTrav->pNextGlobalNode;
        }
    }
}

static void printGlobalCB()
{
    char str[1024] = {0};
    GLOBAL_CB_NODE* pTrav = pGlobalCBHead;
    if (!pTrav) {
        sprintf(str, "Global Command Buffer (CB) list is empty :(\n");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
    }
    else {
        sprintf(str, "Details of Global CB list w/ HEAD at %p:", (void*)pTrav);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
        while (pTrav) {
            sprintf(str, "    Global CB Node (%p) w/ pNextGlobalCBNode (%p) has CB %p, fence %p, and pMemObjList %p", (void*)pTrav, (void*)pTrav->pNextGlobalCBNode, (void*)pTrav->cmdBuffer, (void*)pTrav->fence, (void*)pTrav->pMemObjList);
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
            MINI_NODE* pMemObjTrav = pTrav->pMemObjList;
            while (pMemObjTrav) {
                sprintf(str, "      MEM_NODE(%p): Mem obj %p, pNext %p", (void*)pMemObjTrav, (void*)pMemObjTrav->mem, (void*)pMemObjTrav->pNext);
                layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, MEMTRACK_NONE, "MEM", str);
                pMemObjTrav = pMemObjTrav->pNext;
            }
            pTrav = pTrav->pNextGlobalCBNode;
        }
    }
}

static XGL_FENCE createLocalFence()
{
    XGL_FENCE_CREATE_INFO fci;
    fci.sType = XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.pNext = NULL;
    fci.flags = 0;
    XGL_FENCE fence;
    nextTable.CreateFence(globalDevice, &fci, &fence);
    return fence;
}

static void initMemTracker(void)
{
    const char *strOpt;
    // initialize MemTracker options
    strOpt = getLayerOption("MemTrackerReportLevel");
    if (strOpt != NULL)
        g_reportingLevel = atoi(strOpt);

    strOpt = getLayerOption("MemTrackerDebugAction");
    if (strOpt != NULL)
        g_debugAction = atoi(strOpt);

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

    xglGetProcAddrType fpGetProcAddr = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (char *) "xglGetProcAddr");
    nextTable.GetProcAddr = fpGetProcAddr;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateInstance(const XGL_APPLICATION_INFO* pAppInfo, const XGL_ALLOC_CALLBACKS* pAllocCb, XGL_INSTANCE* pInstance)
{
    XGL_RESULT result = nextTable.CreateInstance(pAppInfo, pAllocCb, pInstance);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyInstance(XGL_INSTANCE instance)
{
    XGL_RESULT result = nextTable.DestroyInstance(instance);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateGpus(XGL_INSTANCE instance, uint32_t maxGpus, uint32_t* pGpuCount, XGL_PHYSICAL_GPU* pGpus)
{
    XGL_RESULT result = nextTable.EnumerateGpus(instance, maxGpus, pGpuCount, pGpus);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetGpuInfo(XGL_PHYSICAL_GPU gpu, XGL_PHYSICAL_GPU_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initMemTracker);
    XGL_RESULT result = nextTable.GetGpuInfo((XGL_PHYSICAL_GPU)gpuw->nextObject, infoType, pDataSize, pData);
    return result;
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
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, device, 0, MEMTRACK_NONE, "MEM", str);
    printMemList();
    printGlobalCB();
    printObjList();
    if (XGL_FALSE == deleteGlobalCBList()) {
        sprintf(str, "Issue deleting global CB list in xglDestroyDevice()");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, device, 0, MEMTRACK_INTERNAL_ERROR, "MEM", str);
    }
    // Report any memory leaks
    GLOBAL_MEM_OBJ_NODE* pTrav = pGlobalMemObjHead;
    while (pTrav) {
        sprintf(str, "Mem Object %p has not been freed. You should clean up this memory by calling xglFreeMemory(%p) prior to xglDestroyDevice().", pTrav->mem, pTrav->mem);
        layerCbMsg(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, pTrav->mem, 0, MEMTRACK_MEMORY_LEAK, "MEM", str);
        pTrav = pTrav->pNextGlobalNode;
    }
    XGL_RESULT result = nextTable.DestroyDevice(device);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetExtensionSupport(XGL_PHYSICAL_GPU gpu, const char* pExtName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initMemTracker);
    XGL_RESULT result = nextTable.GetExtensionSupport((XGL_PHYSICAL_GPU)gpuw->nextObject, pExtName);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(XGL_PHYSICAL_GPU gpu, size_t maxLayerCount, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved)
{
        if (gpu != NULL)
    {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        pCurObj = gpuw;
        loader_platform_thread_once(&g_initOnce, initMemTracker);
        XGL_RESULT result = nextTable.EnumerateLayers((XGL_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetDeviceQueue(XGL_DEVICE device, XGL_QUEUE_TYPE queueType, uint32_t queueIndex, XGL_QUEUE* pQueue)
{
    XGL_RESULT result = nextTable.GetDeviceQueue(device, queueType, queueIndex, pQueue);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSubmit(XGL_QUEUE queue, uint32_t cmdBufferCount, const XGL_CMD_BUFFER* pCmdBuffers, uint32_t memRefCount, const XGL_MEMORY_REF* pMemRefs, XGL_FENCE fence)
{
    // TODO : Need to track fence and clear mem references when fence clears
    XGL_FENCE localFence = fence;
    if (XGL_NULL_HANDLE == fence) { // allocate our own fence to track cmd buffer
        localFence = createLocalFence();
    }
    char str[1024];
    sprintf(str, "In xglQueueSubmit(), checking %u cmdBuffers with %u memRefs", cmdBufferCount, memRefCount);
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, queue, 0, MEMTRACK_NONE, "MEM", str);
    printMemList();
    printGlobalCB();
    for (uint32_t i = 0; i < cmdBufferCount; i++) {
        setCBFence(pCmdBuffers[i], localFence);
        sprintf(str, "Verifying mem refs for CB %p", pCmdBuffers[i]);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pCmdBuffers[i], 0, MEMTRACK_NONE, "MEM", str);
        if (XGL_FALSE == validateCBMemRef(pCmdBuffers[i], memRefCount, pMemRefs)) {
            sprintf(str, "Unable to verify memory references for CB %p", (void*)pCmdBuffers[i]);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pCmdBuffers[i], 0, MEMTRACK_CB_MISSING_MEM_REF, "MEM", str);
        }
    }
    printGlobalCB();
    XGL_RESULT result = nextTable.QueueSubmit(queue, cmdBufferCount, pCmdBuffers, memRefCount, pMemRefs, localFence);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSetGlobalMemReferences(XGL_QUEUE queue, uint32_t memRefCount, const XGL_MEMORY_REF* pMemRefs)
{
    // TODO : Use global mem references as part of list checked on QueueSubmit above
    XGL_RESULT result = nextTable.QueueSetGlobalMemReferences(queue, memRefCount, pMemRefs);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueWaitIdle(XGL_QUEUE queue)
{
    XGL_RESULT result = nextTable.QueueWaitIdle(queue);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDeviceWaitIdle(XGL_DEVICE device)
{
    XGL_RESULT result = nextTable.DeviceWaitIdle(device);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglAllocMemory(XGL_DEVICE device, const XGL_MEMORY_ALLOC_INFO* pAllocInfo, XGL_GPU_MEMORY* pMem)
{
    XGL_RESULT result = nextTable.AllocMemory(device, pAllocInfo, pMem);
    // TODO : Track allocations and overall size here
    insertGlobalMemObj(*pMem, pAllocInfo);
    printMemList();
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglFreeMemory(XGL_GPU_MEMORY mem)
{
    /* From spec : A memory object is freed by calling xglFreeMemory() when it is no longer needed. Before
     * freeing a memory object, an application must ensure the memory object is unbound from
     * all API objects referencing it and that it is not referenced by any queued command buffers
     */
    if (XGL_FALSE == freeMemNode(mem)) {
        char str[1024];
        sprintf(str, "Issue while freeing mem obj %p", (void*)mem);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_FREE_MEM_ERROR, "MEM", str);
    }
    printMemList();
    printObjList();
    printGlobalCB();
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetMultiGpuCompatibility(XGL_PHYSICAL_GPU gpu0, XGL_PHYSICAL_GPU gpu1, XGL_GPU_COMPATIBILITY_INFO* pInfo)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu0;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initMemTracker);
    XGL_RESULT result = nextTable.GetMultiGpuCompatibility((XGL_PHYSICAL_GPU)gpuw->nextObject, gpu1, pInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenSharedMemory(XGL_DEVICE device, const XGL_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem)
{
    // TODO : Track this
    XGL_RESULT result = nextTable.OpenSharedMemory(device, pOpenInfo, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenSharedQueueSemaphore(XGL_DEVICE device, const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo, XGL_QUEUE_SEMAPHORE* pSemaphore)
{
    XGL_RESULT result = nextTable.OpenSharedQueueSemaphore(device, pOpenInfo, pSemaphore);
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
    // First check if this is a CmdBuffer
    if (NULL != getGlobalCBNode((XGL_CMD_BUFFER)object)) {
        deleteGlobalCBNode((XGL_CMD_BUFFER)object);
    }
    // Now locate node in global list along with prev node
    GLOBAL_OBJECT_NODE* pTrav = pGlobalObjectHead;
    GLOBAL_OBJECT_NODE* pPrev = pTrav;
    while (pTrav) {
        if (object == pTrav->object)
            break;
        pPrev = pTrav;
        pTrav = pTrav->pNext;
    }
    if (pTrav) {
        if (pTrav->pMemNode) {
            // Wsi allocated Memory is tied to image object so clear the binding and free that memory automatically
            if (0 == pTrav->pMemNode->allocInfo.allocationSize) { // Wsi allocated memory has NULL allocInfo w/ 0 size
                XGL_GPU_MEMORY memToFree = pTrav->pMemNode->mem;
                clearObjectBinding(object);
                freeMemNode(memToFree);
            }
            else {
                char str[1024];
                sprintf(str, "Destroying obj %p that is still bound to memory object %p\nYou should first clear binding by calling xglBindObjectMemory(%p, 0, XGL_NULL_HANDLE)", object, (void*)pTrav->pMemNode->mem, object);
                layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_DESTROY_OBJECT_ERROR, "MEM", str);
                // From the spec : If an object has previous memory binding, it is required to unbind memory from an API object before it is destroyed.
                clearObjectBinding(object);
            }
        }
        if (pGlobalObjectHead == pTrav) // update HEAD if needed
            pGlobalObjectHead = pTrav->pNext;
        // Delete the obj node from global list
        pPrev->pNext = pTrav->pNext;
        free(pTrav);
    }
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
    // Track objects tied to memory
    if (XGL_FALSE == updateObjectBinding(object, mem)) {
        char str[1024];
        sprintf(str, "Unable to set object %p binding to mem obj %p", (void*)object, (void*)mem);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    printObjList();
    printMemList();
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindObjectMemoryRange(XGL_OBJECT object, uint32_t allocationIdx, XGL_GPU_SIZE rangeOffset, XGL_GPU_SIZE rangeSize, XGL_GPU_MEMORY mem, XGL_GPU_SIZE memOffset)
{
    XGL_RESULT result = nextTable.BindObjectMemoryRange(object, allocationIdx, rangeOffset, rangeSize, mem, memOffset);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindImageMemoryRange(XGL_IMAGE image, uint32_t allocationIdx, const XGL_IMAGE_MEMORY_BIND_INFO* bindInfo, XGL_GPU_MEMORY mem, XGL_GPU_SIZE memOffset)
{
    XGL_RESULT result = nextTable.BindImageMemoryRange(image, allocationIdx, bindInfo, mem, memOffset);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateFence(XGL_DEVICE device, const XGL_FENCE_CREATE_INFO* pCreateInfo, XGL_FENCE* pFence)
{
    XGL_RESULT result = nextTable.CreateFence(device, pCreateInfo, pFence);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFenceStatus(XGL_FENCE fence)
{
    XGL_RESULT result = nextTable.GetFenceStatus(fence);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWaitForFences(XGL_DEVICE device, uint32_t fenceCount, const XGL_FENCE* pFences, bool32_t waitAll, uint64_t timeout)
{
    XGL_RESULT result = nextTable.WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateQueueSemaphore(XGL_DEVICE device, const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo, XGL_QUEUE_SEMAPHORE* pSemaphore)
{
    XGL_RESULT result = nextTable.CreateQueueSemaphore(device, pCreateInfo, pSemaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSignalQueueSemaphore(XGL_QUEUE queue, XGL_QUEUE_SEMAPHORE semaphore)
{
    XGL_RESULT result = nextTable.SignalQueueSemaphore(queue, semaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWaitQueueSemaphore(XGL_QUEUE queue, XGL_QUEUE_SEMAPHORE semaphore)
{
    XGL_RESULT result = nextTable.WaitQueueSemaphore(queue, semaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateEvent(XGL_DEVICE device, const XGL_EVENT_CREATE_INFO* pCreateInfo, XGL_EVENT* pEvent)
{
    XGL_RESULT result = nextTable.CreateEvent(device, pCreateInfo, pEvent);
    if (XGL_SUCCESS == result) {
        insertGlobalObjectNode(*pEvent, pCreateInfo->sType, pCreateInfo, sizeof(XGL_EVENT_CREATE_INFO), "event");
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetEventStatus(XGL_EVENT event)
{
    XGL_RESULT result = nextTable.GetEventStatus(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetEvent(XGL_EVENT event)
{
    XGL_RESULT result = nextTable.SetEvent(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglResetEvent(XGL_EVENT event)
{
    XGL_RESULT result = nextTable.ResetEvent(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateQueryPool(XGL_DEVICE device, const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo, XGL_QUERY_POOL* pQueryPool)
{
    XGL_RESULT result = nextTable.CreateQueryPool(device, pCreateInfo, pQueryPool);
    if (XGL_SUCCESS == result) {
        insertGlobalObjectNode(*pQueryPool, pCreateInfo->sType, pCreateInfo, sizeof(XGL_QUERY_POOL_CREATE_INFO), "query_pool");
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetQueryPoolResults(XGL_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount, size_t* pDataSize, void* pData)
{
    XGL_RESULT result = nextTable.GetQueryPoolResults(queryPool, startQuery, queryCount, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFormatInfo(XGL_DEVICE device, XGL_FORMAT format, XGL_FORMAT_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    XGL_RESULT result = nextTable.GetFormatInfo(device, format, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateBuffer(XGL_DEVICE device, const XGL_BUFFER_CREATE_INFO* pCreateInfo, XGL_BUFFER* pBuffer)
{
    XGL_RESULT result = nextTable.CreateBuffer(device, pCreateInfo, pBuffer);
    if (XGL_SUCCESS == result) {
        insertGlobalObjectNode(*pBuffer, pCreateInfo->sType, pCreateInfo, sizeof(XGL_BUFFER_CREATE_INFO), "buffer");
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateBufferView(XGL_DEVICE device, const XGL_BUFFER_VIEW_CREATE_INFO* pCreateInfo, XGL_BUFFER_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateBufferView(device, pCreateInfo, pView);
    if (result == XGL_SUCCESS) {
        insertGlobalObjectNode(*pView, pCreateInfo->sType, pCreateInfo, sizeof(XGL_BUFFER_VIEW_CREATE_INFO), "buffer_view");
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImage(XGL_DEVICE device, const XGL_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage)
{
    XGL_RESULT result = nextTable.CreateImage(device, pCreateInfo, pImage);
    if (XGL_SUCCESS == result) {
        insertGlobalObjectNode(*pImage, pCreateInfo->sType, pCreateInfo, sizeof(XGL_IMAGE_CREATE_INFO), "image");
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetFastClearColor(XGL_IMAGE image, const float color[4])
{
    XGL_RESULT result = nextTable.SetFastClearColor(image, color);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetFastClearDepth(XGL_IMAGE image, float depth)
{
    XGL_RESULT result = nextTable.SetFastClearDepth(image, depth);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetImageSubresourceInfo(XGL_IMAGE image, const XGL_IMAGE_SUBRESOURCE* pSubresource, XGL_SUBRESOURCE_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    XGL_RESULT result = nextTable.GetImageSubresourceInfo(image, pSubresource, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImageView(XGL_DEVICE device, const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo, XGL_IMAGE_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateImageView(device, pCreateInfo, pView);
    if (result == XGL_SUCCESS) {
        insertGlobalObjectNode(*pView, pCreateInfo->sType, pCreateInfo, sizeof(XGL_IMAGE_VIEW_CREATE_INFO), "image_view");
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateColorAttachmentView(XGL_DEVICE device, const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo, XGL_COLOR_ATTACHMENT_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateColorAttachmentView(device, pCreateInfo, pView);
    if (result == XGL_SUCCESS) {
        insertGlobalObjectNode(*pView, pCreateInfo->sType, pCreateInfo, sizeof(XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO), "color_attachment_view");
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDepthStencilView(XGL_DEVICE device, const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo, XGL_DEPTH_STENCIL_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateDepthStencilView(device, pCreateInfo, pView);
    if (result == XGL_SUCCESS) {
        insertGlobalObjectNode(*pView, pCreateInfo->sType, pCreateInfo, sizeof(XGL_DEPTH_STENCIL_VIEW_CREATE_INFO), "ds_view");
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
        insertGlobalObjectNode(*pPipeline, pCreateInfo->sType, pCreateInfo, sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO), "graphics_pipeline");
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateComputePipeline(XGL_DEVICE device, const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline)
{
    XGL_RESULT result = nextTable.CreateComputePipeline(device, pCreateInfo, pPipeline);
    if (result == XGL_SUCCESS) {
        insertGlobalObjectNode(*pPipeline, pCreateInfo->sType, pCreateInfo, sizeof(XGL_COMPUTE_PIPELINE_CREATE_INFO), "compute_pipeline");
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglStorePipeline(XGL_PIPELINE pipeline, size_t* pDataSize, void* pData)
{
    XGL_RESULT result = nextTable.StorePipeline(pipeline, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglLoadPipeline(XGL_DEVICE device, size_t dataSize, const void* pData, XGL_PIPELINE* pPipeline)
{
    XGL_RESULT result = nextTable.LoadPipeline(device, dataSize, pData, pPipeline);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreatePipelineDelta(XGL_DEVICE device, XGL_PIPELINE p1, XGL_PIPELINE p2, XGL_PIPELINE_DELTA* delta)
{
    XGL_RESULT result = nextTable.CreatePipelineDelta(device, p1, p2, delta);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateSampler(XGL_DEVICE device, const XGL_SAMPLER_CREATE_INFO* pCreateInfo, XGL_SAMPLER* pSampler)
{
    XGL_RESULT result = nextTable.CreateSampler(device, pCreateInfo, pSampler);
    if (result == XGL_SUCCESS) {
        insertGlobalObjectNode(*pSampler, pCreateInfo->sType, pCreateInfo, sizeof(XGL_SAMPLER_CREATE_INFO), "sampler");
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorSetLayout(XGL_DEVICE device, XGL_FLAGS stageFlags, const uint32_t* pSetBindPoints, XGL_DESCRIPTOR_SET_LAYOUT priorSetLayout, const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pSetLayoutInfoList, XGL_DESCRIPTOR_SET_LAYOUT* pSetLayout)
{
    XGL_RESULT result = nextTable.CreateDescriptorSetLayout(device, stageFlags, pSetBindPoints, priorSetLayout, pSetLayoutInfoList, pSetLayout);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBeginDescriptorRegionUpdate(XGL_DEVICE device, XGL_DESCRIPTOR_UPDATE_MODE updateMode)
{
    XGL_RESULT result = nextTable.BeginDescriptorRegionUpdate(device, updateMode);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEndDescriptorRegionUpdate(XGL_DEVICE device, XGL_CMD_BUFFER cmd)
{
    XGL_RESULT result = nextTable.EndDescriptorRegionUpdate(device, cmd);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorRegion(XGL_DEVICE device, XGL_DESCRIPTOR_REGION_USAGE regionUsage, uint32_t maxSets, const XGL_DESCRIPTOR_REGION_CREATE_INFO* pCreateInfo, XGL_DESCRIPTOR_REGION* pDescriptorRegion)
{
    XGL_RESULT result = nextTable.CreateDescriptorRegion(device, regionUsage, maxSets, pCreateInfo, pDescriptorRegion);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglClearDescriptorRegion(XGL_DESCRIPTOR_REGION descriptorRegion)
{
    XGL_RESULT result = nextTable.ClearDescriptorRegion(descriptorRegion);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglAllocDescriptorSets(XGL_DESCRIPTOR_REGION descriptorRegion, XGL_DESCRIPTOR_SET_USAGE setUsage, uint32_t count, const XGL_DESCRIPTOR_SET_LAYOUT* pSetLayouts, XGL_DESCRIPTOR_SET* pDescriptorSets, uint32_t* pCount)
{
    XGL_RESULT result = nextTable.AllocDescriptorSets(descriptorRegion, setUsage, count, pSetLayouts, pDescriptorSets, pCount);
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglClearDescriptorSets(XGL_DESCRIPTOR_REGION descriptorRegion, uint32_t count, const XGL_DESCRIPTOR_SET* pDescriptorSets)
{
    nextTable.ClearDescriptorSets(descriptorRegion, count, pDescriptorSets);
}

XGL_LAYER_EXPORT void XGLAPI xglUpdateDescriptors(XGL_DESCRIPTOR_SET descriptorSet, const void* pUpdateChain)
{
    nextTable.UpdateDescriptors(descriptorSet, pUpdateChain);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicViewportState(XGL_DEVICE device, const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_VP_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateDynamicViewportState(device, pCreateInfo, pState);
    if (result == XGL_SUCCESS) {
        insertGlobalObjectNode(*pState, pCreateInfo->sType, pCreateInfo, sizeof(XGL_DYNAMIC_VP_STATE_CREATE_INFO), "viewport_state");
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicRasterState(XGL_DEVICE device, const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_RS_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateDynamicRasterState(device, pCreateInfo, pState);
    if (result == XGL_SUCCESS) {
        insertGlobalObjectNode(*pState, pCreateInfo->sType, pCreateInfo, sizeof(XGL_DYNAMIC_RS_STATE_CREATE_INFO), "raster_state");
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicColorBlendState(XGL_DEVICE device, const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pCreateInfo,  XGL_DYNAMIC_CB_STATE_OBJECT*  pState)
{
    XGL_RESULT result = nextTable.CreateDynamicColorBlendState(device, pCreateInfo, pState);
    if (result == XGL_SUCCESS) {
        insertGlobalObjectNode(*pState, pCreateInfo->sType, pCreateInfo, sizeof(XGL_DYNAMIC_CB_STATE_CREATE_INFO), "cb_state");
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicDepthStencilState(XGL_DEVICE device, const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pCreateInfo,    XGL_DYNAMIC_DS_STATE_OBJECT*    pState)
{
    XGL_RESULT result = nextTable.CreateDynamicDepthStencilState(device, pCreateInfo, pState);
    if (result == XGL_SUCCESS) {
        insertGlobalObjectNode(*pState, pCreateInfo->sType, pCreateInfo, sizeof(XGL_DYNAMIC_DS_STATE_CREATE_INFO), "ds_state");
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateCommandBuffer(XGL_DEVICE device, const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo, XGL_CMD_BUFFER* pCmdBuffer)
{
    XGL_RESULT result = nextTable.CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    // At time of cmd buffer creation, create global cmd buffer node for the returned cmd buffer
    if (*pCmdBuffer)
        insertGlobalCB(*pCmdBuffer);
    printGlobalCB();
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBeginCommandBuffer(XGL_CMD_BUFFER cmdBuffer, const XGL_CMD_BUFFER_BEGIN_INFO* pBeginInfo)
{
    // This implicitly resets the Cmd Buffer so clear memory references
    XGL_RESULT result = nextTable.BeginCommandBuffer(cmdBuffer, pBeginInfo);
    freeCBBindings(cmdBuffer);
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
    // Clear memory references as this point.  Anything else to do here?
    freeCBBindings(cmdBuffer);
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

XGL_LAYER_EXPORT void XGLAPI xglCmdBindPipelineDelta(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE_DELTA delta)
{
    nextTable.CmdBindPipelineDelta(cmdBuffer, pipelineBindPoint, delta);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindDynamicStateObject(XGL_CMD_BUFFER cmdBuffer, XGL_STATE_BIND_POINT stateBindPoint, XGL_DYNAMIC_STATE_OBJECT state)
{
    GLOBAL_OBJECT_NODE *pNode;
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
    nextTable.CmdBindDynamicStateObject(cmdBuffer, stateBindPoint, state);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindDescriptorSet(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_DESCRIPTOR_SET descriptorSet, const uint32_t* pUserData)
{
    // TODO : Somewhere need to verify that all textures referenced by shaders in DS are in some type of *SHADER_READ* state
    nextTable.CmdBindDescriptorSet(cmdBuffer, pipelineBindPoint, descriptorSet, pUserData);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindVertexBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t binding)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(buffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdBindVertexBuffer() call unable to update binding of buffer %p to cmdBuffer %p", buffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    // Now update CB's vertex binding list
    GLOBAL_CB_NODE* pCBTrav = getGlobalCBNode(cmdBuffer);
    if (!pCBTrav) {
        char str[1024];
        sprintf(str, "Trying to BindVertexuffer obj %p to CB %p but no Node for that CB. Was CB incorrectly destroyed?", buffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_INVALID_CB, "MEM", str);
    } else {
        MEMORY_BINDING *pBindInfo;
        uint32_t dontCare;
        XGL_RESULT result;
        pBindInfo = malloc(sizeof(MEMORY_BINDING));
        pBindInfo->offset = offset;
        pBindInfo->binding = binding;
        pBindInfo->buffer = buffer;
        result = insertMiniNode(&pCBTrav->pVertexBufList, pBindInfo, &dontCare);
        if (result) {
            char str[1024];
            sprintf(str, "In xglCmdBindVertexBuffer and ran out of memory to track binding. CmdBuffer: %p, buffer %p", cmdBuffer, buffer);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_OUT_OF_MEMORY_ERROR, "MEM", str);
        }
    }
    lastVtxBinding = binding;
    nextTable.CmdBindVertexBuffer(cmdBuffer, buffer, offset, binding);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindIndexBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, XGL_INDEX_TYPE indexType)
{
    // Track this buffer. What exactly is this call doing?
    XGL_GPU_MEMORY mem = getMemBindingFromObject(buffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdBindIndexData() call unable to update binding of buffer %p to cmdBuffer %p", buffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    // Now update CB's index binding list
    GLOBAL_CB_NODE* pCBTrav = getGlobalCBNode(cmdBuffer);
    if (!pCBTrav) {
        char str[1024];
        sprintf(str, "Trying to BindIndexData buffer obj %p to CB %p but no Node for that CB. Was CB incorrectly destroyed?", buffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_INVALID_MEM_OBJ, (char *) "MEM", (char *) str);
    } else {
        MEMORY_BINDING *pBindInfo;
        uint32_t dontCare;
        XGL_RESULT result;
        pBindInfo = malloc(sizeof(MEMORY_BINDING));
        pBindInfo->indexType = indexType;
        pBindInfo->buffer = buffer;
        pBindInfo->offset = offset;
        pBindInfo->binding = 0;
        result = insertMiniNode(&pCBTrav->pIndexBufList, pBindInfo, &dontCare);
        if (result) {
            char str[1024];
            sprintf(str, "In xglCmdBindIndexData and ran out of memory to track binding. CmdBuffer: %p, buffer %p", cmdBuffer, buffer);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_OUT_OF_MEMORY_ERROR, "MEM", str);
        }
    }

    nextTable.CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}



XGL_LAYER_EXPORT void XGLAPI xglCmdDraw(XGL_CMD_BUFFER cmdBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount)
{
    nextTable.CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDrawIndexed(XGL_CMD_BUFFER cmdBuffer, uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount)
{
    nextTable.CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDrawIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t count, uint32_t stride)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(buffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdDrawIndirect() call unable to update binding of buffer %p to cmdBuffer %p", buffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDrawIndexedIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t count, uint32_t stride)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(buffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdDrawIndexedIndirect() call unable to update binding of buffer %p to cmdBuffer %p", buffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDispatch(XGL_CMD_BUFFER cmdBuffer, uint32_t x, uint32_t y, uint32_t z)
{
    nextTable.CmdDispatch(cmdBuffer, x, y, z);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDispatchIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(buffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdDispatchIndirect() call unable to update binding of buffer %p to cmdBuffer %p", buffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER srcBuffer, XGL_BUFFER destBuffer, uint32_t regionCount, const XGL_BUFFER_COPY* pRegions)
{
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
    nextTable.CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, uint32_t regionCount, const XGL_IMAGE_COPY* pRegions)
{
    // TODO : Each image will have mem mapping so track them
    nextTable.CmdCopyImage(cmdBuffer, srcImage, destImage, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyBufferToImage(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER srcBuffer, XGL_IMAGE destImage, uint32_t regionCount, const XGL_BUFFER_IMAGE_COPY* pRegions)
{
    // TODO : Track this
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
    nextTable.CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyImageToBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_BUFFER destBuffer, uint32_t regionCount, const XGL_BUFFER_IMAGE_COPY* pRegions)
{
    // TODO : Track this
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
    nextTable.CmdCopyImageToBuffer(cmdBuffer, srcImage, destBuffer, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCloneImageData(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE_LAYOUT srcImageLayout, XGL_IMAGE destImage, XGL_IMAGE_LAYOUT destImageLayout)
{
    // TODO : Each image will have mem mapping so track them
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
    nextTable.CmdCloneImageData(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdUpdateBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE dataSize, const uint32_t* pData)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(destBuffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdUpdateMemory() call unable to update binding of destBuffer %p to cmdBuffer %p", destBuffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdFillBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE fillSize, uint32_t data)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(destBuffer);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdFillMemory() call unable to update binding of destBuffer %p to cmdBuffer %p", destBuffer, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdClearColorImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const float color[4], uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    // TODO : Verify memory is in XGL_IMAGE_STATE_CLEAR state
    XGL_GPU_MEMORY mem = getMemBindingFromObject(image);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdClearColorImage() call unable to update binding of image buffer %p to cmdBuffer %p", image, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdClearColorImage(cmdBuffer, image, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdClearColorImageRaw(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const uint32_t color[4], uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    // TODO : Verify memory is in XGL_IMAGE_STATE_CLEAR state
    XGL_GPU_MEMORY mem = getMemBindingFromObject(image);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdClearColorImageRaw() call unable to update binding of image buffer %p to cmdBuffer %p", image, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdClearColorImageRaw(cmdBuffer, image, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdClearDepthStencil(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, float depth, uint32_t stencil, uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    // TODO : Verify memory is in XGL_IMAGE_STATE_CLEAR state
    XGL_GPU_MEMORY mem = getMemBindingFromObject(image);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdClearDepthStencil() call unable to update binding of image buffer %p to cmdBuffer %p", image, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdClearDepthStencil(cmdBuffer, image, depth, stencil, rangeCount, pRanges);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdResolveImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, uint32_t rectCount, const XGL_IMAGE_RESOLVE* pRects)
{
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
    nextTable.CmdResolveImage(cmdBuffer, srcImage, destImage, rectCount, pRects);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdSetEvent(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event, XGL_SET_EVENT pipeEvent)
{
    nextTable.CmdSetEvent(cmdBuffer, event, pipeEvent);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdResetEvent(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event)
{
    nextTable.CmdResetEvent(cmdBuffer, event);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdWaitEvents(XGL_CMD_BUFFER cmdBuffer, const XGL_EVENT_WAIT_INFO* pWaitInfo)
{
    nextTable.CmdWaitEvents(cmdBuffer, pWaitInfo);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdPipelineBarrier(XGL_CMD_BUFFER cmdBuffer, const XGL_PIPELINE_BARRIER* pBarrier)
{
    nextTable.CmdPipelineBarrier(cmdBuffer, pBarrier);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBeginQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t slot, XGL_FLAGS flags)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(queryPool);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdBeginQuery() call unable to update binding of queryPool buffer %p to cmdBuffer %p", queryPool, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdEndQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t slot)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(queryPool);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdEndQuery() call unable to update binding of queryPool buffer %p to cmdBuffer %p", queryPool, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdEndQuery(cmdBuffer, queryPool, slot);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdResetQueryPool(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(queryPool);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdResetQueryPool() call unable to update binding of queryPool buffer %p to cmdBuffer %p", queryPool, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdWriteTimestamp(XGL_CMD_BUFFER cmdBuffer, XGL_TIMESTAMP_TYPE timestampType, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset)
{
    nextTable.CmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdInitAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, const uint32_t* pData)
{
    nextTable.CmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdLoadAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, XGL_BUFFER srcBuffer, XGL_GPU_SIZE srcOffset)
{
    nextTable.CmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcBuffer, srcOffset);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdSaveAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset)
{
    nextTable.CmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destBuffer, destOffset);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateFramebuffer(XGL_DEVICE device, const XGL_FRAMEBUFFER_CREATE_INFO* pCreateInfo, XGL_FRAMEBUFFER* pFramebuffer)
{
    XGL_RESULT result = nextTable.CreateFramebuffer(device, pCreateInfo, pFramebuffer);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateRenderPass(XGL_DEVICE device, const XGL_RENDER_PASS_CREATE_INFO* pCreateInfo, XGL_RENDER_PASS* pRenderPass)
{
    XGL_RESULT result = nextTable.CreateRenderPass(device, pCreateInfo, pRenderPass);
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBeginRenderPass(XGL_CMD_BUFFER cmdBuffer, XGL_RENDER_PASS renderPass)
{
    nextTable.CmdBeginRenderPass(cmdBuffer, renderPass);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdEndRenderPass(XGL_CMD_BUFFER cmdBuffer, XGL_RENDER_PASS renderPass)
{
    nextTable.CmdEndRenderPass(cmdBuffer, renderPass);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetValidationLevel(XGL_DEVICE device, XGL_VALIDATION_LEVEL validationLevel)
{
    XGL_RESULT result = nextTable.DbgSetValidationLevel(device, validationLevel);
    return result;
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
    XGL_RESULT result = nextTable.DbgUnregisterMsgCallback(pfnMsgCallback);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetMessageFilter(XGL_DEVICE device, int32_t msgCode, XGL_DBG_MSG_FILTER filter)
{
    XGL_RESULT result = nextTable.DbgSetMessageFilter(device, msgCode, filter);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetObjectTag(XGL_BASE_OBJECT object, size_t tagSize, const void* pTag)
{
    XGL_RESULT result = nextTable.DbgSetObjectTag(object, tagSize, pTag);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetGlobalOption(XGL_DBG_GLOBAL_OPTION dbgOption, size_t dataSize, const void* pData)
{
    XGL_RESULT result = nextTable.DbgSetGlobalOption(dbgOption, dataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetDeviceOption(XGL_DEVICE device, XGL_DBG_DEVICE_OPTION dbgOption, size_t dataSize, const void* pData)
{
    XGL_RESULT result = nextTable.DbgSetDeviceOption(device, dbgOption, dataSize, pData);
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDbgMarkerBegin(XGL_CMD_BUFFER cmdBuffer, const char* pMarker)
{
    nextTable.CmdDbgMarkerBegin(cmdBuffer, pMarker);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDbgMarkerEnd(XGL_CMD_BUFFER cmdBuffer)
{
    nextTable.CmdDbgMarkerEnd(cmdBuffer);
}

#if defined(_WIN32)
// FIXME: NEED WINDOWS EQUIVALENT
#else // WIN32
XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11AssociateConnection(XGL_PHYSICAL_GPU gpu, const XGL_WSI_X11_CONNECTION_INFO* pConnectionInfo)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initMemTracker);
    XGL_RESULT result = nextTable.WsiX11AssociateConnection((XGL_PHYSICAL_GPU)gpuw->nextObject, pConnectionInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11GetMSC(XGL_DEVICE device, xcb_window_t window, xcb_randr_crtc_t crtc, uint64_t* pMsc)
{
    XGL_RESULT result = nextTable.WsiX11GetMSC(device, window, crtc, pMsc);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11CreatePresentableImage(XGL_DEVICE device, const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem)
{
    XGL_RESULT result = nextTable.WsiX11CreatePresentableImage(device, pCreateInfo, pImage, pMem);
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
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11QueuePresent(XGL_QUEUE queue, const XGL_WSI_X11_PRESENT_INFO* pPresentInfo, XGL_FENCE fence)
{
    XGL_RESULT result = nextTable.WsiX11QueuePresent(queue, pPresentInfo, fence);
    return result;
}
#endif // WIN32

XGL_LAYER_EXPORT void* XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const char* funcName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    void *addr;

    if (gpu == NULL)
        return NULL;
    pCurObj = gpuw;
    loader_platform_thread_once(&g_initOnce, initMemTracker);

    addr = layer_intercept_proc(funcName);
    if (addr)
        return addr;
    else {
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA(gpuw->nextObject, funcName);
    }
}
