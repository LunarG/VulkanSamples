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
#include <pthread.h>
#include "xgl_struct_string_helper.h"
#include "mem_tracker.h"
#include "layers_config.h"

static XGL_LAYER_DISPATCH_TABLE nextTable;
static XGL_BASE_LAYER_OBJECT *pCurObj;
static pthread_once_t g_initOnce = PTHREAD_ONCE_INIT;

// Ptr to LL of dbg functions
static XGL_LAYER_DBG_FUNCTION_NODE *g_pDbgFunctionHead = NULL;
static XGL_LAYER_DBG_REPORT_LEVEL g_reportingLevel = XGL_DBG_LAYER_LEVEL_ERROR;
static XGL_LAYER_DBG_ACTION g_debugAction = XGL_DBG_LAYER_ACTION_LOG_MSG;
static FILE *g_logFile = NULL;

// Utility function to handle reporting
static XGL_VOID layerCbMsg(XGL_DBG_MSG_TYPE msgType,
    XGL_VALIDATION_LEVEL validationLevel,
    XGL_BASE_OBJECT      srcObject,
    XGL_SIZE             location,
    XGL_INT              msgCode,
    const char*          pLayerPrefix,
    const char*          pMsg)
{
    if (g_debugAction & (XGL_DBG_LAYER_ACTION_LOG_MSG | XGL_DBG_LAYER_ACTION_CALLBACK)) {
         XGL_LAYER_DBG_FUNCTION_NODE *pTrav = g_pDbgFunctionHead;
         switch (msgType) {
            case XGL_DBG_MSG_ERROR:
                if (g_reportingLevel <= XGL_DBG_LAYER_LEVEL_ERROR) {
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_LOG_MSG)
                        fprintf(g_logFile, "{%s}ERROR : %s\n", pLayerPrefix, pMsg);
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_CALLBACK)
                        while (pTrav) {
                            pTrav->pfnMsgCallback(msgType, validationLevel, srcObject, location, msgCode, pMsg, pTrav->pUserData);
                            pTrav = pTrav->pNext;
                        }
                }
                break;
            case XGL_DBG_MSG_WARNING:
                if (g_reportingLevel <= XGL_DBG_LAYER_LEVEL_WARN) {
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_LOG_MSG)
                        fprintf(g_logFile, "{%s}WARN : %s\n", pLayerPrefix, pMsg);
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_CALLBACK)
                        while (pTrav) {
                            pTrav->pfnMsgCallback(msgType, validationLevel, srcObject, location, msgCode, pMsg, pTrav->pUserData);
                            pTrav = pTrav->pNext;
                        }
                }
                break;
            case XGL_DBG_MSG_PERF_WARNING:
                if (g_reportingLevel <= XGL_DBG_LAYER_LEVEL_PERF_WARN) {
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_LOG_MSG)
                        fprintf(g_logFile, "{%s}PERF_WARN : %s\n", pLayerPrefix, pMsg);
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_CALLBACK)
                        while (pTrav) {
                            pTrav->pfnMsgCallback(msgType, validationLevel, srcObject, location, msgCode, pMsg, pTrav->pUserData);
                            pTrav = pTrav->pNext;
                        }
                }
                break;
            default:
                if (g_reportingLevel <= XGL_DBG_LAYER_LEVEL_INFO) {
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_LOG_MSG)
                        fprintf(g_logFile, "{%s}INFO : %s\n", pLayerPrefix, pMsg);
                    if (g_debugAction & XGL_DBG_LAYER_ACTION_CALLBACK)
                        while (pTrav) {
                            pTrav->pfnMsgCallback(msgType, validationLevel, srcObject, location, msgCode, pMsg, pTrav->pUserData);
                            pTrav = pTrav->pNext;
                        }
                }
                break;
        }
    }
}
/*
 * Data Structure overview
 *  There are 3 global Linked-Lists (LLs)
 *  pGlobalCBHead points to head of Command Buffer (CB) LL
 *    Off of each node in this LL there is a separate LL of
 *    memory objects that are referenced by this CB
 *  pGlobalMemObjHead points to head of Memory Object LL
 *    Off of each node in this LL there are 2 separate LL
 *    One is a list of all CBs referencing this mem obj
 *    Two is a list of all XGL Objects that are bound to this memory
 *  pGlobalObjHead point to head of XGL Objects w/ bound mem LL
 *    Each node of this LL contains a ptr to global Mem Obj node for bound mem
 *
 * The "Global" nodes are for the main LLs
 * The "mini" nodes are for ancillary LLs that are pointed to from global nodes
 *
 * Algorithm overview
 * These are the primary events that should happen related to different objects
 * 1. Command buffers
 *   CREATION - Add node to global LL
 *   CMD BIND - If mem associated, add mem reference to mini LL
 *   DESTROY - Remove from global LL, decrement (and report) mem references
 * 2. Mem Objects
 *   CREATION - Add node to global LL
 *   OBJ BIND - Add obj node to mini LL for that mem node
 *   CMB BIND - If mem-related add CB node to mini LL for that mem node
 *   DESTROY - Flag as errors any remaining refs and Remove from global LL
 * 3. Generic Objects
 *   MEM BIND - DESTROY any previous binding, Add global obj node w/ ref to global mem obj node, Add obj node to mini LL for that mem node
 *   DESTROY - If mem bound, remove reference from mini LL for that mem Node, remove global obj node
 */
// TODO : Is there a way to track when Cmd Buffer finishes & remove mem references at that point?
// TODO : Could potentially store a list of freed mem allocs to flag when they're incorrectly used

// Generic data struct for various "mini" Linked-Lists
//  This just wraps some type of XGL OBJ and a pNext ptr
//  Used for xgl obj, cmd buffer, and mem obj wrapping
typedef struct _MINI_NODE {
    struct _MINI_NODE* pNext;
    union { // different objects that can be wrapped
        XGL_OBJECT object;
        XGL_GPU_MEMORY mem;
        XGL_CMD_BUFFER cmdBuffer;
        XGL_BASE_OBJECT data;     // for generic handling of data
    };
} MINI_NODE;

struct GLOBAL_MEM_OBJ_NODE;

// Store a single LL of command buffers
typedef struct _GLOBAL_CB_NODE {
    struct _GLOBAL_CB_NODE* pNextGlobalCBNode;
    MINI_NODE* pMemObjList; // LL of Mem objs referenced by this CB
    XGL_CMD_BUFFER cmdBuffer;
    XGL_FENCE fence; // fence tracking this cmd buffer
} GLOBAL_CB_NODE;

// Data struct for tracking memory object
typedef struct _GLOBAL_MEM_OBJ_NODE {
    struct _GLOBAL_MEM_OBJ_NODE* pNextGlobalNode; // Ptr to next mem obj in global list of all objs
    MINI_NODE* pObjBindings; // Ptr to list of objects bound to this memory
    MINI_NODE* pCmdBufferBindings; // Ptr to list of cmd buffers that this mem object references
    XGL_UINT refCount; // Count of references (obj bindings or CB use)
    XGL_GPU_MEMORY mem;
    XGL_MEMORY_ALLOC_INFO allocInfo;
} GLOBAL_MEM_OBJ_NODE;

typedef struct _GLOBAL_OBJECT_NODE {
    struct _GLOBAL_OBJECT_NODE* pNext;
    GLOBAL_MEM_OBJ_NODE* pMemNode;
    XGL_OBJECT object;
} GLOBAL_OBJECT_NODE;

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
static XGL_RESULT insertMiniNode(MINI_NODE** pHEAD, const XGL_BASE_OBJECT data, XGL_UINT* insert)
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
static XGL_BOOL setCBFence(const XGL_CMD_BUFFER cb, const XGL_FENCE fence)
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

static XGL_BOOL validateCBMemRef(const XGL_CMD_BUFFER cb, XGL_UINT memRefCount, const XGL_MEMORY_REF* pMemRefs)
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
        if (pAllocInfo) // MEM alloc created by xglWsiX11CreatePresentableImage() doesn't have alloc info struct
            memcpy(&pTrav->allocInfo, pAllocInfo, sizeof(XGL_MEMORY_ALLOC_INFO));
        pTrav->mem = mem;
    }
}

// Return ptr to node in global LL containing mem, or NULL if not found
static GLOBAL_MEM_OBJ_NODE* getGlobalMemNode(const XGL_GPU_MEMORY mem)
{
    GLOBAL_MEM_OBJ_NODE* pTrav = pGlobalMemObjHead;
    while (pTrav && (pTrav->mem != mem))
        pTrav = pTrav->pNextGlobalNode;
    return pTrav;
}

// Find Global CB Node and add mem binding to mini LL
// Find Global Mem Obj Node and add CB binding to mini LL
static XGL_BOOL updateCBBinding(const XGL_CMD_BUFFER cb, const XGL_GPU_MEMORY mem)
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
    if (XGL_SUCCESS != result)
        return result;

    // Now update Global CB's Mini Mem binding list
    GLOBAL_CB_NODE* pCBTrav = getGlobalCBNode(cb);
    if (!pCBTrav) {
        char str[1024];
        sprintf(str, "Trying to bind mem obj %p to CB %p but no Node for that CB.    Was it CB incorrectly destroyed?", mem, cb);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cb, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
        return XGL_FALSE;
    }
    XGL_UINT dontCare;
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
static XGL_BOOL freeCBBindings(const XGL_CMD_BUFFER cb)
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
static XGL_BOOL deleteGlobalCBNode(const XGL_CMD_BUFFER cb)
{
    if (XGL_FALSE == freeCBBindings(cb))
        return XGL_FALSE;
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
static XGL_BOOL deleteGlobalCBList()
{
    XGL_BOOL result = XGL_TRUE;
    GLOBAL_CB_NODE* pCBTrav = pGlobalCBHead;
    while (pCBTrav) {
        XGL_CMD_BUFFER cbToDelete = pCBTrav->cmdBuffer;
        pCBTrav = pCBTrav->pNextGlobalCBNode;
        XGL_BOOL tmpResult = deleteGlobalCBNode(cbToDelete);
        // If any result is FALSE, final result should be FALSE
        if ((XGL_FALSE ==  tmpResult) || (XGL_FALSE == result))
            result = XGL_FALSE;
    }
    return result;
}

// For given MemObj node, report Obj & CB bindings
static void reportMemReferences(const GLOBAL_MEM_OBJ_NODE* pMemObjTrav)
{
    XGL_UINT refCount = 0; // Count found references
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
static XGL_BOOL checkCBCompleted(const XGL_CMD_BUFFER cb)
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

static XGL_BOOL freeMemNode(XGL_GPU_MEMORY mem)
{
    XGL_BOOL result = XGL_TRUE;
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

static GLOBAL_OBJECT_NODE* insertGlobalObjectNode(XGL_OBJECT object)
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
        return pTrav;
    }
}

// Remove object binding performs 3 tasks:
// 1. Remove object node from Global Mem Obj mini LL of obj bindings & free it
// 2. Decrement refCount for Global Mem Obj
// 3. Clear Global Mem Obj ptr from Global Object Node
static XGL_BOOL clearObjectBinding(XGL_OBJECT object)
{
    GLOBAL_OBJECT_NODE* pGlobalObjTrav = getGlobalObjectNode(object);
    if (!pGlobalObjTrav) {
        char str[1024];
        sprintf(str, "Attempting to clear mem binding for object %p", object);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
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
static XGL_BOOL updateObjectBinding(XGL_OBJECT object, XGL_GPU_MEMORY mem)
{
    // Handle NULL case separately, just clear previous binding & decrement reference
    if (mem == XGL_NULL_HANDLE) {
        clearObjectBinding(object);
        return XGL_TRUE;
    }
    // Find obj node or add it if we don't have one
    GLOBAL_OBJECT_NODE* pGlobalObjTrav = getGlobalObjectNode(object);
    if (!pGlobalObjTrav)
        pGlobalObjTrav = insertGlobalObjectNode(object);

    // non-null case so should have real mem obj
    GLOBAL_MEM_OBJ_NODE* pTrav = getGlobalMemNode(mem);
    if (!pTrav) {
        char str[1024];
        sprintf(str, "While trying to bind mem for obj %p, couldn't find node for mem obj %p", (void*)object, (void*)mem);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, mem, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM", str);
        return XGL_FALSE;
    }
    XGL_RESULT result = insertMiniNode(&pTrav->pObjBindings, object, &pTrav->refCount);
    if (XGL_SUCCESS != result)
        return result;

    if (pGlobalObjTrav->pMemNode) {
        clearObjectBinding(object); // Need to clear the previous object binding before setting new binding
        char str[1024];
        sprintf(str, "Updating memory binding for object %p from mem obj %p to %p", object, pGlobalObjTrav->pMemNode->mem, mem);
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, object, 0, MEMTRACK_NONE, "MEM", str);
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

static void initMemTracker()
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
    GetProcAddrType fpNextGPA;
    fpNextGPA = pCurObj->pGPA;
    assert(fpNextGPA);

    GetProcAddrType fpGetProcAddr = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetProcAddr");
    nextTable.GetProcAddr = fpGetProcAddr;
    InitAndEnumerateGpusType fpInitAndEnumerateGpus = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglInitAndEnumerateGpus");
    nextTable.InitAndEnumerateGpus = fpInitAndEnumerateGpus;
    GetGpuInfoType fpGetGpuInfo = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetGpuInfo");
    nextTable.GetGpuInfo = fpGetGpuInfo;
    CreateDeviceType fpCreateDevice = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateDevice");
    nextTable.CreateDevice = fpCreateDevice;
    DestroyDeviceType fpDestroyDevice = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDestroyDevice");
    nextTable.DestroyDevice = fpDestroyDevice;
    GetExtensionSupportType fpGetExtensionSupport = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetExtensionSupport");
    nextTable.GetExtensionSupport = fpGetExtensionSupport;
    EnumerateLayersType fpEnumerateLayers = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglEnumerateLayers");
    nextTable.EnumerateLayers = fpEnumerateLayers;
    GetDeviceQueueType fpGetDeviceQueue = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetDeviceQueue");
    nextTable.GetDeviceQueue = fpGetDeviceQueue;
    QueueSubmitType fpQueueSubmit = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglQueueSubmit");
    nextTable.QueueSubmit = fpQueueSubmit;
    QueueSetGlobalMemReferencesType fpQueueSetGlobalMemReferences = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglQueueSetGlobalMemReferences");
    nextTable.QueueSetGlobalMemReferences = fpQueueSetGlobalMemReferences;
    QueueWaitIdleType fpQueueWaitIdle = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglQueueWaitIdle");
    nextTable.QueueWaitIdle = fpQueueWaitIdle;
    DeviceWaitIdleType fpDeviceWaitIdle = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDeviceWaitIdle");
    nextTable.DeviceWaitIdle = fpDeviceWaitIdle;
    GetMemoryHeapCountType fpGetMemoryHeapCount = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetMemoryHeapCount");
    nextTable.GetMemoryHeapCount = fpGetMemoryHeapCount;
    GetMemoryHeapInfoType fpGetMemoryHeapInfo = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetMemoryHeapInfo");
    nextTable.GetMemoryHeapInfo = fpGetMemoryHeapInfo;
    AllocMemoryType fpAllocMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglAllocMemory");
    nextTable.AllocMemory = fpAllocMemory;
    FreeMemoryType fpFreeMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglFreeMemory");
    nextTable.FreeMemory = fpFreeMemory;
    SetMemoryPriorityType fpSetMemoryPriority = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglSetMemoryPriority");
    nextTable.SetMemoryPriority = fpSetMemoryPriority;
    MapMemoryType fpMapMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglMapMemory");
    nextTable.MapMemory = fpMapMemory;
    UnmapMemoryType fpUnmapMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglUnmapMemory");
    nextTable.UnmapMemory = fpUnmapMemory;
    PinSystemMemoryType fpPinSystemMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglPinSystemMemory");
    nextTable.PinSystemMemory = fpPinSystemMemory;
    RemapVirtualMemoryPagesType fpRemapVirtualMemoryPages = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglRemapVirtualMemoryPages");
    nextTable.RemapVirtualMemoryPages = fpRemapVirtualMemoryPages;
    GetMultiGpuCompatibilityType fpGetMultiGpuCompatibility = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetMultiGpuCompatibility");
    nextTable.GetMultiGpuCompatibility = fpGetMultiGpuCompatibility;
    OpenSharedMemoryType fpOpenSharedMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglOpenSharedMemory");
    nextTable.OpenSharedMemory = fpOpenSharedMemory;
    OpenSharedQueueSemaphoreType fpOpenSharedQueueSemaphore = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglOpenSharedQueueSemaphore");
    nextTable.OpenSharedQueueSemaphore = fpOpenSharedQueueSemaphore;
    OpenPeerMemoryType fpOpenPeerMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglOpenPeerMemory");
    nextTable.OpenPeerMemory = fpOpenPeerMemory;
    OpenPeerImageType fpOpenPeerImage = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglOpenPeerImage");
    nextTable.OpenPeerImage = fpOpenPeerImage;
    DestroyObjectType fpDestroyObject = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDestroyObject");
    nextTable.DestroyObject = fpDestroyObject;
    GetObjectInfoType fpGetObjectInfo = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetObjectInfo");
    nextTable.GetObjectInfo = fpGetObjectInfo;
    BindObjectMemoryType fpBindObjectMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglBindObjectMemory");
    nextTable.BindObjectMemory = fpBindObjectMemory;
    CreateFenceType fpCreateFence = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateFence");
    nextTable.CreateFence = fpCreateFence;
    GetFenceStatusType fpGetFenceStatus = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetFenceStatus");
    nextTable.GetFenceStatus = fpGetFenceStatus;
    WaitForFencesType fpWaitForFences = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglWaitForFences");
    nextTable.WaitForFences = fpWaitForFences;
    CreateQueueSemaphoreType fpCreateQueueSemaphore = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateQueueSemaphore");
    nextTable.CreateQueueSemaphore = fpCreateQueueSemaphore;
    SignalQueueSemaphoreType fpSignalQueueSemaphore = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglSignalQueueSemaphore");
    nextTable.SignalQueueSemaphore = fpSignalQueueSemaphore;
    WaitQueueSemaphoreType fpWaitQueueSemaphore = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglWaitQueueSemaphore");
    nextTable.WaitQueueSemaphore = fpWaitQueueSemaphore;
    CreateEventType fpCreateEvent = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateEvent");
    nextTable.CreateEvent = fpCreateEvent;
    GetEventStatusType fpGetEventStatus = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetEventStatus");
    nextTable.GetEventStatus = fpGetEventStatus;
    SetEventType fpSetEvent = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglSetEvent");
    nextTable.SetEvent = fpSetEvent;
    ResetEventType fpResetEvent = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglResetEvent");
    nextTable.ResetEvent = fpResetEvent;
    CreateQueryPoolType fpCreateQueryPool = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateQueryPool");
    nextTable.CreateQueryPool = fpCreateQueryPool;
    GetQueryPoolResultsType fpGetQueryPoolResults = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetQueryPoolResults");
    nextTable.GetQueryPoolResults = fpGetQueryPoolResults;
    GetFormatInfoType fpGetFormatInfo = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetFormatInfo");
    nextTable.GetFormatInfo = fpGetFormatInfo;
    CreateImageType fpCreateImage = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateImage");
    nextTable.CreateImage = fpCreateImage;
    GetImageSubresourceInfoType fpGetImageSubresourceInfo = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetImageSubresourceInfo");
    nextTable.GetImageSubresourceInfo = fpGetImageSubresourceInfo;
    CreateImageViewType fpCreateImageView = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateImageView");
    nextTable.CreateImageView = fpCreateImageView;
    CreateColorAttachmentViewType fpCreateColorAttachmentView = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateColorAttachmentView");
    nextTable.CreateColorAttachmentView = fpCreateColorAttachmentView;
    CreateDepthStencilViewType fpCreateDepthStencilView = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateDepthStencilView");
    nextTable.CreateDepthStencilView = fpCreateDepthStencilView;
    CreateShaderType fpCreateShader = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateShader");
    nextTable.CreateShader = fpCreateShader;
    CreateGraphicsPipelineType fpCreateGraphicsPipeline = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateGraphicsPipeline");
    nextTable.CreateGraphicsPipeline = fpCreateGraphicsPipeline;
    CreateComputePipelineType fpCreateComputePipeline = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateComputePipeline");
    nextTable.CreateComputePipeline = fpCreateComputePipeline;
    StorePipelineType fpStorePipeline = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglStorePipeline");
    nextTable.StorePipeline = fpStorePipeline;
    LoadPipelineType fpLoadPipeline = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglLoadPipeline");
    nextTable.LoadPipeline = fpLoadPipeline;
    CreatePipelineDeltaType fpCreatePipelineDelta = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreatePipelineDelta");
    nextTable.CreatePipelineDelta = fpCreatePipelineDelta;
    CreateSamplerType fpCreateSampler = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateSampler");
    nextTable.CreateSampler = fpCreateSampler;
    CreateDescriptorSetType fpCreateDescriptorSet = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateDescriptorSet");
    nextTable.CreateDescriptorSet = fpCreateDescriptorSet;
    BeginDescriptorSetUpdateType fpBeginDescriptorSetUpdate = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglBeginDescriptorSetUpdate");
    nextTable.BeginDescriptorSetUpdate = fpBeginDescriptorSetUpdate;
    EndDescriptorSetUpdateType fpEndDescriptorSetUpdate = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglEndDescriptorSetUpdate");
    nextTable.EndDescriptorSetUpdate = fpEndDescriptorSetUpdate;
    AttachSamplerDescriptorsType fpAttachSamplerDescriptors = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglAttachSamplerDescriptors");
    nextTable.AttachSamplerDescriptors = fpAttachSamplerDescriptors;
    AttachImageViewDescriptorsType fpAttachImageViewDescriptors = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglAttachImageViewDescriptors");
    nextTable.AttachImageViewDescriptors = fpAttachImageViewDescriptors;
    AttachMemoryViewDescriptorsType fpAttachMemoryViewDescriptors = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglAttachMemoryViewDescriptors");
    nextTable.AttachMemoryViewDescriptors = fpAttachMemoryViewDescriptors;
    AttachNestedDescriptorsType fpAttachNestedDescriptors = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglAttachNestedDescriptors");
    nextTable.AttachNestedDescriptors = fpAttachNestedDescriptors;
    ClearDescriptorSetSlotsType fpClearDescriptorSetSlots = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglClearDescriptorSetSlots");
    nextTable.ClearDescriptorSetSlots = fpClearDescriptorSetSlots;
    CreateViewportStateType fpCreateViewportState = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateViewportState");
    nextTable.CreateViewportState = fpCreateViewportState;
    CreateRasterStateType fpCreateRasterState = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateRasterState");
    nextTable.CreateRasterState = fpCreateRasterState;
    CreateMsaaStateType fpCreateMsaaState = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateMsaaState");
    nextTable.CreateMsaaState = fpCreateMsaaState;
    CreateColorBlendStateType fpCreateColorBlendState = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateColorBlendState");
    nextTable.CreateColorBlendState = fpCreateColorBlendState;
    CreateDepthStencilStateType fpCreateDepthStencilState = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateDepthStencilState");
    nextTable.CreateDepthStencilState = fpCreateDepthStencilState;
    CreateCommandBufferType fpCreateCommandBuffer = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateCommandBuffer");
    nextTable.CreateCommandBuffer = fpCreateCommandBuffer;
    BeginCommandBufferType fpBeginCommandBuffer = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglBeginCommandBuffer");
    nextTable.BeginCommandBuffer = fpBeginCommandBuffer;
    EndCommandBufferType fpEndCommandBuffer = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglEndCommandBuffer");
    nextTable.EndCommandBuffer = fpEndCommandBuffer;
    ResetCommandBufferType fpResetCommandBuffer = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglResetCommandBuffer");
    nextTable.ResetCommandBuffer = fpResetCommandBuffer;
    CmdBindPipelineType fpCmdBindPipeline = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBindPipeline");
    nextTable.CmdBindPipeline = fpCmdBindPipeline;
    CmdBindPipelineDeltaType fpCmdBindPipelineDelta = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBindPipelineDelta");
    nextTable.CmdBindPipelineDelta = fpCmdBindPipelineDelta;
    CmdBindStateObjectType fpCmdBindStateObject = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBindStateObject");
    nextTable.CmdBindStateObject = fpCmdBindStateObject;
    CmdBindDescriptorSetType fpCmdBindDescriptorSet = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBindDescriptorSet");
    nextTable.CmdBindDescriptorSet = fpCmdBindDescriptorSet;
    CmdBindDynamicMemoryViewType fpCmdBindDynamicMemoryView = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBindDynamicMemoryView");
    nextTable.CmdBindDynamicMemoryView = fpCmdBindDynamicMemoryView;
    CmdBindIndexDataType fpCmdBindIndexData = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBindIndexData");
    nextTable.CmdBindIndexData = fpCmdBindIndexData;
    CmdBindAttachmentsType fpCmdBindAttachments = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBindAttachments");
    nextTable.CmdBindAttachments = fpCmdBindAttachments;
    CmdPrepareMemoryRegionsType fpCmdPrepareMemoryRegions = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdPrepareMemoryRegions");
    nextTable.CmdPrepareMemoryRegions = fpCmdPrepareMemoryRegions;
    CmdPrepareImagesType fpCmdPrepareImages = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdPrepareImages");
    nextTable.CmdPrepareImages = fpCmdPrepareImages;
    CmdDrawType fpCmdDraw = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDraw");
    nextTable.CmdDraw = fpCmdDraw;
    CmdDrawIndexedType fpCmdDrawIndexed = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDrawIndexed");
    nextTable.CmdDrawIndexed = fpCmdDrawIndexed;
    CmdDrawIndirectType fpCmdDrawIndirect = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDrawIndirect");
    nextTable.CmdDrawIndirect = fpCmdDrawIndirect;
    CmdDrawIndexedIndirectType fpCmdDrawIndexedIndirect = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDrawIndexedIndirect");
    nextTable.CmdDrawIndexedIndirect = fpCmdDrawIndexedIndirect;
    CmdDispatchType fpCmdDispatch = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDispatch");
    nextTable.CmdDispatch = fpCmdDispatch;
    CmdDispatchIndirectType fpCmdDispatchIndirect = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDispatchIndirect");
    nextTable.CmdDispatchIndirect = fpCmdDispatchIndirect;
    CmdCopyMemoryType fpCmdCopyMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdCopyMemory");
    nextTable.CmdCopyMemory = fpCmdCopyMemory;
    CmdCopyImageType fpCmdCopyImage = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdCopyImage");
    nextTable.CmdCopyImage = fpCmdCopyImage;
    CmdCopyMemoryToImageType fpCmdCopyMemoryToImage = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdCopyMemoryToImage");
    nextTable.CmdCopyMemoryToImage = fpCmdCopyMemoryToImage;
    CmdCopyImageToMemoryType fpCmdCopyImageToMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdCopyImageToMemory");
    nextTable.CmdCopyImageToMemory = fpCmdCopyImageToMemory;
    CmdCloneImageDataType fpCmdCloneImageData = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdCloneImageData");
    nextTable.CmdCloneImageData = fpCmdCloneImageData;
    CmdUpdateMemoryType fpCmdUpdateMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdUpdateMemory");
    nextTable.CmdUpdateMemory = fpCmdUpdateMemory;
    CmdFillMemoryType fpCmdFillMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdFillMemory");
    nextTable.CmdFillMemory = fpCmdFillMemory;
    CmdClearColorImageType fpCmdClearColorImage = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdClearColorImage");
    nextTable.CmdClearColorImage = fpCmdClearColorImage;
    CmdClearColorImageRawType fpCmdClearColorImageRaw = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdClearColorImageRaw");
    nextTable.CmdClearColorImageRaw = fpCmdClearColorImageRaw;
    CmdClearDepthStencilType fpCmdClearDepthStencil = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdClearDepthStencil");
    nextTable.CmdClearDepthStencil = fpCmdClearDepthStencil;
    CmdResolveImageType fpCmdResolveImage = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdResolveImage");
    nextTable.CmdResolveImage = fpCmdResolveImage;
    CmdSetEventType fpCmdSetEvent = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdSetEvent");
    nextTable.CmdSetEvent = fpCmdSetEvent;
    CmdResetEventType fpCmdResetEvent = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdResetEvent");
    nextTable.CmdResetEvent = fpCmdResetEvent;
    CmdMemoryAtomicType fpCmdMemoryAtomic = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdMemoryAtomic");
    nextTable.CmdMemoryAtomic = fpCmdMemoryAtomic;
    CmdBeginQueryType fpCmdBeginQuery = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBeginQuery");
    nextTable.CmdBeginQuery = fpCmdBeginQuery;
    CmdEndQueryType fpCmdEndQuery = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdEndQuery");
    nextTable.CmdEndQuery = fpCmdEndQuery;
    CmdResetQueryPoolType fpCmdResetQueryPool = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdResetQueryPool");
    nextTable.CmdResetQueryPool = fpCmdResetQueryPool;
    CmdWriteTimestampType fpCmdWriteTimestamp = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdWriteTimestamp");
    nextTable.CmdWriteTimestamp = fpCmdWriteTimestamp;
    CmdInitAtomicCountersType fpCmdInitAtomicCounters = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdInitAtomicCounters");
    nextTable.CmdInitAtomicCounters = fpCmdInitAtomicCounters;
    CmdLoadAtomicCountersType fpCmdLoadAtomicCounters = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdLoadAtomicCounters");
    nextTable.CmdLoadAtomicCounters = fpCmdLoadAtomicCounters;
    CmdSaveAtomicCountersType fpCmdSaveAtomicCounters = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdSaveAtomicCounters");
    nextTable.CmdSaveAtomicCounters = fpCmdSaveAtomicCounters;
    DbgSetValidationLevelType fpDbgSetValidationLevel = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDbgSetValidationLevel");
    nextTable.DbgSetValidationLevel = fpDbgSetValidationLevel;
    DbgRegisterMsgCallbackType fpDbgRegisterMsgCallback = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDbgRegisterMsgCallback");
    nextTable.DbgRegisterMsgCallback = fpDbgRegisterMsgCallback;
    DbgUnregisterMsgCallbackType fpDbgUnregisterMsgCallback = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDbgUnregisterMsgCallback");
    nextTable.DbgUnregisterMsgCallback = fpDbgUnregisterMsgCallback;
    DbgSetMessageFilterType fpDbgSetMessageFilter = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDbgSetMessageFilter");
    nextTable.DbgSetMessageFilter = fpDbgSetMessageFilter;
    DbgSetObjectTagType fpDbgSetObjectTag = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDbgSetObjectTag");
    nextTable.DbgSetObjectTag = fpDbgSetObjectTag;
    DbgSetGlobalOptionType fpDbgSetGlobalOption = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDbgSetGlobalOption");
    nextTable.DbgSetGlobalOption = fpDbgSetGlobalOption;
    DbgSetDeviceOptionType fpDbgSetDeviceOption = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDbgSetDeviceOption");
    nextTable.DbgSetDeviceOption = fpDbgSetDeviceOption;
    CmdDbgMarkerBeginType fpCmdDbgMarkerBegin = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDbgMarkerBegin");
    nextTable.CmdDbgMarkerBegin = fpCmdDbgMarkerBegin;
    CmdDbgMarkerEndType fpCmdDbgMarkerEnd = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDbgMarkerEnd");
    nextTable.CmdDbgMarkerEnd = fpCmdDbgMarkerEnd;
    WsiX11AssociateConnectionType fpWsiX11AssociateConnection = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglWsiX11AssociateConnection");
    nextTable.WsiX11AssociateConnection = fpWsiX11AssociateConnection;
    WsiX11GetMSCType fpWsiX11GetMSC = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglWsiX11GetMSC");
    nextTable.WsiX11GetMSC = fpWsiX11GetMSC;
    WsiX11CreatePresentableImageType fpWsiX11CreatePresentableImage = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglWsiX11CreatePresentableImage");
    nextTable.WsiX11CreatePresentableImage = fpWsiX11CreatePresentableImage;
    WsiX11QueuePresentType fpWsiX11QueuePresent = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglWsiX11QueuePresent");
    nextTable.WsiX11QueuePresent = fpWsiX11QueuePresent;
}


XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetGpuInfo(XGL_PHYSICAL_GPU gpu, XGL_PHYSICAL_GPU_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    pthread_once(&g_initOnce, initMemTracker);
    XGL_RESULT result = nextTable.GetGpuInfo((XGL_PHYSICAL_GPU)gpuw->nextObject, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo, XGL_DEVICE* pDevice)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    pthread_once(&g_initOnce, initMemTracker);
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetExtensionSupport(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pExtName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    pthread_once(&g_initOnce, initMemTracker);
    XGL_RESULT result = nextTable.GetExtensionSupport((XGL_PHYSICAL_GPU)gpuw->nextObject, pExtName);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(XGL_PHYSICAL_GPU gpu, XGL_SIZE maxLayerCount, XGL_SIZE maxStringSize, XGL_CHAR* const* pOutLayers, XGL_SIZE * pOutLayerCount, XGL_VOID* pReserved)
{
        if (gpu != NULL)
    {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        pCurObj = gpuw;
        pthread_once(&g_initOnce, initMemTracker);
        XGL_RESULT result = nextTable.EnumerateLayers((XGL_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayers, pOutLayerCount, pReserved);
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetDeviceQueue(XGL_DEVICE device, XGL_QUEUE_TYPE queueType, XGL_UINT queueIndex, XGL_QUEUE* pQueue)
{
    XGL_RESULT result = nextTable.GetDeviceQueue(device, queueType, queueIndex, pQueue);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSubmit(XGL_QUEUE queue, XGL_UINT cmdBufferCount, const XGL_CMD_BUFFER* pCmdBuffers, XGL_UINT memRefCount, const XGL_MEMORY_REF* pMemRefs, XGL_FENCE fence)
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSetGlobalMemReferences(XGL_QUEUE queue, XGL_UINT memRefCount, const XGL_MEMORY_REF* pMemRefs)
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetMemoryHeapCount(XGL_DEVICE device, XGL_UINT* pCount)
{
    // TODO : Track memory stats here
    XGL_RESULT result = nextTable.GetMemoryHeapCount(device, pCount);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetMemoryHeapInfo(XGL_DEVICE device, XGL_UINT heapId, XGL_MEMORY_HEAP_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    // TODO : Track memory stats here
    XGL_RESULT result = nextTable.GetMemoryHeapInfo(device, heapId, infoType, pDataSize, pData);
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglMapMemory(XGL_GPU_MEMORY mem, XGL_FLAGS flags, XGL_VOID** ppData)
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglPinSystemMemory(XGL_DEVICE device, const XGL_VOID* pSysMem, XGL_SIZE memSize, XGL_GPU_MEMORY* pMem)
{
    // TODO : Track this
    //  Verify that memory is actually pinnable
    XGL_RESULT result = nextTable.PinSystemMemory(device, pSysMem, memSize, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglRemapVirtualMemoryPages(XGL_DEVICE device, XGL_UINT rangeCount, const XGL_VIRTUAL_MEMORY_REMAP_RANGE* pRanges, XGL_UINT preWaitSemaphoreCount, const XGL_QUEUE_SEMAPHORE* pPreWaitSemaphores, XGL_UINT postSignalSemaphoreCount, const XGL_QUEUE_SEMAPHORE* pPostSignalSemaphores)
{
    // TODO : Track this
    XGL_RESULT result = nextTable.RemapVirtualMemoryPages(device, rangeCount, pRanges, preWaitSemaphoreCount, pPreWaitSemaphores, postSignalSemaphoreCount, pPostSignalSemaphores);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetMultiGpuCompatibility(XGL_PHYSICAL_GPU gpu0, XGL_PHYSICAL_GPU gpu1, XGL_GPU_COMPATIBILITY_INFO* pInfo)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu0;
    pCurObj = gpuw;
    pthread_once(&g_initOnce, initMemTracker);
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetObjectInfo(XGL_BASE_OBJECT object, XGL_OBJECT_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    // TODO : What to track here?
    //   Could potentially save returned mem requirements and validate values passed into BindObjectMemory for this object
    // From spec : The only objects that are guaranteed to have no external memory requirements are devices, queues, command buffers, shaders and memory objects.
    XGL_RESULT result = nextTable.GetObjectInfo(object, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindObjectMemory(XGL_OBJECT object, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset)
{
    XGL_RESULT result = nextTable.BindObjectMemory(object, mem, offset);
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWaitForFences(XGL_DEVICE device, XGL_UINT fenceCount, const XGL_FENCE* pFences, XGL_BOOL waitAll, XGL_UINT64 timeout)
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
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetQueryPoolResults(XGL_QUERY_POOL queryPool, XGL_UINT startQuery, XGL_UINT queryCount, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_RESULT result = nextTable.GetQueryPoolResults(queryPool, startQuery, queryCount, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFormatInfo(XGL_DEVICE device, XGL_FORMAT format, XGL_FORMAT_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_RESULT result = nextTable.GetFormatInfo(device, format, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImage(XGL_DEVICE device, const XGL_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage)
{
    XGL_RESULT result = nextTable.CreateImage(device, pCreateInfo, pImage);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetImageSubresourceInfo(XGL_IMAGE image, const XGL_IMAGE_SUBRESOURCE* pSubresource, XGL_SUBRESOURCE_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_RESULT result = nextTable.GetImageSubresourceInfo(image, pSubresource, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImageView(XGL_DEVICE device, const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo, XGL_IMAGE_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateImageView(device, pCreateInfo, pView);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateColorAttachmentView(XGL_DEVICE device, const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo, XGL_COLOR_ATTACHMENT_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateColorAttachmentView(device, pCreateInfo, pView);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDepthStencilView(XGL_DEVICE device, const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo, XGL_DEPTH_STENCIL_VIEW* pView)
{
    XGL_RESULT result = nextTable.CreateDepthStencilView(device, pCreateInfo, pView);
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
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateComputePipeline(XGL_DEVICE device, const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline)
{
    XGL_RESULT result = nextTable.CreateComputePipeline(device, pCreateInfo, pPipeline);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglStorePipeline(XGL_PIPELINE pipeline, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_RESULT result = nextTable.StorePipeline(pipeline, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglLoadPipeline(XGL_DEVICE device, XGL_SIZE dataSize, const XGL_VOID* pData, XGL_PIPELINE* pPipeline)
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
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorSet(XGL_DEVICE device, const XGL_DESCRIPTOR_SET_CREATE_INFO* pCreateInfo, XGL_DESCRIPTOR_SET* pDescriptorSet)
{
    XGL_RESULT result = nextTable.CreateDescriptorSet(device, pCreateInfo, pDescriptorSet);
    return result;
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglBeginDescriptorSetUpdate(XGL_DESCRIPTOR_SET descriptorSet)
{
    nextTable.BeginDescriptorSetUpdate(descriptorSet);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglEndDescriptorSetUpdate(XGL_DESCRIPTOR_SET descriptorSet)
{
    nextTable.EndDescriptorSetUpdate(descriptorSet);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglAttachSamplerDescriptors(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_SAMPLER* pSamplers)
{
    nextTable.AttachSamplerDescriptors(descriptorSet, startSlot, slotCount, pSamplers);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglAttachImageViewDescriptors(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_IMAGE_VIEW_ATTACH_INFO* pImageViews)
{
    nextTable.AttachImageViewDescriptors(descriptorSet, startSlot, slotCount, pImageViews);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglAttachMemoryViewDescriptors(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_MEMORY_VIEW_ATTACH_INFO* pMemViews)
{
    nextTable.AttachMemoryViewDescriptors(descriptorSet, startSlot, slotCount, pMemViews);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglAttachNestedDescriptors(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_DESCRIPTOR_SET_ATTACH_INFO* pNestedDescriptorSets)
{
    nextTable.AttachNestedDescriptors(descriptorSet, startSlot, slotCount, pNestedDescriptorSets);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglClearDescriptorSetSlots(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount)
{
    nextTable.ClearDescriptorSetSlots(descriptorSet, startSlot, slotCount);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateViewportState(XGL_DEVICE device, const XGL_VIEWPORT_STATE_CREATE_INFO* pCreateInfo, XGL_VIEWPORT_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateViewportState(device, pCreateInfo, pState);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateRasterState(XGL_DEVICE device, const XGL_RASTER_STATE_CREATE_INFO* pCreateInfo, XGL_RASTER_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateRasterState(device, pCreateInfo, pState);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateMsaaState(XGL_DEVICE device, const XGL_MSAA_STATE_CREATE_INFO* pCreateInfo, XGL_MSAA_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateMsaaState(device, pCreateInfo, pState);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateColorBlendState(XGL_DEVICE device, const XGL_COLOR_BLEND_STATE_CREATE_INFO* pCreateInfo, XGL_COLOR_BLEND_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateColorBlendState(device, pCreateInfo, pState);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDepthStencilState(XGL_DEVICE device, const XGL_DEPTH_STENCIL_STATE_CREATE_INFO* pCreateInfo, XGL_DEPTH_STENCIL_STATE_OBJECT* pState)
{
    XGL_RESULT result = nextTable.CreateDepthStencilState(device, pCreateInfo, pState);
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBeginCommandBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_FLAGS flags)
{
    // This implicitly resets the Cmd Buffer so clear memory references
    freeCBBindings(cmdBuffer);
    XGL_RESULT result = nextTable.BeginCommandBuffer(cmdBuffer, flags);
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
XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindPipeline(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE pipeline)
{
    nextTable.CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindPipelineDelta(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE_DELTA delta)
{
    nextTable.CmdBindPipelineDelta(cmdBuffer, pipelineBindPoint, delta);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindStateObject(XGL_CMD_BUFFER cmdBuffer, XGL_STATE_BIND_POINT stateBindPoint, XGL_STATE_OBJECT state)
{
    nextTable.CmdBindStateObject(cmdBuffer, stateBindPoint, state);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindDescriptorSet(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT index, XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT slotOffset)
{
    nextTable.CmdBindDescriptorSet(cmdBuffer, pipelineBindPoint, index, descriptorSet, slotOffset);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindDynamicMemoryView(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, const XGL_MEMORY_VIEW_ATTACH_INFO* pMemView)
{
    nextTable.CmdBindDynamicMemoryView(cmdBuffer, pipelineBindPoint, pMemView);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindIndexData(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset, XGL_INDEX_TYPE indexType)
{
    // Track this memory. What exactly is this call doing?
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdBindIndexData() call unable to update binding of mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdBindIndexData(cmdBuffer, mem, offset, indexType);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindAttachments(XGL_CMD_BUFFER cmdBuffer, XGL_UINT colorAttachmentCount, const XGL_COLOR_ATTACHMENT_BIND_INFO* pColorAttachments, const XGL_DEPTH_STENCIL_BIND_INFO* pDepthStencilAttachment)
{
    nextTable.CmdBindAttachments(cmdBuffer, colorAttachmentCount, pColorAttachments, pDepthStencilAttachment);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdPrepareMemoryRegions(XGL_CMD_BUFFER cmdBuffer, XGL_UINT transitionCount, const XGL_MEMORY_STATE_TRANSITION* pStateTransitions)
{
    // TODO : Track memory state transitions
    //   Flag incorrect transitions when states don't match
    XGL_GPU_MEMORY mem = pStateTransitions->mem;
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdPrepareMemoryRegions() call unable to update binding of mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdPrepareMemoryRegions(cmdBuffer, transitionCount, pStateTransitions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdPrepareImages(XGL_CMD_BUFFER cmdBuffer, XGL_UINT transitionCount, const XGL_IMAGE_STATE_TRANSITION* pStateTransitions)
{
    // TODO : This will have an image object that's bound to memory in the pStateTransitions struct
    //   Need to pull object based on image and then pull mem mapping from that object
    XGL_GPU_MEMORY mem = getMemBindingFromObject((XGL_OBJECT)pStateTransitions->image);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdPrepareImages() call unable to update binding of mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdPrepareImages(cmdBuffer, transitionCount, pStateTransitions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDraw(XGL_CMD_BUFFER cmdBuffer, XGL_UINT firstVertex, XGL_UINT vertexCount, XGL_UINT firstInstance, XGL_UINT instanceCount)
{
    nextTable.CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDrawIndexed(XGL_CMD_BUFFER cmdBuffer, XGL_UINT firstIndex, XGL_UINT indexCount, XGL_INT vertexOffset, XGL_UINT firstInstance, XGL_UINT instanceCount)
{
    nextTable.CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDrawIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset, XGL_UINT32 count, XGL_UINT32 stride)
{
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdDrawIndirect() call unable to update binding of mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdDrawIndirect(cmdBuffer, mem, offset, count, stride);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDrawIndexedIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset, XGL_UINT32 count, XGL_UINT32 stride)
{
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdDrawIndexedIndirect() call unable to update binding of mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdDrawIndexedIndirect(cmdBuffer, mem, offset, count, stride);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDispatch(XGL_CMD_BUFFER cmdBuffer, XGL_UINT x, XGL_UINT y, XGL_UINT z)
{
    nextTable.CmdDispatch(cmdBuffer, x, y, z);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDispatchIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset)
{
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdDispatchIndirect() call unable to update binding of mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdDispatchIndirect(cmdBuffer, mem, offset);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCopyMemory(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY srcMem, XGL_GPU_MEMORY destMem, XGL_UINT regionCount, const XGL_MEMORY_COPY* pRegions)
{
    if (XGL_FALSE == updateCBBinding(cmdBuffer, srcMem)) {
        char str[1024];
        sprintf(str, "In xglCmdCopyMemory() call unable to update binding of srcMem %p to cmdBuffer %p", srcMem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    if (XGL_FALSE == updateCBBinding(cmdBuffer, destMem)) {
        char str[1024];
        sprintf(str, "In xglCmdCopyMemory() call unable to update binding of destMem %p to cmdBuffer %p", destMem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdCopyMemory(cmdBuffer, srcMem, destMem, regionCount, pRegions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCopyImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, XGL_UINT regionCount, const XGL_IMAGE_COPY* pRegions)
{
    // TODO : Each image will have mem mapping so track them
    nextTable.CmdCopyImage(cmdBuffer, srcImage, destImage, regionCount, pRegions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCopyMemoryToImage(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY srcMem, XGL_IMAGE destImage, XGL_UINT regionCount, const XGL_MEMORY_IMAGE_COPY* pRegions)
{
    // TODO : Track this
    XGL_GPU_MEMORY mem = getMemBindingFromObject(destImage);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdCopyMemoryToImage() call unable to update binding of destImage mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    if (XGL_FALSE == updateCBBinding(cmdBuffer, srcMem)) {
        char str[1024];
        sprintf(str, "In xglCmdCopyMemoryToImage() call unable to update binding of srcMem %p to cmdBuffer %p", srcMem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdCopyMemoryToImage(cmdBuffer, srcMem, destImage, regionCount, pRegions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCopyImageToMemory(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_GPU_MEMORY destMem, XGL_UINT regionCount, const XGL_MEMORY_IMAGE_COPY* pRegions)
{
    // TODO : Track this
    XGL_GPU_MEMORY mem = getMemBindingFromObject(srcImage);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdCopyImageToMemory() call unable to update binding of srcImage mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    if (XGL_FALSE == updateCBBinding(cmdBuffer, destMem)) {
        char str[1024];
        sprintf(str, "In xglCmdCopyImageToMemory() call unable to update binding of destMem %p to cmdBuffer %p", destMem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdCopyImageToMemory(cmdBuffer, srcImage, destMem, regionCount, pRegions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCloneImageData(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE_STATE srcImageState, XGL_IMAGE destImage, XGL_IMAGE_STATE destImageState)
{
    // TODO : Each image will have mem mapping so track them
    XGL_GPU_MEMORY mem = getMemBindingFromObject(srcImage);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdCloneImageData() call unable to update binding of srcImage mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    mem = getMemBindingFromObject(destImage);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdCloneImageData() call unable to update binding of destImage mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdCloneImageData(cmdBuffer, srcImage, srcImageState, destImage, destImageState);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdUpdateMemory(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE dataSize, const XGL_UINT32* pData)
{
    if (XGL_FALSE == updateCBBinding(cmdBuffer, destMem)) {
        char str[1024];
        sprintf(str, "In xglCmdUpdateMemory() call unable to update binding of destMem %p to cmdBuffer %p", destMem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdUpdateMemory(cmdBuffer, destMem, destOffset, dataSize, pData);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdFillMemory(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE fillSize, XGL_UINT32 data)
{
    if (XGL_FALSE == updateCBBinding(cmdBuffer, destMem)) {
        char str[1024];
        sprintf(str, "In xglCmdFillMemory() call unable to update binding of destMem %p to cmdBuffer %p", destMem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdFillMemory(cmdBuffer, destMem, destOffset, fillSize, data);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdClearColorImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const XGL_FLOAT color[4], XGL_UINT rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(image);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdClearColorImage() call unable to update binding of image mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdClearColorImage(cmdBuffer, image, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdClearColorImageRaw(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const XGL_UINT32 color[4], XGL_UINT rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(image);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdClearColorImageRaw() call unable to update binding of image mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdClearColorImageRaw(cmdBuffer, image, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdClearDepthStencil(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, XGL_FLOAT depth, XGL_UINT32 stencil, XGL_UINT rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(image);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdClearDepthStencil() call unable to update binding of image mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdClearDepthStencil(cmdBuffer, image, depth, stencil, rangeCount, pRanges);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdResolveImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, XGL_UINT rectCount, const XGL_IMAGE_RESOLVE* pRects)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(srcImage);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdResolveImage() call unable to update binding of srcImage mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    mem = getMemBindingFromObject(destImage);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdResolveImage() call unable to update binding of destImage mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdResolveImage(cmdBuffer, srcImage, destImage, rectCount, pRects);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdSetEvent(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event)
{
    nextTable.CmdSetEvent(cmdBuffer, event);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdResetEvent(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event)
{
    nextTable.CmdResetEvent(cmdBuffer, event);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdMemoryAtomic(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset, XGL_UINT64 srcData, XGL_ATOMIC_OP atomicOp)
{
    if (XGL_FALSE == updateCBBinding(cmdBuffer, destMem)) {
        char str[1024];
        sprintf(str, "In xglCmdMemoryAtomic() call unable to update binding of destMem %p to cmdBuffer %p", destMem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdMemoryAtomic(cmdBuffer, destMem, destOffset, srcData, atomicOp);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBeginQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, XGL_UINT slot, XGL_FLAGS flags)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(queryPool);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdBeginQuery() call unable to update binding of queryPool mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdEndQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, XGL_UINT slot)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(queryPool);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdEndQuery() call unable to update binding of queryPool mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdEndQuery(cmdBuffer, queryPool, slot);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdResetQueryPool(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, XGL_UINT startQuery, XGL_UINT queryCount)
{
    XGL_GPU_MEMORY mem = getMemBindingFromObject(queryPool);
    if (XGL_FALSE == updateCBBinding(cmdBuffer, mem)) {
        char str[1024];
        sprintf(str, "In xglCmdResetQueryPool() call unable to update binding of queryPool mem %p to cmdBuffer %p", mem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdWriteTimestamp(XGL_CMD_BUFFER cmdBuffer, XGL_TIMESTAMP_TYPE timestampType, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset)
{
    if (XGL_FALSE == updateCBBinding(cmdBuffer, destMem)) {
        char str[1024];
        sprintf(str, "In xglCmdWriteTimestamp() call unable to update binding of destMem %p to cmdBuffer %p", destMem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdWriteTimestamp(cmdBuffer, timestampType, destMem, destOffset);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdInitAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT startCounter, XGL_UINT counterCount, const XGL_UINT32* pData)
{
    nextTable.CmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdLoadAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT startCounter, XGL_UINT counterCount, XGL_GPU_MEMORY srcMem, XGL_GPU_SIZE srcOffset)
{
    if (XGL_FALSE == updateCBBinding(cmdBuffer, srcMem)) {
        char str[1024];
        sprintf(str, "In xglCmdLoadAtomicCounters() call unable to update binding of srcMem %p to cmdBuffer %p", srcMem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcMem, srcOffset);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdSaveAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT startCounter, XGL_UINT counterCount, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset)
{
    if (XGL_FALSE == updateCBBinding(cmdBuffer, destMem)) {
        char str[1024];
        sprintf(str, "In xglCmdSaveAtomicCounters() call unable to update binding of destMem %p to cmdBuffer %p", destMem, cmdBuffer);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
    }
    nextTable.CmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destMem, destOffset);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetValidationLevel(XGL_DEVICE device, XGL_VALIDATION_LEVEL validationLevel)
{
    XGL_RESULT result = nextTable.DbgSetValidationLevel(device, validationLevel);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, XGL_VOID* pUserData)
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

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetMessageFilter(XGL_DEVICE device, XGL_INT msgCode, XGL_DBG_MSG_FILTER filter)
{
    XGL_RESULT result = nextTable.DbgSetMessageFilter(device, msgCode, filter);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetObjectTag(XGL_BASE_OBJECT object, XGL_SIZE tagSize, const XGL_VOID* pTag)
{
    XGL_RESULT result = nextTable.DbgSetObjectTag(object, tagSize, pTag);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetGlobalOption(XGL_DBG_GLOBAL_OPTION dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData)
{
    XGL_RESULT result = nextTable.DbgSetGlobalOption(dbgOption, dataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetDeviceOption(XGL_DEVICE device, XGL_DBG_DEVICE_OPTION dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData)
{
    XGL_RESULT result = nextTable.DbgSetDeviceOption(device, dbgOption, dataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDbgMarkerBegin(XGL_CMD_BUFFER cmdBuffer, const XGL_CHAR* pMarker)
{
    nextTable.CmdDbgMarkerBegin(cmdBuffer, pMarker);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDbgMarkerEnd(XGL_CMD_BUFFER cmdBuffer)
{
    nextTable.CmdDbgMarkerEnd(cmdBuffer);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11AssociateConnection(XGL_PHYSICAL_GPU gpu, const XGL_WSI_X11_CONNECTION_INFO* pConnectionInfo)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    pthread_once(&g_initOnce, initMemTracker);
    XGL_RESULT result = nextTable.WsiX11AssociateConnection((XGL_PHYSICAL_GPU)gpuw->nextObject, pConnectionInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11GetMSC(XGL_DEVICE device, xcb_window_t window, xcb_randr_crtc_t crtc, XGL_UINT64* pMsc)
{
    XGL_RESULT result = nextTable.WsiX11GetMSC(device, window, crtc, pMsc);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11CreatePresentableImage(XGL_DEVICE device, const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem)
{
    XGL_RESULT result = nextTable.WsiX11CreatePresentableImage(device, pCreateInfo, pImage, pMem);
    // First insert the new Mem Object and then bind it to created image
    insertGlobalMemObj(*pMem, NULL);
    if (XGL_FALSE == updateObjectBinding(*pImage, *pMem)) {
        char str[1024];
        sprintf(str, "In xglWsiX11CreatePresentableImage(), unable to set image %p binding to mem obj %p", (void*)*pImage, (void*)*pMem);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, *pImage, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM", str);
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

XGL_LAYER_EXPORT XGL_VOID* XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* funcName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    if (gpu == NULL)
        return NULL;
    pCurObj = gpuw;
    pthread_once(&g_initOnce, initMemTracker);

    if (!strncmp("xglGetProcAddr", funcName, sizeof("xglGetProcAddr")))
        return xglGetProcAddr;
    else if (!strncmp("xglInitAndEnumerateGpus", funcName, sizeof("xglInitAndEnumerateGpus")))
        return nextTable.InitAndEnumerateGpus;
    else if (!strncmp("xglGetGpuInfo", funcName, sizeof("xglGetGpuInfo")))
        return xglGetGpuInfo;
    else if (!strncmp("xglCreateDevice", funcName, sizeof("xglCreateDevice")))
        return xglCreateDevice;
    else if (!strncmp("xglDestroyDevice", funcName, sizeof("xglDestroyDevice")))
        return xglDestroyDevice;
    else if (!strncmp("xglGetExtensionSupport", funcName, sizeof("xglGetExtensionSupport")))
        return xglGetExtensionSupport;
    else if (!strncmp("xglEnumerateLayers", funcName, sizeof("xglEnumerateLayers")))
        return xglEnumerateLayers;
    else if (!strncmp("xglGetDeviceQueue", funcName, sizeof("xglGetDeviceQueue")))
        return xglGetDeviceQueue;
    else if (!strncmp("xglQueueSubmit", funcName, sizeof("xglQueueSubmit")))
        return xglQueueSubmit;
    else if (!strncmp("xglQueueSetGlobalMemReferences", funcName, sizeof("xglQueueSetGlobalMemReferences")))
        return xglQueueSetGlobalMemReferences;
    else if (!strncmp("xglQueueWaitIdle", funcName, sizeof("xglQueueWaitIdle")))
        return xglQueueWaitIdle;
    else if (!strncmp("xglDeviceWaitIdle", funcName, sizeof("xglDeviceWaitIdle")))
        return xglDeviceWaitIdle;
    else if (!strncmp("xglGetMemoryHeapCount", funcName, sizeof("xglGetMemoryHeapCount")))
        return xglGetMemoryHeapCount;
    else if (!strncmp("xglGetMemoryHeapInfo", funcName, sizeof("xglGetMemoryHeapInfo")))
        return xglGetMemoryHeapInfo;
    else if (!strncmp("xglAllocMemory", funcName, sizeof("xglAllocMemory")))
        return xglAllocMemory;
    else if (!strncmp("xglFreeMemory", funcName, sizeof("xglFreeMemory")))
        return xglFreeMemory;
    else if (!strncmp("xglSetMemoryPriority", funcName, sizeof("xglSetMemoryPriority")))
        return xglSetMemoryPriority;
    else if (!strncmp("xglMapMemory", funcName, sizeof("xglMapMemory")))
        return xglMapMemory;
    else if (!strncmp("xglUnmapMemory", funcName, sizeof("xglUnmapMemory")))
        return xglUnmapMemory;
    else if (!strncmp("xglPinSystemMemory", funcName, sizeof("xglPinSystemMemory")))
        return xglPinSystemMemory;
    else if (!strncmp("xglRemapVirtualMemoryPages", funcName, sizeof("xglRemapVirtualMemoryPages")))
        return xglRemapVirtualMemoryPages;
    else if (!strncmp("xglGetMultiGpuCompatibility", funcName, sizeof("xglGetMultiGpuCompatibility")))
        return xglGetMultiGpuCompatibility;
    else if (!strncmp("xglOpenSharedMemory", funcName, sizeof("xglOpenSharedMemory")))
        return xglOpenSharedMemory;
    else if (!strncmp("xglOpenSharedQueueSemaphore", funcName, sizeof("xglOpenSharedQueueSemaphore")))
        return xglOpenSharedQueueSemaphore;
    else if (!strncmp("xglOpenPeerMemory", funcName, sizeof("xglOpenPeerMemory")))
        return xglOpenPeerMemory;
    else if (!strncmp("xglOpenPeerImage", funcName, sizeof("xglOpenPeerImage")))
        return xglOpenPeerImage;
    else if (!strncmp("xglDestroyObject", funcName, sizeof("xglDestroyObject")))
        return xglDestroyObject;
    else if (!strncmp("xglGetObjectInfo", funcName, sizeof("xglGetObjectInfo")))
        return xglGetObjectInfo;
    else if (!strncmp("xglBindObjectMemory", funcName, sizeof("xglBindObjectMemory")))
        return xglBindObjectMemory;
    else if (!strncmp("xglCreateFence", funcName, sizeof("xglCreateFence")))
        return xglCreateFence;
    else if (!strncmp("xglGetFenceStatus", funcName, sizeof("xglGetFenceStatus")))
        return xglGetFenceStatus;
    else if (!strncmp("xglWaitForFences", funcName, sizeof("xglWaitForFences")))
        return xglWaitForFences;
    else if (!strncmp("xglCreateQueueSemaphore", funcName, sizeof("xglCreateQueueSemaphore")))
        return xglCreateQueueSemaphore;
    else if (!strncmp("xglSignalQueueSemaphore", funcName, sizeof("xglSignalQueueSemaphore")))
        return xglSignalQueueSemaphore;
    else if (!strncmp("xglWaitQueueSemaphore", funcName, sizeof("xglWaitQueueSemaphore")))
        return xglWaitQueueSemaphore;
    else if (!strncmp("xglCreateEvent", funcName, sizeof("xglCreateEvent")))
        return xglCreateEvent;
    else if (!strncmp("xglGetEventStatus", funcName, sizeof("xglGetEventStatus")))
        return xglGetEventStatus;
    else if (!strncmp("xglSetEvent", funcName, sizeof("xglSetEvent")))
        return xglSetEvent;
    else if (!strncmp("xglResetEvent", funcName, sizeof("xglResetEvent")))
        return xglResetEvent;
    else if (!strncmp("xglCreateQueryPool", funcName, sizeof("xglCreateQueryPool")))
        return xglCreateQueryPool;
    else if (!strncmp("xglGetQueryPoolResults", funcName, sizeof("xglGetQueryPoolResults")))
        return xglGetQueryPoolResults;
    else if (!strncmp("xglGetFormatInfo", funcName, sizeof("xglGetFormatInfo")))
        return xglGetFormatInfo;
    else if (!strncmp("xglCreateImage", funcName, sizeof("xglCreateImage")))
        return xglCreateImage;
    else if (!strncmp("xglGetImageSubresourceInfo", funcName, sizeof("xglGetImageSubresourceInfo")))
        return xglGetImageSubresourceInfo;
    else if (!strncmp("xglCreateImageView", funcName, sizeof("xglCreateImageView")))
        return xglCreateImageView;
    else if (!strncmp("xglCreateColorAttachmentView", funcName, sizeof("xglCreateColorAttachmentView")))
        return xglCreateColorAttachmentView;
    else if (!strncmp("xglCreateDepthStencilView", funcName, sizeof("xglCreateDepthStencilView")))
        return xglCreateDepthStencilView;
    else if (!strncmp("xglCreateShader", funcName, sizeof("xglCreateShader")))
        return xglCreateShader;
    else if (!strncmp("xglCreateGraphicsPipeline", funcName, sizeof("xglCreateGraphicsPipeline")))
        return xglCreateGraphicsPipeline;
    else if (!strncmp("xglCreateComputePipeline", funcName, sizeof("xglCreateComputePipeline")))
        return xglCreateComputePipeline;
    else if (!strncmp("xglStorePipeline", funcName, sizeof("xglStorePipeline")))
        return xglStorePipeline;
    else if (!strncmp("xglLoadPipeline", funcName, sizeof("xglLoadPipeline")))
        return xglLoadPipeline;
    else if (!strncmp("xglCreatePipelineDelta", funcName, sizeof("xglCreatePipelineDelta")))
        return xglCreatePipelineDelta;
    else if (!strncmp("xglCreateSampler", funcName, sizeof("xglCreateSampler")))
        return xglCreateSampler;
    else if (!strncmp("xglCreateDescriptorSet", funcName, sizeof("xglCreateDescriptorSet")))
        return xglCreateDescriptorSet;
    else if (!strncmp("xglBeginDescriptorSetUpdate", funcName, sizeof("xglBeginDescriptorSetUpdate")))
        return xglBeginDescriptorSetUpdate;
    else if (!strncmp("xglEndDescriptorSetUpdate", funcName, sizeof("xglEndDescriptorSetUpdate")))
        return xglEndDescriptorSetUpdate;
    else if (!strncmp("xglAttachSamplerDescriptors", funcName, sizeof("xglAttachSamplerDescriptors")))
        return xglAttachSamplerDescriptors;
    else if (!strncmp("xglAttachImageViewDescriptors", funcName, sizeof("xglAttachImageViewDescriptors")))
        return xglAttachImageViewDescriptors;
    else if (!strncmp("xglAttachMemoryViewDescriptors", funcName, sizeof("xglAttachMemoryViewDescriptors")))
        return xglAttachMemoryViewDescriptors;
    else if (!strncmp("xglAttachNestedDescriptors", funcName, sizeof("xglAttachNestedDescriptors")))
        return xglAttachNestedDescriptors;
    else if (!strncmp("xglClearDescriptorSetSlots", funcName, sizeof("xglClearDescriptorSetSlots")))
        return xglClearDescriptorSetSlots;
    else if (!strncmp("xglCreateViewportState", funcName, sizeof("xglCreateViewportState")))
        return xglCreateViewportState;
    else if (!strncmp("xglCreateRasterState", funcName, sizeof("xglCreateRasterState")))
        return xglCreateRasterState;
    else if (!strncmp("xglCreateMsaaState", funcName, sizeof("xglCreateMsaaState")))
        return xglCreateMsaaState;
    else if (!strncmp("xglCreateColorBlendState", funcName, sizeof("xglCreateColorBlendState")))
        return xglCreateColorBlendState;
    else if (!strncmp("xglCreateDepthStencilState", funcName, sizeof("xglCreateDepthStencilState")))
        return xglCreateDepthStencilState;
    else if (!strncmp("xglCreateCommandBuffer", funcName, sizeof("xglCreateCommandBuffer")))
        return xglCreateCommandBuffer;
    else if (!strncmp("xglBeginCommandBuffer", funcName, sizeof("xglBeginCommandBuffer")))
        return xglBeginCommandBuffer;
    else if (!strncmp("xglEndCommandBuffer", funcName, sizeof("xglEndCommandBuffer")))
        return xglEndCommandBuffer;
    else if (!strncmp("xglResetCommandBuffer", funcName, sizeof("xglResetCommandBuffer")))
        return xglResetCommandBuffer;
    else if (!strncmp("xglCmdBindPipeline", funcName, sizeof("xglCmdBindPipeline")))
        return xglCmdBindPipeline;
    else if (!strncmp("xglCmdBindPipelineDelta", funcName, sizeof("xglCmdBindPipelineDelta")))
        return xglCmdBindPipelineDelta;
    else if (!strncmp("xglCmdBindStateObject", funcName, sizeof("xglCmdBindStateObject")))
        return xglCmdBindStateObject;
    else if (!strncmp("xglCmdBindDescriptorSet", funcName, sizeof("xglCmdBindDescriptorSet")))
        return xglCmdBindDescriptorSet;
    else if (!strncmp("xglCmdBindDynamicMemoryView", funcName, sizeof("xglCmdBindDynamicMemoryView")))
        return xglCmdBindDynamicMemoryView;
    else if (!strncmp("xglCmdBindIndexData", funcName, sizeof("xglCmdBindIndexData")))
        return xglCmdBindIndexData;
    else if (!strncmp("xglCmdBindAttachments", funcName, sizeof("xglCmdBindAttachments")))
        return xglCmdBindAttachments;
    else if (!strncmp("xglCmdPrepareMemoryRegions", funcName, sizeof("xglCmdPrepareMemoryRegions")))
        return xglCmdPrepareMemoryRegions;
    else if (!strncmp("xglCmdPrepareImages", funcName, sizeof("xglCmdPrepareImages")))
        return xglCmdPrepareImages;
    else if (!strncmp("xglCmdDraw", funcName, sizeof("xglCmdDraw")))
        return xglCmdDraw;
    else if (!strncmp("xglCmdDrawIndexed", funcName, sizeof("xglCmdDrawIndexed")))
        return xglCmdDrawIndexed;
    else if (!strncmp("xglCmdDrawIndirect", funcName, sizeof("xglCmdDrawIndirect")))
        return xglCmdDrawIndirect;
    else if (!strncmp("xglCmdDrawIndexedIndirect", funcName, sizeof("xglCmdDrawIndexedIndirect")))
        return xglCmdDrawIndexedIndirect;
    else if (!strncmp("xglCmdDispatch", funcName, sizeof("xglCmdDispatch")))
        return xglCmdDispatch;
    else if (!strncmp("xglCmdDispatchIndirect", funcName, sizeof("xglCmdDispatchIndirect")))
        return xglCmdDispatchIndirect;
    else if (!strncmp("xglCmdCopyMemory", funcName, sizeof("xglCmdCopyMemory")))
        return xglCmdCopyMemory;
    else if (!strncmp("xglCmdCopyImage", funcName, sizeof("xglCmdCopyImage")))
        return xglCmdCopyImage;
    else if (!strncmp("xglCmdCopyMemoryToImage", funcName, sizeof("xglCmdCopyMemoryToImage")))
        return xglCmdCopyMemoryToImage;
    else if (!strncmp("xglCmdCopyImageToMemory", funcName, sizeof("xglCmdCopyImageToMemory")))
        return xglCmdCopyImageToMemory;
    else if (!strncmp("xglCmdCloneImageData", funcName, sizeof("xglCmdCloneImageData")))
        return xglCmdCloneImageData;
    else if (!strncmp("xglCmdUpdateMemory", funcName, sizeof("xglCmdUpdateMemory")))
        return xglCmdUpdateMemory;
    else if (!strncmp("xglCmdFillMemory", funcName, sizeof("xglCmdFillMemory")))
        return xglCmdFillMemory;
    else if (!strncmp("xglCmdClearColorImage", funcName, sizeof("xglCmdClearColorImage")))
        return xglCmdClearColorImage;
    else if (!strncmp("xglCmdClearColorImageRaw", funcName, sizeof("xglCmdClearColorImageRaw")))
        return xglCmdClearColorImageRaw;
    else if (!strncmp("xglCmdClearDepthStencil", funcName, sizeof("xglCmdClearDepthStencil")))
        return xglCmdClearDepthStencil;
    else if (!strncmp("xglCmdResolveImage", funcName, sizeof("xglCmdResolveImage")))
        return xglCmdResolveImage;
    else if (!strncmp("xglCmdSetEvent", funcName, sizeof("xglCmdSetEvent")))
        return xglCmdSetEvent;
    else if (!strncmp("xglCmdResetEvent", funcName, sizeof("xglCmdResetEvent")))
        return xglCmdResetEvent;
    else if (!strncmp("xglCmdMemoryAtomic", funcName, sizeof("xglCmdMemoryAtomic")))
        return xglCmdMemoryAtomic;
    else if (!strncmp("xglCmdBeginQuery", funcName, sizeof("xglCmdBeginQuery")))
        return xglCmdBeginQuery;
    else if (!strncmp("xglCmdEndQuery", funcName, sizeof("xglCmdEndQuery")))
        return xglCmdEndQuery;
    else if (!strncmp("xglCmdResetQueryPool", funcName, sizeof("xglCmdResetQueryPool")))
        return xglCmdResetQueryPool;
    else if (!strncmp("xglCmdWriteTimestamp", funcName, sizeof("xglCmdWriteTimestamp")))
        return xglCmdWriteTimestamp;
    else if (!strncmp("xglCmdInitAtomicCounters", funcName, sizeof("xglCmdInitAtomicCounters")))
        return xglCmdInitAtomicCounters;
    else if (!strncmp("xglCmdLoadAtomicCounters", funcName, sizeof("xglCmdLoadAtomicCounters")))
        return xglCmdLoadAtomicCounters;
    else if (!strncmp("xglCmdSaveAtomicCounters", funcName, sizeof("xglCmdSaveAtomicCounters")))
        return xglCmdSaveAtomicCounters;
    else if (!strncmp("xglDbgSetValidationLevel", funcName, sizeof("xglDbgSetValidationLevel")))
        return xglDbgSetValidationLevel;
    else if (!strncmp("xglDbgRegisterMsgCallback", funcName, sizeof("xglDbgRegisterMsgCallback")))
        return xglDbgRegisterMsgCallback;
    else if (!strncmp("xglDbgUnregisterMsgCallback", funcName, sizeof("xglDbgUnregisterMsgCallback")))
        return xglDbgUnregisterMsgCallback;
    else if (!strncmp("xglDbgSetMessageFilter", funcName, sizeof("xglDbgSetMessageFilter")))
        return xglDbgSetMessageFilter;
    else if (!strncmp("xglDbgSetObjectTag", funcName, sizeof("xglDbgSetObjectTag")))
        return xglDbgSetObjectTag;
    else if (!strncmp("xglDbgSetGlobalOption", funcName, sizeof("xglDbgSetGlobalOption")))
        return xglDbgSetGlobalOption;
    else if (!strncmp("xglDbgSetDeviceOption", funcName, sizeof("xglDbgSetDeviceOption")))
        return xglDbgSetDeviceOption;
    else if (!strncmp("xglCmdDbgMarkerBegin", funcName, sizeof("xglCmdDbgMarkerBegin")))
        return xglCmdDbgMarkerBegin;
    else if (!strncmp("xglCmdDbgMarkerEnd", funcName, sizeof("xglCmdDbgMarkerEnd")))
        return xglCmdDbgMarkerEnd;
    else if (!strncmp("xglWsiX11AssociateConnection", funcName, sizeof("xglWsiX11AssociateConnection")))
        return xglWsiX11AssociateConnection;
    else if (!strncmp("xglWsiX11GetMSC", funcName, sizeof("xglWsiX11GetMSC")))
        return xglWsiX11GetMSC;
    else if (!strncmp("xglWsiX11CreatePresentableImage", funcName, sizeof("xglWsiX11CreatePresentableImage")))
        return xglWsiX11CreatePresentableImage;
    else if (!strncmp("xglWsiX11QueuePresent", funcName, sizeof("xglWsiX11QueuePresent")))
        return xglWsiX11QueuePresent;
    else {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA(gpuw->nextObject, funcName);
    }
}
