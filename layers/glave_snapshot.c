/*
 * GLAVE & vulkan
 *
 * Copyright (C) 2015 LunarG, Inc. and Valve Corporation
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
#include "loader_platform.h"
#include "glave_snapshot.h"

#define LAYER_NAME_STR "GlaveSnapshot"
#define LAYER_ABBREV_STR "GLVSnap"

static XGL_LAYER_DISPATCH_TABLE nextTable;
static XGL_BASE_LAYER_OBJECT *pCurObj;
// The following is #included again to catch certain OS-specific functions being used:
#include "loader_platform.h"
#include "layers_config.h"
#include "layers_msg.h"
static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);
static long long unsigned int object_track_index = 0;
static int objLockInitialized = 0;
static loader_platform_thread_mutex objLock;

// We maintain a "Global" list which links every object and a
//  per-Object list which just links objects of a given type
// The object node has both pointers so the actual nodes are shared between the two lists
typedef struct _objNode {
    GLVSNAPSHOT_NODE   obj;
    struct _objNode *pNextObj;
    struct _objNode *pNextGlobal;
} objNode;
static objNode *pObjectHead[XGL_NUM_OBJECT_TYPE] = {0};
static objNode *pGlobalHead = NULL;
static uint64_t numObjs[XGL_NUM_OBJECT_TYPE] = {0};
static uint64_t numTotalObjs = 0;
static uint32_t maxMemRefsPerSubmission = 0;

// Debug function to print global list and each individual object list
static void ll_print_lists()
{
    objNode* pTrav = pGlobalHead;
    printf("=====GLOBAL OBJECT LIST (%lu total objs):\n", numTotalObjs);
    while (pTrav) {
        printf("   ObjNode (%p) w/ %s obj %p has pNextGlobal %p\n", (void*)pTrav, string_XGL_OBJECT_TYPE(pTrav->obj.objType), pTrav->obj.pObj, (void*)pTrav->pNextGlobal);
        pTrav = pTrav->pNextGlobal;
    }
    for (uint32_t i = 0; i < XGL_NUM_OBJECT_TYPE; i++) {
        pTrav = pObjectHead[i];
        if (pTrav) {
            printf("=====%s OBJECT LIST (%lu objs):\n", string_XGL_OBJECT_TYPE(pTrav->obj.objType), numObjs[i]);
            while (pTrav) {
                printf("   ObjNode (%p) w/ %s obj %p has pNextObj %p\n", (void*)pTrav, string_XGL_OBJECT_TYPE(pTrav->obj.objType), pTrav->obj.pObj, (void*)pTrav->pNextObj);
                pTrav = pTrav->pNextObj;
            }
        }
    }
}

static void ll_insert_obj(void* pObj, XGL_OBJECT_TYPE objType) {
    char str[1024];
    sprintf(str, "OBJ[%llu] : CREATE %s object %p", object_track_index++, string_XGL_OBJECT_TYPE(objType), (void*)pObj);
    layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pObj, 0, GLVSNAPSHOT_NONE, LAYER_ABBREV_STR, str);
    objNode* pNewObjNode = (objNode*)malloc(sizeof(objNode));
    pNewObjNode->obj.pObj = pObj;
    pNewObjNode->obj.objType = objType;
    pNewObjNode->obj.status  = OBJSTATUS_NONE;
    pNewObjNode->obj.numUses = 0;
    // insert at front of global list
    pNewObjNode->pNextGlobal = pGlobalHead;
    pGlobalHead = pNewObjNode;
    // insert at front of object list
    pNewObjNode->pNextObj = pObjectHead[objType];
    pObjectHead[objType] = pNewObjNode;
    // increment obj counts
    numObjs[objType]++;
    numTotalObjs++;
    //sprintf(str, "OBJ_STAT : %lu total objs & %lu %s objs.", numTotalObjs, numObjs[objType], string_XGL_OBJECT_TYPE(objType));
    if (0) ll_print_lists();
}

// Traverse global list and return type for given object
static XGL_OBJECT_TYPE ll_get_obj_type(XGL_OBJECT object) {
    objNode *pTrav = pGlobalHead;
    while (pTrav) {
        if (pTrav->obj.pObj == object)
            return pTrav->obj.objType;
        pTrav = pTrav->pNextGlobal;
    }
    char str[1024];
    sprintf(str, "Attempting look-up on obj %p but it is NOT in the global list!", (void*)object);
    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, object, 0, GLVSNAPSHOT_MISSING_OBJECT, LAYER_ABBREV_STR, str);
    return XGL_OBJECT_TYPE_UNKNOWN;
}

#if 0
static uint64_t ll_get_obj_uses(void* pObj, XGL_OBJECT_TYPE objType) {
    objNode *pTrav = pObjectHead[objType];
    while (pTrav) {
        if (pTrav->obj.pObj == pObj) {
            return pTrav->obj.numUses;
        }
        pTrav = pTrav->pNextObj;
    }
    return 0;
}
#endif

static void ll_increment_use_count(void* pObj, XGL_OBJECT_TYPE objType) {
    objNode *pTrav = pObjectHead[objType];
    while (pTrav) {
        if (pTrav->obj.pObj == pObj) {
            pTrav->obj.numUses++;
            char str[1024];
            sprintf(str, "OBJ[%llu] : USING %s object %p (%lu total uses)", object_track_index++, string_XGL_OBJECT_TYPE(objType), (void*)pObj, pTrav->obj.numUses);
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pObj, 0, GLVSNAPSHOT_NONE, LAYER_ABBREV_STR, str);
            return;
        }
        pTrav = pTrav->pNextObj;
    }
    // If we do not find obj, insert it and then increment count
    char str[1024];
    sprintf(str, "Unable to increment count for obj %p, will add to list as %s type and increment count", pObj, string_XGL_OBJECT_TYPE(objType));
    layerCbMsg(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, pObj, 0, GLVSNAPSHOT_UNKNOWN_OBJECT, LAYER_ABBREV_STR, str);

    ll_insert_obj(pObj, objType);
    ll_increment_use_count(pObj, objType);
}

// We usually do not know Obj type when we destroy it so have to fetch
//  Type from global list w/ ll_destroy_obj()
//   and then do the full removal from both lists w/ ll_remove_obj_type()
static void ll_remove_obj_type(void* pObj, XGL_OBJECT_TYPE objType) {
    objNode *pTrav = pObjectHead[objType];
    objNode *pPrev = pObjectHead[objType];
    while (pTrav) {
        if (pTrav->obj.pObj == pObj) {
            pPrev->pNextObj = pTrav->pNextObj;
            // update HEAD of Obj list as needed
            if (pObjectHead[objType] == pTrav)
                pObjectHead[objType] = pTrav->pNextObj;
            assert(numObjs[objType] > 0);
            numObjs[objType]--;
            char str[1024];
            sprintf(str, "OBJ[%llu] : DESTROY %s object %p", object_track_index++, string_XGL_OBJECT_TYPE(objType), (void*)pObj);
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pObj, 0, GLVSNAPSHOT_NONE, LAYER_ABBREV_STR, str);
            return;
        }
        pPrev = pTrav;
        pTrav = pTrav->pNextObj;
    }
    char str[1024];
    sprintf(str, "OBJ INTERNAL ERROR : Obj %p was in global list but not in %s list", pObj, string_XGL_OBJECT_TYPE(objType));
    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pObj, 0, GLVSNAPSHOT_INTERNAL_ERROR, LAYER_ABBREV_STR, str);
}

// Parse global list to find obj type, then remove obj from obj type list, finally
//   remove obj from global list
static void ll_destroy_obj(void* pObj) {
    objNode *pTrav = pGlobalHead;
    objNode *pPrev = pGlobalHead;
    while (pTrav) {
        if (pTrav->obj.pObj == pObj) {
            ll_remove_obj_type(pObj, pTrav->obj.objType);
            pPrev->pNextGlobal = pTrav->pNextGlobal;
            // update HEAD of global list if needed
            if (pGlobalHead == pTrav)
                pGlobalHead = pTrav->pNextGlobal;
            assert(numTotalObjs > 0);
            numTotalObjs--;
            char str[1024];
            sprintf(str, "OBJ_STAT Removed %s obj %p that was used %lu times (%lu total objs & %lu %s objs).", string_XGL_OBJECT_TYPE(pTrav->obj.objType), pTrav->obj.pObj, pTrav->obj.numUses, numTotalObjs, numObjs[pTrav->obj.objType], string_XGL_OBJECT_TYPE(pTrav->obj.objType));
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, pObj, 0, GLVSNAPSHOT_NONE, LAYER_ABBREV_STR, str);
            free(pTrav);
            return;
        }
        pPrev = pTrav;
        pTrav = pTrav->pNextGlobal;
    }
    char str[1024];
    sprintf(str, "Unable to remove obj %p. Was it created? Has it already been destroyed?", pObj);
    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pObj, 0, GLVSNAPSHOT_DESTROY_OBJECT_FAILED, LAYER_ABBREV_STR, str);
}

// Set selected flag state for an object node
static void set_status(void* pObj, XGL_OBJECT_TYPE objType, OBJECT_STATUS status_flag) {
    if (pObj != NULL) {
        objNode *pTrav = pObjectHead[objType];
        while (pTrav) {
            if (pTrav->obj.pObj == pObj) {
                pTrav->obj.status |= status_flag;
                return;
            }
            pTrav = pTrav->pNextObj;
        }
        // If we do not find it print an error
        char str[1024];
        sprintf(str, "Unable to set status for non-existent object %p of %s type", pObj, string_XGL_OBJECT_TYPE(objType));
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pObj, 0, GLVSNAPSHOT_UNKNOWN_OBJECT, LAYER_ABBREV_STR, str);
    }
}

// Track selected state for an object node
static void track_object_status(void* pObj, XGL_STATE_BIND_POINT stateBindPoint) {
    objNode *pTrav = pObjectHead[XGL_OBJECT_TYPE_CMD_BUFFER];

    while (pTrav) {
        if (pTrav->obj.pObj == pObj) {
            if (stateBindPoint == XGL_STATE_BIND_VIEWPORT) {
                pTrav->obj.status |= OBJSTATUS_VIEWPORT_BOUND;
            } else if (stateBindPoint == XGL_STATE_BIND_RASTER) {
                pTrav->obj.status |= OBJSTATUS_RASTER_BOUND;
            } else if (stateBindPoint == XGL_STATE_BIND_COLOR_BLEND) {
                pTrav->obj.status |= OBJSTATUS_COLOR_BLEND_BOUND;
            } else if (stateBindPoint == XGL_STATE_BIND_DEPTH_STENCIL) {
                pTrav->obj.status |= OBJSTATUS_DEPTH_STENCIL_BOUND;
            }
            return;
        }
        pTrav = pTrav->pNextObj;
    }
    // If we do not find it print an error
    char str[1024];
    sprintf(str, "Unable to track status for non-existent Command Buffer object %p", pObj);
    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pObj, 0, GLVSNAPSHOT_UNKNOWN_OBJECT, LAYER_ABBREV_STR, str);
}

// Reset selected flag state for an object node
static void reset_status(void* pObj, XGL_OBJECT_TYPE objType, OBJECT_STATUS status_flag) {
    objNode *pTrav = pObjectHead[objType];
    while (pTrav) {
        if (pTrav->obj.pObj == pObj) {
            pTrav->obj.status &= ~status_flag;
            return;
        }
        pTrav = pTrav->pNextObj;
    }
    // If we do not find it print an error
    char str[1024];
    sprintf(str, "Unable to reset status for non-existent object %p of %s type", pObj, string_XGL_OBJECT_TYPE(objType));
    layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pObj, 0, GLVSNAPSHOT_UNKNOWN_OBJECT, LAYER_ABBREV_STR, str);
}

// Check object status for selected flag state
static bool32_t validate_status(void* pObj, XGL_OBJECT_TYPE objType, OBJECT_STATUS status_mask, OBJECT_STATUS status_flag, XGL_DBG_MSG_TYPE error_level, GLAVE_SNAPSHOT_ERROR error_code, char* fail_msg) {
    objNode *pTrav = pObjectHead[objType];
    while (pTrav) {
        if (pTrav->obj.pObj == pObj) {
            if ((pTrav->obj.status & status_mask) != status_flag) {
                char str[1024];
                sprintf(str, "OBJECT VALIDATION WARNING: %s object %p: %s", string_XGL_OBJECT_TYPE(objType), (void*)pObj, fail_msg);
                layerCbMsg(error_level, XGL_VALIDATION_LEVEL_0, pObj, 0, error_code, LAYER_ABBREV_STR, str);
                return XGL_FALSE;
            }
            return XGL_TRUE;
        }
        pTrav = pTrav->pNextObj;
    }
    if (objType != XGL_OBJECT_TYPE_PRESENTABLE_IMAGE_MEMORY) {
        // If we do not find it print an error
        char str[1024];
        sprintf(str, "Unable to obtain status for non-existent object %p of %s type", pObj, string_XGL_OBJECT_TYPE(objType));
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, pObj, 0, GLVSNAPSHOT_UNKNOWN_OBJECT, LAYER_ABBREV_STR, str);
    }
    return XGL_FALSE;
}

static void validate_draw_state_flags(void* pObj) {
    validate_status((void*)pObj, XGL_OBJECT_TYPE_CMD_BUFFER, OBJSTATUS_VIEWPORT_BOUND,      OBJSTATUS_VIEWPORT_BOUND,      XGL_DBG_MSG_ERROR,    GLVSNAPSHOT_VIEWPORT_NOT_BOUND,      "Viewport object not bound to this command buffer");
    validate_status((void*)pObj, XGL_OBJECT_TYPE_CMD_BUFFER, OBJSTATUS_RASTER_BOUND,        OBJSTATUS_RASTER_BOUND,        XGL_DBG_MSG_ERROR,    GLVSNAPSHOT_RASTER_NOT_BOUND,        "Raster object not bound to this command buffer");
    validate_status((void*)pObj, XGL_OBJECT_TYPE_CMD_BUFFER, OBJSTATUS_COLOR_BLEND_BOUND,   OBJSTATUS_COLOR_BLEND_BOUND,   XGL_DBG_MSG_UNKNOWN,  GLVSNAPSHOT_COLOR_BLEND_NOT_BOUND,   "Color-blend object not bound to this command buffer");
    validate_status((void*)pObj, XGL_OBJECT_TYPE_CMD_BUFFER, OBJSTATUS_DEPTH_STENCIL_BOUND, OBJSTATUS_DEPTH_STENCIL_BOUND, XGL_DBG_MSG_UNKNOWN,  GLVSNAPSHOT_DEPTH_STENCIL_NOT_BOUND, "Depth-stencil object not bound to this command buffer");
}

static void validate_memory_mapping_status(const XGL_MEMORY_REF* pMemRefs, uint32_t numRefs) {
    uint32_t i;
    for (i = 0; i < numRefs; i++) {
        if(pMemRefs[i].mem) {
            // If mem reference is in presentable image memory list, skip the check of the GPU_MEMORY list
            if (!validate_status((void *)pMemRefs[i].mem, XGL_OBJECT_TYPE_PRESENTABLE_IMAGE_MEMORY, OBJSTATUS_NONE, OBJSTATUS_NONE, XGL_DBG_MSG_UNKNOWN, GLVSNAPSHOT_NONE, NULL) == XGL_TRUE)
            {
                validate_status((void *)pMemRefs[i].mem, XGL_OBJECT_TYPE_GPU_MEMORY, OBJSTATUS_GPU_MEM_MAPPED, OBJSTATUS_NONE, XGL_DBG_MSG_ERROR, GLVSNAPSHOT_GPU_MEM_MAPPED, "A Mapped Memory Object was referenced in a command buffer");
            }
        }
    }
}

static void validate_mem_ref_count(uint32_t numRefs) {
    if (maxMemRefsPerSubmission == 0) {
        char str[1024];
        sprintf(str, "xglQueueSubmit called before calling xglGetGpuInfo");
        layerCbMsg(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, NULL, 0, GLVSNAPSHOT_GETGPUINFO_NOT_CALLED, LAYER_ABBREV_STR, str);
    } else {
        if (numRefs > maxMemRefsPerSubmission) {
            char str[1024];
            sprintf(str, "xglQueueSubmit Memory reference count (%d) exceeds allowable GPU limit (%d)", numRefs, maxMemRefsPerSubmission);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, GLVSNAPSHOT_MEMREFCOUNT_MAX_EXCEEDED, LAYER_ABBREV_STR, str);
        }
    }
}

static void setGpuInfoState(void *pData) {
    maxMemRefsPerSubmission = ((XGL_PHYSICAL_GPU_PROPERTIES *)pData)->maxMemRefsPerSubmission;
}

#include "xgl_dispatch_table_helper.h"
static void initGlaveSnapshot(void)
{

    const char *strOpt;
    // initialize GlaveSnapshot options
    getLayerOptionEnum(LAYER_NAME_STR "ReportLevel", (uint32_t *) &g_reportingLevel);
    g_actionIsDefault = getLayerOptionEnum(LAYER_NAME_STR "DebugAction", (uint32_t *) &g_debugAction);

    if (g_debugAction & XGL_DBG_LAYER_ACTION_LOG_MSG)
    {
        strOpt = getLayerOption(LAYER_NAME_STR "LogFilename");
        if (strOpt)
        {
            g_logFile = fopen(strOpt, "w");
        }
        if (g_logFile == NULL)
            g_logFile = stdout;
    }

    xglGetProcAddrType fpNextGPA;
    fpNextGPA = pCurObj->pGPA;
    assert(fpNextGPA);

    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (XGL_PHYSICAL_GPU) pCurObj->nextObject);
    if (!objLockInitialized)
    {
        // TODO/TBD: Need to delete this mutex sometime.  How???
        loader_platform_thread_create_mutex(&objLock);
        objLockInitialized = 1;
    }
}


XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateInstance(const XGL_APPLICATION_INFO* pAppInfo, const XGL_ALLOC_CALLBACKS* pAllocCb, XGL_INSTANCE* pInstance)
{
    XGL_RESULT result = nextTable.CreateInstance(pAppInfo, pAllocCb, pInstance);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pInstance, XGL_OBJECT_TYPE_INSTANCE);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyInstance(XGL_INSTANCE instance)
{
    XGL_RESULT result = nextTable.DestroyInstance(instance);
    loader_platform_thread_lock_mutex(&objLock);
    ll_destroy_obj((void*)instance);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateGpus(XGL_INSTANCE instance, uint32_t maxGpus, uint32_t* pGpuCount, XGL_PHYSICAL_GPU* pGpus)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)instance, XGL_OBJECT_TYPE_INSTANCE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.EnumerateGpus(instance, maxGpus, pGpuCount, pGpus);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetGpuInfo(XGL_PHYSICAL_GPU gpu, XGL_PHYSICAL_GPU_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initGlaveSnapshot);
    XGL_RESULT result = nextTable.GetGpuInfo((XGL_PHYSICAL_GPU)gpuw->nextObject, infoType, pDataSize, pData);
    if (infoType == XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES) {
        if (pData != NULL) {
            setGpuInfoState(pData);
        }
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo, XGL_DEVICE* pDevice)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initGlaveSnapshot);
    XGL_RESULT result = nextTable.CreateDevice((XGL_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pDevice, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyDevice(XGL_DEVICE device)
{
    XGL_RESULT result = nextTable.DestroyDevice(device);
    loader_platform_thread_lock_mutex(&objLock);
    ll_destroy_obj((void*)device);
    loader_platform_thread_unlock_mutex(&objLock);
    // Report any remaining objects in LL
    objNode *pTrav = pGlobalHead;
    while (pTrav) {
        if (pTrav->obj.objType == XGL_OBJECT_TYPE_PRESENTABLE_IMAGE_MEMORY) {
            objNode *pDel = pTrav;
            pTrav = pTrav->pNextGlobal;
            ll_destroy_obj((void*)(pDel->obj.pObj));
        } else {
            char str[1024];
            sprintf(str, "OBJ ERROR : %s object %p has not been destroyed (was used %lu times).", string_XGL_OBJECT_TYPE(pTrav->obj.objType), pTrav->obj.pObj, pTrav->obj.numUses);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, device, 0, GLVSNAPSHOT_OBJECT_LEAK, LAYER_ABBREV_STR, str);
            pTrav = pTrav->pNextGlobal;
        }
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetExtensionSupport(XGL_PHYSICAL_GPU gpu, const char* pExtName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)gpu, XGL_OBJECT_TYPE_PHYSICAL_GPU);
    loader_platform_thread_unlock_mutex(&objLock);
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initGlaveSnapshot);
    XGL_RESULT result = nextTable.GetExtensionSupport((XGL_PHYSICAL_GPU)gpuw->nextObject, pExtName);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(XGL_PHYSICAL_GPU gpu, size_t maxLayerCount, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved)
{
    if (gpu != NULL) {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)gpu, XGL_OBJECT_TYPE_PHYSICAL_GPU);
    loader_platform_thread_unlock_mutex(&objLock);
        pCurObj = gpuw;
        loader_platform_thread_once(&tabOnce, initGlaveSnapshot);
        XGL_RESULT result = nextTable.EnumerateLayers((XGL_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
            return result;
    } else {
        if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)
            return XGL_ERROR_INVALID_POINTER;
        // This layer compatible with all GPUs
        *pOutLayerCount = 1;
        strncpy((char *) pOutLayers[0], LAYER_NAME_STR, maxStringSize);
        return XGL_SUCCESS;
    }
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetDeviceQueue(XGL_DEVICE device, XGL_QUEUE_TYPE queueType, uint32_t queueIndex, XGL_QUEUE* pQueue)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.GetDeviceQueue(device, queueType, queueIndex, pQueue);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSubmit(XGL_QUEUE queue, uint32_t cmdBufferCount, const XGL_CMD_BUFFER* pCmdBuffers, uint32_t memRefCount, const XGL_MEMORY_REF* pMemRefs, XGL_FENCE fence)
{
    set_status((void*)fence, XGL_OBJECT_TYPE_FENCE, OBJSTATUS_FENCE_IS_SUBMITTED);
    validate_memory_mapping_status(pMemRefs, memRefCount);
    validate_mem_ref_count(memRefCount);
    XGL_RESULT result = nextTable.QueueSubmit(queue, cmdBufferCount, pCmdBuffers, memRefCount, pMemRefs, fence);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSetGlobalMemReferences(XGL_QUEUE queue, uint32_t memRefCount, const XGL_MEMORY_REF* pMemRefs)
{
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
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.DeviceWaitIdle(device);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglAllocMemory(XGL_DEVICE device, const XGL_MEMORY_ALLOC_INFO* pAllocInfo, XGL_GPU_MEMORY* pMem)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.AllocMemory(device, pAllocInfo, pMem);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pMem, XGL_OBJECT_TYPE_GPU_MEMORY);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglFreeMemory(XGL_GPU_MEMORY mem)
{
    XGL_RESULT result = nextTable.FreeMemory(mem);
    loader_platform_thread_lock_mutex(&objLock);
    ll_destroy_obj((void*)mem);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetMemoryPriority(XGL_GPU_MEMORY mem, XGL_MEMORY_PRIORITY priority)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)mem, XGL_OBJECT_TYPE_GPU_MEMORY);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.SetMemoryPriority(mem, priority);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglMapMemory(XGL_GPU_MEMORY mem, XGL_FLAGS flags, void** ppData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)mem, XGL_OBJECT_TYPE_GPU_MEMORY);
    loader_platform_thread_unlock_mutex(&objLock);
    set_status((void*)mem, XGL_OBJECT_TYPE_GPU_MEMORY, OBJSTATUS_GPU_MEM_MAPPED);
    XGL_RESULT result = nextTable.MapMemory(mem, flags, ppData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglUnmapMemory(XGL_GPU_MEMORY mem)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)mem, XGL_OBJECT_TYPE_GPU_MEMORY);
    loader_platform_thread_unlock_mutex(&objLock);
    reset_status((void*)mem, XGL_OBJECT_TYPE_GPU_MEMORY, OBJSTATUS_GPU_MEM_MAPPED);
    XGL_RESULT result = nextTable.UnmapMemory(mem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglPinSystemMemory(XGL_DEVICE device, const void* pSysMem, size_t memSize, XGL_GPU_MEMORY* pMem)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.PinSystemMemory(device, pSysMem, memSize, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetMultiGpuCompatibility(XGL_PHYSICAL_GPU gpu0, XGL_PHYSICAL_GPU gpu1, XGL_GPU_COMPATIBILITY_INFO* pInfo)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu0;
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)gpu0, XGL_OBJECT_TYPE_PHYSICAL_GPU);
    loader_platform_thread_unlock_mutex(&objLock);
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initGlaveSnapshot);
    XGL_RESULT result = nextTable.GetMultiGpuCompatibility((XGL_PHYSICAL_GPU)gpuw->nextObject, gpu1, pInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenSharedMemory(XGL_DEVICE device, const XGL_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.OpenSharedMemory(device, pOpenInfo, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenSharedQueueSemaphore(XGL_DEVICE device, const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo, XGL_QUEUE_SEMAPHORE* pSemaphore)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.OpenSharedQueueSemaphore(device, pOpenInfo, pSemaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenPeerMemory(XGL_DEVICE device, const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.OpenPeerMemory(device, pOpenInfo, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenPeerImage(XGL_DEVICE device, const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo, XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.OpenPeerImage(device, pOpenInfo, pImage, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyObject(XGL_OBJECT object)
{
    XGL_RESULT result = nextTable.DestroyObject(object);
    loader_platform_thread_lock_mutex(&objLock);
    ll_destroy_obj((void*)object);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetObjectInfo(XGL_BASE_OBJECT object, XGL_OBJECT_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)object, ll_get_obj_type(object));
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.GetObjectInfo(object, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindObjectMemory(XGL_OBJECT object, uint32_t allocationIdx, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)object, ll_get_obj_type(object));
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.BindObjectMemory(object, allocationIdx, mem, offset);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindObjectMemoryRange(XGL_OBJECT object, uint32_t allocationIdx, XGL_GPU_SIZE rangeOffset, XGL_GPU_SIZE rangeSize, XGL_GPU_MEMORY mem, XGL_GPU_SIZE memOffset)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)object, ll_get_obj_type(object));
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.BindObjectMemoryRange(object, allocationIdx, rangeOffset, rangeSize, mem, memOffset);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindImageMemoryRange(XGL_IMAGE image, uint32_t allocationIdx, const XGL_IMAGE_MEMORY_BIND_INFO* bindInfo, XGL_GPU_MEMORY mem, XGL_GPU_SIZE memOffset)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)image, XGL_OBJECT_TYPE_IMAGE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.BindImageMemoryRange(image, allocationIdx, bindInfo, mem, memOffset);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateFence(XGL_DEVICE device, const XGL_FENCE_CREATE_INFO* pCreateInfo, XGL_FENCE* pFence)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateFence(device, pCreateInfo, pFence);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pFence, XGL_OBJECT_TYPE_FENCE);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFenceStatus(XGL_FENCE fence)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)fence, XGL_OBJECT_TYPE_FENCE);
    loader_platform_thread_unlock_mutex(&objLock);
    // Warn if submitted_flag is not set
    validate_status((void*)fence, XGL_OBJECT_TYPE_FENCE, OBJSTATUS_FENCE_IS_SUBMITTED, OBJSTATUS_FENCE_IS_SUBMITTED, XGL_DBG_MSG_ERROR, GLVSNAPSHOT_INVALID_FENCE, "Status Requested for Unsubmitted Fence");
    XGL_RESULT result = nextTable.GetFenceStatus(fence);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWaitForFences(XGL_DEVICE device, uint32_t fenceCount, const XGL_FENCE* pFences, bool32_t waitAll, uint64_t timeout)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateQueueSemaphore(XGL_DEVICE device, const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo, XGL_QUEUE_SEMAPHORE* pSemaphore)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateQueueSemaphore(device, pCreateInfo, pSemaphore);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pSemaphore, XGL_OBJECT_TYPE_QUEUE_SEMAPHORE);
    loader_platform_thread_unlock_mutex(&objLock);
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
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateEvent(device, pCreateInfo, pEvent);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pEvent, XGL_OBJECT_TYPE_EVENT);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetEventStatus(XGL_EVENT event)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)event, XGL_OBJECT_TYPE_EVENT);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.GetEventStatus(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetEvent(XGL_EVENT event)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)event, XGL_OBJECT_TYPE_EVENT);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.SetEvent(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglResetEvent(XGL_EVENT event)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)event, XGL_OBJECT_TYPE_EVENT);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.ResetEvent(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateQueryPool(XGL_DEVICE device, const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo, XGL_QUERY_POOL* pQueryPool)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateQueryPool(device, pCreateInfo, pQueryPool);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pQueryPool, XGL_OBJECT_TYPE_QUERY_POOL);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetQueryPoolResults(XGL_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount, size_t* pDataSize, void* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)queryPool, XGL_OBJECT_TYPE_QUERY_POOL);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.GetQueryPoolResults(queryPool, startQuery, queryCount, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFormatInfo(XGL_DEVICE device, XGL_FORMAT format, XGL_FORMAT_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.GetFormatInfo(device, format, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateBuffer(XGL_DEVICE device, const XGL_BUFFER_CREATE_INFO* pCreateInfo, XGL_BUFFER* pBuffer)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateBuffer(device, pCreateInfo, pBuffer);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pBuffer, XGL_OBJECT_TYPE_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateBufferView(XGL_DEVICE device, const XGL_BUFFER_VIEW_CREATE_INFO* pCreateInfo, XGL_BUFFER_VIEW* pView)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateBufferView(device, pCreateInfo, pView);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pView, XGL_OBJECT_TYPE_BUFFER_VIEW);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImage(XGL_DEVICE device, const XGL_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateImage(device, pCreateInfo, pImage);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pImage, XGL_OBJECT_TYPE_IMAGE);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetFastClearColor(XGL_IMAGE image, const float color[4])
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)image, XGL_OBJECT_TYPE_IMAGE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.SetFastClearColor(image, color);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetFastClearDepth(XGL_IMAGE image, float depth)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)image, XGL_OBJECT_TYPE_IMAGE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.SetFastClearDepth(image, depth);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetImageSubresourceInfo(XGL_IMAGE image, const XGL_IMAGE_SUBRESOURCE* pSubresource, XGL_SUBRESOURCE_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)image, XGL_OBJECT_TYPE_IMAGE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.GetImageSubresourceInfo(image, pSubresource, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImageView(XGL_DEVICE device, const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo, XGL_IMAGE_VIEW* pView)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateImageView(device, pCreateInfo, pView);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pView, XGL_OBJECT_TYPE_IMAGE_VIEW);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateColorAttachmentView(XGL_DEVICE device, const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo, XGL_COLOR_ATTACHMENT_VIEW* pView)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateColorAttachmentView(device, pCreateInfo, pView);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pView, XGL_OBJECT_TYPE_COLOR_ATTACHMENT_VIEW);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDepthStencilView(XGL_DEVICE device, const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo, XGL_DEPTH_STENCIL_VIEW* pView)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateDepthStencilView(device, pCreateInfo, pView);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pView, XGL_OBJECT_TYPE_DEPTH_STENCIL_VIEW);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateShader(XGL_DEVICE device, const XGL_SHADER_CREATE_INFO* pCreateInfo, XGL_SHADER* pShader)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateShader(device, pCreateInfo, pShader);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pShader, XGL_OBJECT_TYPE_SHADER);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateGraphicsPipeline(XGL_DEVICE device, const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pPipeline, XGL_OBJECT_TYPE_PIPELINE);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateComputePipeline(XGL_DEVICE device, const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateComputePipeline(device, pCreateInfo, pPipeline);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pPipeline, XGL_OBJECT_TYPE_PIPELINE);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglStorePipeline(XGL_PIPELINE pipeline, size_t* pDataSize, void* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)pipeline, XGL_OBJECT_TYPE_PIPELINE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.StorePipeline(pipeline, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglLoadPipeline(XGL_DEVICE device, size_t dataSize, const void* pData, XGL_PIPELINE* pPipeline)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.LoadPipeline(device, dataSize, pData, pPipeline);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreatePipelineDelta(XGL_DEVICE device, XGL_PIPELINE p1, XGL_PIPELINE p2, XGL_PIPELINE_DELTA* delta)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreatePipelineDelta(device, p1, p2, delta);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*delta, XGL_OBJECT_TYPE_PIPELINE_DELTA);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateSampler(XGL_DEVICE device, const XGL_SAMPLER_CREATE_INFO* pCreateInfo, XGL_SAMPLER* pSampler)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateSampler(device, pCreateInfo, pSampler);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pSampler, XGL_OBJECT_TYPE_SAMPLER);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorSetLayout(XGL_DEVICE device, XGL_FLAGS stageFlags, const uint32_t* pSetBindPoints, XGL_DESCRIPTOR_SET_LAYOUT priorSetLayout, const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pSetLayoutInfoList, XGL_DESCRIPTOR_SET_LAYOUT* pSetLayout)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateDescriptorSetLayout(device, stageFlags, pSetBindPoints, priorSetLayout, pSetLayoutInfoList, pSetLayout);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pSetLayout, XGL_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBeginDescriptorRegionUpdate(XGL_DEVICE device, XGL_DESCRIPTOR_UPDATE_MODE updateMode)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.BeginDescriptorRegionUpdate(device, updateMode);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEndDescriptorRegionUpdate(XGL_DEVICE device, XGL_CMD_BUFFER cmd)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.EndDescriptorRegionUpdate(device, cmd);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorRegion(XGL_DEVICE device, XGL_DESCRIPTOR_REGION_USAGE regionUsage, uint32_t maxSets, const XGL_DESCRIPTOR_REGION_CREATE_INFO* pCreateInfo, XGL_DESCRIPTOR_REGION* pDescriptorRegion)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateDescriptorRegion(device, regionUsage, maxSets, pCreateInfo, pDescriptorRegion);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pDescriptorRegion, XGL_OBJECT_TYPE_DESCRIPTOR_REGION);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglClearDescriptorRegion(XGL_DESCRIPTOR_REGION descriptorRegion)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)descriptorRegion, XGL_OBJECT_TYPE_DESCRIPTOR_REGION);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.ClearDescriptorRegion(descriptorRegion);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglAllocDescriptorSets(XGL_DESCRIPTOR_REGION descriptorRegion, XGL_DESCRIPTOR_SET_USAGE setUsage, uint32_t count, const XGL_DESCRIPTOR_SET_LAYOUT* pSetLayouts, XGL_DESCRIPTOR_SET* pDescriptorSets, uint32_t* pCount)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)descriptorRegion, XGL_OBJECT_TYPE_DESCRIPTOR_REGION);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.AllocDescriptorSets(descriptorRegion, setUsage, count, pSetLayouts, pDescriptorSets, pCount);
    for (uint32_t i = 0; i < *pCount; i++) {
        loader_platform_thread_lock_mutex(&objLock);
        ll_insert_obj((void*)pDescriptorSets[i], XGL_OBJECT_TYPE_DESCRIPTOR_SET);
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglClearDescriptorSets(XGL_DESCRIPTOR_REGION descriptorRegion, uint32_t count, const XGL_DESCRIPTOR_SET* pDescriptorSets)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)descriptorRegion, XGL_OBJECT_TYPE_DESCRIPTOR_REGION);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.ClearDescriptorSets(descriptorRegion, count, pDescriptorSets);
}

XGL_LAYER_EXPORT void XGLAPI xglUpdateDescriptors(XGL_DESCRIPTOR_SET descriptorSet, const void* pUpdateChain)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)descriptorSet, XGL_OBJECT_TYPE_DESCRIPTOR_SET);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.UpdateDescriptors(descriptorSet, pUpdateChain);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicViewportState(XGL_DEVICE device, const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_VP_STATE_OBJECT* pState)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateDynamicViewportState(device, pCreateInfo, pState);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pState, XGL_OBJECT_TYPE_DYNAMIC_VP_STATE_OBJECT);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicRasterState(XGL_DEVICE device, const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_RS_STATE_OBJECT* pState)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateDynamicRasterState(device, pCreateInfo, pState);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pState, XGL_OBJECT_TYPE_DYNAMIC_RS_STATE_OBJECT);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicColorBlendState(XGL_DEVICE device, const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_CB_STATE_OBJECT* pState)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateDynamicColorBlendState(device, pCreateInfo, pState);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pState, XGL_OBJECT_TYPE_DYNAMIC_CB_STATE_OBJECT);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicDepthStencilState(XGL_DEVICE device, const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_DS_STATE_OBJECT* pState)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateDynamicDepthStencilState(device, pCreateInfo, pState);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pState, XGL_OBJECT_TYPE_DYNAMIC_DS_STATE_OBJECT);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateCommandBuffer(XGL_DEVICE device, const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo, XGL_CMD_BUFFER* pCmdBuffer)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pCmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBeginCommandBuffer(XGL_CMD_BUFFER cmdBuffer, const XGL_CMD_BUFFER_BEGIN_INFO* pBeginInfo)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.BeginCommandBuffer(cmdBuffer, pBeginInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEndCommandBuffer(XGL_CMD_BUFFER cmdBuffer)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    reset_status((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER, (OBJSTATUS_VIEWPORT_BOUND    |
                                                                OBJSTATUS_RASTER_BOUND      |
                                                                OBJSTATUS_COLOR_BLEND_BOUND |
                                                                OBJSTATUS_DEPTH_STENCIL_BOUND));
    XGL_RESULT result = nextTable.EndCommandBuffer(cmdBuffer);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglResetCommandBuffer(XGL_CMD_BUFFER cmdBuffer)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.ResetCommandBuffer(cmdBuffer);
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindPipeline(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE pipeline)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindPipelineDelta(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE_DELTA delta)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdBindPipelineDelta(cmdBuffer, pipelineBindPoint, delta);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindDynamicStateObject(XGL_CMD_BUFFER cmdBuffer, XGL_STATE_BIND_POINT stateBindPoint, XGL_DYNAMIC_STATE_OBJECT state)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    track_object_status((void*)cmdBuffer, stateBindPoint);
    nextTable.CmdBindDynamicStateObject(cmdBuffer, stateBindPoint, state);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindDescriptorSet(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_DESCRIPTOR_SET descriptorSet, const uint32_t* pUserData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdBindDescriptorSet(cmdBuffer, pipelineBindPoint, descriptorSet, pUserData);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindVertexBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t binding)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdBindVertexBuffer(cmdBuffer, buffer, offset, binding);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindIndexBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, XGL_INDEX_TYPE indexType)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDraw(XGL_CMD_BUFFER cmdBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    validate_draw_state_flags((void *)cmdBuffer);
    nextTable.CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDrawIndexed(XGL_CMD_BUFFER cmdBuffer, uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    validate_draw_state_flags((void *)cmdBuffer);
    nextTable.CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDrawIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t count, uint32_t stride)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    validate_draw_state_flags((void *)cmdBuffer);
    nextTable.CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDrawIndexedIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t count, uint32_t stride)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    validate_draw_state_flags((void *)cmdBuffer);
    nextTable.CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDispatch(XGL_CMD_BUFFER cmdBuffer, uint32_t x, uint32_t y, uint32_t z)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdDispatch(cmdBuffer, x, y, z);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDispatchIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER srcBuffer, XGL_BUFFER destBuffer, uint32_t regionCount, const XGL_BUFFER_COPY* pRegions)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, uint32_t regionCount, const XGL_IMAGE_COPY* pRegions)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdCopyImage(cmdBuffer, srcImage, destImage, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyBufferToImage(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER srcBuffer, XGL_IMAGE destImage, uint32_t regionCount, const XGL_BUFFER_IMAGE_COPY* pRegions)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyImageToBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_BUFFER destBuffer, uint32_t regionCount, const XGL_BUFFER_IMAGE_COPY* pRegions)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdCopyImageToBuffer(cmdBuffer, srcImage, destBuffer, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCloneImageData(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE_LAYOUT srcImageLayout, XGL_IMAGE destImage, XGL_IMAGE_LAYOUT destImageLayout)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdCloneImageData(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdUpdateBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE dataSize, const uint32_t* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdFillBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE fillSize, uint32_t data)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdClearColorImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const float color[4], uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdClearColorImage(cmdBuffer, image, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdClearColorImageRaw(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const uint32_t color[4], uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdClearColorImageRaw(cmdBuffer, image, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdClearDepthStencil(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, float depth, uint32_t stencil, uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdClearDepthStencil(cmdBuffer, image, depth, stencil, rangeCount, pRanges);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdResolveImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, uint32_t rectCount, const XGL_IMAGE_RESOLVE* pRects)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdResolveImage(cmdBuffer, srcImage, destImage, rectCount, pRects);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdSetEvent(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event, XGL_SET_EVENT pipeEvent)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdSetEvent(cmdBuffer, event, pipeEvent);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdResetEvent(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdResetEvent(cmdBuffer, event);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdWaitEvents(XGL_CMD_BUFFER cmdBuffer, const XGL_EVENT_WAIT_INFO* pWaitInfo)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdWaitEvents(cmdBuffer, pWaitInfo);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdPipelineBarrier(XGL_CMD_BUFFER cmdBuffer, const XGL_PIPELINE_BARRIER* pBarrier)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdPipelineBarrier(cmdBuffer, pBarrier);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBeginQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t slot, XGL_FLAGS flags)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdEndQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t slot)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdEndQuery(cmdBuffer, queryPool, slot);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdResetQueryPool(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdWriteTimestamp(XGL_CMD_BUFFER cmdBuffer, XGL_TIMESTAMP_TYPE timestampType, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdInitAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, const uint32_t* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdLoadAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, XGL_BUFFER srcBuffer, XGL_GPU_SIZE srcOffset)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcBuffer, srcOffset);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdSaveAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destBuffer, destOffset);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateFramebuffer(XGL_DEVICE device, const XGL_FRAMEBUFFER_CREATE_INFO* pCreateInfo, XGL_FRAMEBUFFER* pFramebuffer)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateFramebuffer(device, pCreateInfo, pFramebuffer);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pFramebuffer, XGL_OBJECT_TYPE_FRAMEBUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateRenderPass(XGL_DEVICE device, const XGL_RENDER_PASS_CREATE_INFO* pCreateInfo, XGL_RENDER_PASS* pRenderPass)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.CreateRenderPass(device, pCreateInfo, pRenderPass);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pRenderPass, XGL_OBJECT_TYPE_RENDER_PASS);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBeginRenderPass(XGL_CMD_BUFFER cmdBuffer, XGL_RENDER_PASS renderPass)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdBeginRenderPass(cmdBuffer, renderPass);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdEndRenderPass(XGL_CMD_BUFFER cmdBuffer, XGL_RENDER_PASS renderPass)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdEndRenderPass(cmdBuffer, renderPass);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetValidationLevel(XGL_DEVICE device, XGL_VALIDATION_LEVEL validationLevel)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
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
    // force callbacks if DebugAction hasn't been set already other than initial value
    if (g_actionIsDefault) {
        g_debugAction = XGL_DBG_LAYER_ACTION_CALLBACK;
    }    XGL_RESULT result = nextTable.DbgRegisterMsgCallback(pfnMsgCallback, pUserData);
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
        if (g_actionIsDefault)
            g_debugAction = XGL_DBG_LAYER_ACTION_LOG_MSG;
        else
            g_debugAction &= ~XGL_DBG_LAYER_ACTION_CALLBACK;
    }
    XGL_RESULT result = nextTable.DbgUnregisterMsgCallback(pfnMsgCallback);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetMessageFilter(XGL_DEVICE device, int32_t msgCode, XGL_DBG_MSG_FILTER filter)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.DbgSetMessageFilter(device, msgCode, filter);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetObjectTag(XGL_BASE_OBJECT object, size_t tagSize, const void* pTag)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)object, ll_get_obj_type(object));
    loader_platform_thread_unlock_mutex(&objLock);
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
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.DbgSetDeviceOption(device, dbgOption, dataSize, pData);
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDbgMarkerBegin(XGL_CMD_BUFFER cmdBuffer, const char* pMarker)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdDbgMarkerBegin(cmdBuffer, pMarker);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDbgMarkerEnd(XGL_CMD_BUFFER cmdBuffer)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)cmdBuffer, XGL_OBJECT_TYPE_CMD_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdDbgMarkerEnd(cmdBuffer);
}

#if defined(__linux__) || defined(XCB_NVIDIA)

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11AssociateConnection(XGL_PHYSICAL_GPU gpu, const XGL_WSI_X11_CONNECTION_INFO* pConnectionInfo)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)gpu, XGL_OBJECT_TYPE_PHYSICAL_GPU);
    loader_platform_thread_unlock_mutex(&objLock);
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initGlaveSnapshot);
    XGL_RESULT result = nextTable.WsiX11AssociateConnection((XGL_PHYSICAL_GPU)gpuw->nextObject, pConnectionInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11GetMSC(XGL_DEVICE device, xcb_window_t window, xcb_randr_crtc_t crtc, uint64_t* pMsc)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.WsiX11GetMSC(device, window, crtc, pMsc);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11CreatePresentableImage(XGL_DEVICE device, const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count((void*)device, XGL_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    XGL_RESULT result = nextTable.WsiX11CreatePresentableImage(device, pCreateInfo, pImage, pMem);
    loader_platform_thread_lock_mutex(&objLock);
    ll_insert_obj((void*)*pImage, XGL_OBJECT_TYPE_IMAGE);
    ll_insert_obj((void*)*pMem, XGL_OBJECT_TYPE_PRESENTABLE_IMAGE_MEMORY);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11QueuePresent(XGL_QUEUE queue, const XGL_WSI_X11_PRESENT_INFO* pPresentInfo, XGL_FENCE fence)
{
    XGL_RESULT result = nextTable.WsiX11QueuePresent(queue, pPresentInfo, fence);
    return result;
}

#endif

//=============================================================================
// Exported methods
//=============================================================================
uint64_t glvSnapshotGetObjectCount(XGL_OBJECT_TYPE type)
{
    return (type == XGL_OBJECT_TYPE_ANY) ? numTotalObjs : numObjs[type];
}

XGL_RESULT glvSnapshotGetObjects(XGL_OBJECT_TYPE type, uint64_t objCount, GLVSNAPSHOT_NODE* pObjNodeArray)
{
    // This bool flags if we're pulling all objs or just a single class of objs
    bool32_t bAllObjs = (type == XGL_OBJECT_TYPE_ANY);
    // Check the count first thing
    uint64_t maxObjCount = (bAllObjs) ? numTotalObjs : numObjs[type];
    if (objCount > maxObjCount) {
        char str[1024];
        sprintf(str, "OBJ ERROR : Received objTrackGetObjects() request for %lu objs, but there are only %lu objs of type %s", objCount, maxObjCount, string_XGL_OBJECT_TYPE(type));
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, 0, 0, GLVSNAPSHOT_OBJCOUNT_MAX_EXCEEDED, LAYER_ABBREV_STR, str);
        return XGL_ERROR_INVALID_VALUE;
    }
    objNode* pTrav = (bAllObjs) ? pGlobalHead : pObjectHead[type];
    for (uint64_t i = 0; i < objCount; i++) {
        if (!pTrav) {
            char str[1024];
            sprintf(str, "OBJ INTERNAL ERROR : Ran out of %s objs! Should have %lu, but only copied %lu and not the requested %lu.", string_XGL_OBJECT_TYPE(type), maxObjCount, i, objCount);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, 0, 0, GLVSNAPSHOT_INTERNAL_ERROR, LAYER_ABBREV_STR, str);
            return XGL_ERROR_UNKNOWN;
        }
        memcpy(&pObjNodeArray[i], pTrav, sizeof(GLVSNAPSHOT_NODE));
        pTrav = (bAllObjs) ? pTrav->pNextGlobal : pTrav->pNextObj;
    }
    return XGL_SUCCESS;
}

void glvSnapshotPrintObjects(void)
{
    ll_print_lists();
}

#include "xgl_generic_intercept_proc_helper.h"
XGL_LAYER_EXPORT void* XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const char* funcName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    void* addr;
    if (gpu == NULL)
        return NULL;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initGlaveSnapshot);

    addr = layer_intercept_proc(funcName);
    if (addr)
        return addr;
    else if (!strncmp("glvSnapshotGetObjectCount", funcName, sizeof("glvSnapshotGetObjectCount")))
        return glvSnapshotGetObjectCount;
    else if (!strncmp("glvSnapshotGetObjects", funcName, sizeof("glvSnapshotGetObjects")))
        return glvSnapshotGetObjects;
    else if (!strncmp("glvSnapshotPrintObjects", funcName, sizeof("glvSnapshotPrintObjects")))
        return glvSnapshotPrintObjects;
    else {
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((XGL_PHYSICAL_GPU)gpuw->nextObject, funcName);
    }
}

