/*
 *
 * Copyright (C) 2015 Valve Corporation
 * All Rights Reserved.
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
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Peter Lohrmann <peterl@valvesoftware.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "loader_platform.h"
#include "vktrace_snapshot.h"
#include "vk_struct_string_helper.h"

#define LAYER_NAME_STR "VktraceSnapshot"
#define LAYER_ABBREV_STR "VKTRACESnap"

static VkLayerDispatchTable nextTable;
static VkBaseLayerObject *pCurObj;

// The following is #included again to catch certain OS-specific functions being used:
#include "loader_platform.h"
#include "layers_config.h"
#include "layers_msg.h"

static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);
static int objLockInitialized = 0;
static loader_platform_thread_mutex objLock;

// The 'masterSnapshot' which gets the delta merged into it when 'GetSnapshot()' is called.
static VKTRACE_VK_SNAPSHOT s_snapshot = {0};

// The 'deltaSnapshot' which tracks all object creation and deletion.
static VKTRACE_VK_SNAPSHOT s_delta = {0};


//=============================================================================
// Helper structure for a VKTRACE vulkan snapshot.
// These can probably be auto-generated at some point.
//=============================================================================

void vktrace_vk_malloc_and_copy(void** ppDest, size_t size, const void* pSrc)
{
    *ppDest = malloc(size);
    memcpy(*ppDest, pSrc, size);
}

VkDeviceCreateInfo* vktrace_deepcopy_VkDeviceCreateInfo(const VkDeviceCreateInfo* pSrcCreateInfo)
{
    VkDeviceCreateInfo* pDestCreateInfo;

    // NOTE: partially duplicated code from add_VkDeviceCreateInfo_to_packet(...)
    {
        uint32_t i;
        vktrace_vk_malloc_and_copy((void**)&pDestCreateInfo, sizeof(VkDeviceCreateInfo), pSrcCreateInfo);
        vktrace_vk_malloc_and_copy((void**)&pDestCreateInfo->pRequestedQueues, pSrcCreateInfo->requestedQueueCount*sizeof(VkDeviceQueueCreateInfo), pSrcCreateInfo->pRequestedQueues);
        for (i = 0; i < pSrcCreateInfo->requestedQueueCount; i++) {
            vktrace_vk_malloc_and_copy((void**)&pDestCreateInfo->pRequestedQueues[i].pQueuePriorities,
                                       pSrcCreateInfo->pRequestedQueues[i].queueCount*sizeof(float),
                                       pSrcCreateInfo->pRequestedQueues[i].pQueuePriorities);
        }

        if (pSrcCreateInfo->enabledExtensionNameCount > 0)
        {
            vktrace_vk_malloc_and_copy((void**)&pDestCreateInfo->ppEnabledExtensionNames, pSrcCreateInfo->enabledExtensionNameCount * sizeof(char *), pSrcCreateInfo->ppEnabledExtensionNames);
            for (i = 0; i < pSrcCreateInfo->enabledExtensionNameCount; i++)
            {
                vktrace_vk_malloc_and_copy((void**)&pDestCreateInfo->ppEnabledExtensionNames[i], strlen(pSrcCreateInfo->ppEnabledExtensionNames[i]) + 1, pSrcCreateInfo->ppEnabledExtensionNames[i]);
            }
        }
        VkLayerCreateInfo *pSrcNext = ( VkLayerCreateInfo *) pSrcCreateInfo->pNext;
        VkLayerCreateInfo **ppDstNext = ( VkLayerCreateInfo **) &pDestCreateInfo->pNext;
        while (pSrcNext != NULL)
        {
            if ((pSrcNext->sType == VK_STRUCTURE_TYPE_LAYER_CREATE_INFO) && pSrcNext->enabledLayerNameCount > 0)
            {
                vktrace_vk_malloc_and_copy((void**)ppDstNext, sizeof(VkLayerCreateInfo), pSrcNext);
                vktrace_vk_malloc_and_copy((void**)&(*ppDstNext)->ppActiveLayerNames, pSrcNext->enabledLayerNameCount * sizeof(char*), pSrcNext->ppActiveLayerNames);
                for (i = 0; i < pSrcNext->enabledLayerNameCount; i++)
                {
                    vktrace_vk_malloc_and_copy((void**)&(*ppDstNext)->ppActiveLayerNames[i], strlen(pSrcNext->ppActiveLayerNames[i]) + 1, pSrcNext->ppActiveLayerNames[i]);
                }

                ppDstNext = (VkLayerCreateInfo**) &(*ppDstNext)->pNext;
            }
            pSrcNext = (VkLayerCreateInfo*) pSrcNext->pNext;
        }
    }

    return pDestCreateInfo;
}

void vktrace_deepfree_VkDeviceCreateInfo(VkDeviceCreateInfo* pCreateInfo)
{
    uint32_t i;
    if (pCreateInfo->pRequestedQueues != NULL)
    {
        free((void*)pCreateInfo->pRequestedQueues);
    }

    if (pCreateInfo->ppEnabledExtensionNames != NULL)
    {
        for (i = 0; i < pCreateInfo->enabledExtensionNameCount; i++)
        {
            free((void*)pCreateInfo->ppEnabledExtensionNames[i]);
        }
        free((void*)pCreateInfo->ppEnabledExtensionNames);
    }

    VkLayerCreateInfo *pSrcNext = (VkLayerCreateInfo*)pCreateInfo->pNext;
    while (pSrcNext != NULL)
    {
        VkLayerCreateInfo* pTmp = (VkLayerCreateInfo*)pSrcNext->pNext;
        if ((pSrcNext->sType == VK_STRUCTURE_TYPE_LAYER_CREATE_INFO) && pSrcNext->enabledLayerNameCount > 0)
        {
            for (i = 0; i < pSrcNext->enabledLayerNameCount; i++)
            {
                free((void*)pSrcNext->ppActiveLayerNames[i]);
            }

            free((void*)pSrcNext->ppActiveLayerNames);
            free(pSrcNext);
        }
        pSrcNext = pTmp;
    }

    free(pCreateInfo);
}

void vktrace_vk_snapshot_copy_createdevice_params(VKTRACE_VK_SNAPSHOT_CREATEDEVICE_PARAMS* pDest, VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    pDest->physicalDevice = physicalDevice;

    pDest->pCreateInfo = vktrace_deepcopy_VkDeviceCreateInfo(pCreateInfo);

    pDest->pDevice = (VkDevice*)malloc(sizeof(VkDevice));
    *pDest->pDevice = *pDevice;
}

void vktrace_vk_snapshot_destroy_createdevice_params(VKTRACE_VK_SNAPSHOT_CREATEDEVICE_PARAMS* pSrc)
{
    memset(&pSrc->physicalDevice, 0, sizeof(VkPhysicalDevice));

    vktrace_deepfree_VkDeviceCreateInfo(pSrc->pCreateInfo);
    pSrc->pCreateInfo = NULL;

    free(pSrc->pDevice);
    pSrc->pDevice = NULL;
}



// add a new node to the global and object lists, then return it so the caller can populate the object information.
static VKTRACE_VK_SNAPSHOT_LL_NODE* snapshot_insert_object(VKTRACE_VK_SNAPSHOT* pSnapshot, VkObject object, VkObjectType type)
{
    // Create a new node
    VKTRACE_VK_SNAPSHOT_LL_NODE* pNewObjNode = (VKTRACE_VK_SNAPSHOT_LL_NODE*)malloc(sizeof(VKTRACE_VK_SNAPSHOT_LL_NODE));
    memset(pNewObjNode, 0, sizeof(VKTRACE_VK_SNAPSHOT_LL_NODE));
    pNewObjNode->obj.object = object;
    pNewObjNode->obj.objType = type;
    pNewObjNode->obj.status = OBJSTATUS_NONE;

    // insert at front of global list
    pNewObjNode->pNextGlobal = pSnapshot->pGlobalObjs;
    pSnapshot->pGlobalObjs = pNewObjNode;

    // insert at front of object list
    pNewObjNode->pNextObj = pSnapshot->pObjectHead[type];
    pSnapshot->pObjectHead[type] = pNewObjNode;

    // increment count
    pSnapshot->globalObjCount++;
    pSnapshot->numObjs[type]++;

    return pNewObjNode;
}

// This is just a helper function to snapshot_remove_object(..). It is not intended for this to be called directly.
static void snapshot_remove_obj_type(VKTRACE_VK_SNAPSHOT* pSnapshot, VkObject object, VkObjectType objType) {
    VKTRACE_VK_SNAPSHOT_LL_NODE *pTrav = pSnapshot->pObjectHead[objType];
    VKTRACE_VK_SNAPSHOT_LL_NODE *pPrev = pSnapshot->pObjectHead[objType];
    while (pTrav) {
        if (pTrav->obj.object == object) {
            pPrev->pNextObj = pTrav->pNextObj;
            // update HEAD of Obj list as needed
            if (pSnapshot->pObjectHead[objType] == pTrav)
            {
                pSnapshot->pObjectHead[objType] = pTrav->pNextObj;
            }
            assert(pSnapshot->numObjs[objType] > 0);
            pSnapshot->numObjs[objType]--;
            return;
        }
        pPrev = pTrav;
        pTrav = pTrav->pNextObj;
    }
    char str[1024];
    sprintf(str, "OBJ INTERNAL ERROR : Obj %p was in global list but not in %s list", (void*)object, string_VK_OBJECT_TYPE(objType));
    layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, object, 0, VKTRACESNAPSHOT_INTERNAL_ERROR, LAYER_ABBREV_STR, str);
}

// Search global list to find object,
// if found:
// remove object from obj_type list using snapshot_remove_obj_type()
// remove object from global list,
// return object.
// else:
// Report message that we didn't see it get created,
// return NULL.
static VKTRACE_VK_SNAPSHOT_LL_NODE* snapshot_remove_object(VKTRACE_VK_SNAPSHOT* pSnapshot, VkObject object)
{
    VKTRACE_VK_SNAPSHOT_LL_NODE *pTrav = pSnapshot->pGlobalObjs;
    VKTRACE_VK_SNAPSHOT_LL_NODE *pPrev = pSnapshot->pGlobalObjs;
    while (pTrav)
    {
        if (pTrav->obj.object == object)
        {
            snapshot_remove_obj_type(pSnapshot, object, pTrav->obj.objType);
            pPrev->pNextGlobal = pTrav->pNextGlobal;
            // update HEAD of global list if needed
            if (pSnapshot->pGlobalObjs == pTrav)
            {
                pSnapshot->pGlobalObjs = pTrav->pNextGlobal;
            }
            assert(pSnapshot->globalObjCount > 0);
            pSnapshot->globalObjCount--;
            return pTrav;
        }
        pPrev = pTrav;
        pTrav = pTrav->pNextGlobal;
    }

    // Object not found.
    char str[1024];
    sprintf(str, "Object %p was not found in the created object list. It should be added as a deleted object.", (void*)object);
    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, object, 0, VKTRACESNAPSHOT_UNKNOWN_OBJECT, LAYER_ABBREV_STR, str);
    return NULL;
}

// Add a new deleted object node to the list
static void snapshot_insert_deleted_object(VKTRACE_VK_SNAPSHOT* pSnapshot, VkObject object, VkObjectType type)
{
    // Create a new node
    VKTRACE_VK_SNAPSHOT_DELETED_OBJ_NODE* pNewObjNode = (VKTRACE_VK_SNAPSHOT_DELETED_OBJ_NODE*)malloc(sizeof(VKTRACE_VK_SNAPSHOT_DELETED_OBJ_NODE));
    memset(pNewObjNode, 0, sizeof(VKTRACE_VK_SNAPSHOT_DELETED_OBJ_NODE));
    pNewObjNode->objType = type;
    pNewObjNode->object = object;

    // insert at front of list
    pNewObjNode->pNextObj = pSnapshot->pDeltaDeletedObjects;
    pSnapshot->pDeltaDeletedObjects = pNewObjNode;

    // increment count
    pSnapshot->deltaDeletedObjectCount++;
}

// Note: the parameters after pSnapshot match the order of vkCreateDevice(..)
static void snapshot_insert_device(VKTRACE_VK_SNAPSHOT* pSnapshot, VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(pSnapshot, *pDevice, VK_OBJECT_TYPE_DEVICE);
    pNode->obj.pStruct = malloc(sizeof(VKTRACE_VK_SNAPSHOT_DEVICE_NODE));

    VKTRACE_VK_SNAPSHOT_DEVICE_NODE* pDevNode = (VKTRACE_VK_SNAPSHOT_DEVICE_NODE*)pNode->obj.pStruct;
    vktrace_vk_snapshot_copy_createdevice_params(&pDevNode->params, physicalDevice, pCreateInfo, pDevice);

    // insert at front of device list
    pNode->pNextObj = pSnapshot->pDevices;
    pSnapshot->pDevices = pNode;

    // increment count
    pSnapshot->deviceCount++;
}

static void snapshot_remove_device(VKTRACE_VK_SNAPSHOT* pSnapshot, VkDevice device)
{
    VKTRACE_VK_SNAPSHOT_LL_NODE* pFoundObject = snapshot_remove_object(pSnapshot, device);

    if (pFoundObject != NULL)
    {
        VKTRACE_VK_SNAPSHOT_LL_NODE *pTrav = pSnapshot->pDevices;
        VKTRACE_VK_SNAPSHOT_LL_NODE *pPrev = pSnapshot->pDevices;
        while (pTrav != NULL)
        {
            if (pTrav->obj.object == device)
            {
                pPrev->pNextObj = pTrav->pNextObj;
                // update HEAD of Obj list as needed
                if (pSnapshot->pDevices == pTrav)
                    pSnapshot->pDevices = pTrav->pNextObj;

                // delete the object
                if (pTrav->obj.pStruct != NULL)
                {
                    VKTRACE_VK_SNAPSHOT_DEVICE_NODE* pDevNode = (VKTRACE_VK_SNAPSHOT_DEVICE_NODE*)pTrav->obj.pStruct;
                    vktrace_vk_snapshot_destroy_createdevice_params(&pDevNode->params);
                    free(pDevNode);
                }
                free(pTrav);

                if (pSnapshot->deviceCount > 0)
                {
                    pSnapshot->deviceCount--;
                }
                else
                {
                    // TODO: Callback WARNING that too many devices were deleted
                    assert(!"DeviceCount <= 0 means that too many devices were deleted.");
                }
                return;
            }
            pPrev = pTrav;
            pTrav = pTrav->pNextObj;
        }
    }

    // If the code got here, then the device wasn't in the devices list.
    // That means we should add this device to the deleted items list.
    snapshot_insert_deleted_object(&s_delta, device, VK_OBJECT_TYPE_DEVICE);
}

// Traverse global list and return type for given object
static VkObjectType ll_get_obj_type(VkObject object) {
    VKTRACE_VK_SNAPSHOT_LL_NODE *pTrav = s_delta.pGlobalObjs;
    while (pTrav) {
        if (pTrav->obj.object == object)
            return pTrav->obj.objType;
        pTrav = pTrav->pNextGlobal;
    }
    char str[1024];
    sprintf(str, "Attempting look-up on obj %p but it is NOT in the global list!", (void*)object);
    layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, object, 0, VKTRACESNAPSHOT_MISSING_OBJECT, LAYER_ABBREV_STR, str);
    return (VkObjectType)-1;
}

static void ll_increment_use_count(VkObject object, VkObjectType objType) {
    VKTRACE_VK_SNAPSHOT_LL_NODE *pTrav = s_delta.pObjectHead[objType];
    while (pTrav) {
        if (pTrav->obj.object == object) {
            pTrav->obj.numUses++;
            return;
        }
        pTrav = pTrav->pNextObj;
    }

    // If we do not find obj, insert it and then increment count
    // TODO: we can't just create the object, because we don't know what it was created with.
    // Instead, we need to make a list of referenced objects. When the delta is merged with a snapshot, we'll need
    // to confirm that the referenced objects actually exist in the snapshot; otherwise I guess the merge should fail.
    char str[1024];
    sprintf(str, "Unable to increment count for obj %p, will add to list as %s type and increment count", (void*)object, string_VK_OBJECT_TYPE(objType));
    layerCbMsg(VK_DBG_MSG_WARNING, VK_VALIDATION_LEVEL_0, object, 0, VKTRACESNAPSHOT_UNKNOWN_OBJECT, LAYER_ABBREV_STR, str);

//    ll_insert_obj(pObj, objType);
//    ll_increment_use_count(pObj, objType);
}

// Set selected flag state for an object node
static void set_status(VkObject object, VkObjectType objType, OBJECT_STATUS status_flag) {
    if ((void*)object != NULL) {
        VKTRACE_VK_SNAPSHOT_LL_NODE *pTrav = s_delta.pObjectHead[objType];
        while (pTrav) {
            if (pTrav->obj.object == object) {
                pTrav->obj.status |= status_flag;
                return;
            }
            pTrav = pTrav->pNextObj;
        }

        // If we do not find it print an error
        char str[1024];
        sprintf(str, "Unable to set status for non-existent object %p of %s type", (void*)object, string_VK_OBJECT_TYPE(objType));
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, object, 0, VKTRACESNAPSHOT_UNKNOWN_OBJECT, LAYER_ABBREV_STR, str);
    }
}

// Track selected state for an object node
static void track_object_status(VkObject object, VkStateBindPoint stateBindPoint) {
    VKTRACE_VK_SNAPSHOT_LL_NODE *pTrav = s_delta.pObjectHead[VK_OBJECT_TYPE_COMMAND_BUFFER];

    while (pTrav) {
        if (pTrav->obj.object == object) {
            return;
        }
        pTrav = pTrav->pNextObj;
    }

    // If we do not find it print an error
    char str[1024];
    sprintf(str, "Unable to track status for non-existent Command Buffer object %p", (void*)object);
    layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, object, 0, VKTRACESNAPSHOT_UNKNOWN_OBJECT, LAYER_ABBREV_STR, str);
}

// Reset selected flag state for an object node
static void reset_status(VkObject object, VkObjectType objType, OBJECT_STATUS status_flag) {
    VKTRACE_VK_SNAPSHOT_LL_NODE *pTrav = s_delta.pObjectHead[objType];
    while (pTrav) {
        if (pTrav->obj.object == object) {
            pTrav->obj.status &= ~status_flag;
            return;
        }
        pTrav = pTrav->pNextObj;
    }

    // If we do not find it print an error
    char str[1024];
    sprintf(str, "Unable to reset status for non-existent object %p of %s type", (void*)object, string_VK_OBJECT_TYPE(objType));
    layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, object, 0, VKTRACESNAPSHOT_UNKNOWN_OBJECT, LAYER_ABBREV_STR, str);
}

#include "vk_dispatch_table_helper.h"
static void initVktraceSnapshot(void)
{
    const char *strOpt;
    // initialize VktraceSnapshot options
    getLayerOptionEnum(LAYER_NAME_STR "ReportLevel", (uint32_t *) &g_reportingLevel);
    g_actionIsDefault = getLayerOptionEnum(LAYER_NAME_STR "DebugAction", (uint32_t *) &g_debugAction);

    if (g_debugAction & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        strOpt = getLayerOption(LAYER_NAME_STR "LogFilename");
        if (strOpt)
        {
            g_logFile = fopen(strOpt, "w");
        }
        if (g_logFile == NULL)
            g_logFile = stdout;
    }

    PFN_vkGetProcAddr fpNextGPA;
    fpNextGPA = pCurObj->pGPA;
    assert(fpNextGPA);

    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (VkPhysicalDevice) pCurObj->nextObject);
    if (!objLockInitialized)
    {
        // TODO/TBD: Need to delete this mutex sometime.  How???
        loader_platform_thread_create_mutex(&objLock);
        objLockInitialized = 1;
    }
}

//=============================================================================
// vulkan entrypoints
//=============================================================================
VK_LAYER_EXPORT VkResult VKAPI vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, VkInstance* pInstance)
{
    VkResult result = nextTable.CreateInstance(pCreateInfo, pInstance);
    loader_platform_thread_lock_mutex(&objLock);
    snapshot_insert_object(&s_delta, *pInstance, VK_OBJECT_TYPE_INSTANCE);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkDestroyInstance(VkInstance instance)
{
    nextTable.DestroyInstance(instance);
    loader_platform_thread_lock_mutex(&objLock);
    snapshot_remove_object(&s_delta, instance);
    loader_platform_thread_unlock_mutex(&objLock);
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(instance, VK_OBJECT_TYPE_INSTANCE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceInfo(VkPhysicalDevice gpu, VkPhysicalDeviceInfoType infoType, size_t* pDataSize, void* pData)
{
    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initVktraceSnapshot);
    VkResult result = nextTable.GetPhysicalDeviceInfo((VkPhysicalDevice)gpuw->nextObject, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initVktraceSnapshot);
    VkResult result = nextTable.CreateDevice((VkPhysicalDevice)gpuw->nextObject, pCreateInfo, pDevice);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        snapshot_insert_device(&s_delta, gpu, pCreateInfo, pDevice);
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkDestroyDevice(VkDevice device)
{
    nextTable.DestroyDevice(device);
    loader_platform_thread_lock_mutex(&objLock);
    snapshot_remove_device(&s_delta, device);
    loader_platform_thread_unlock_mutex(&objLock);

    // Report any remaining objects in LL
    VKTRACE_VK_SNAPSHOT_LL_NODE *pTrav = s_delta.pGlobalObjs;
    while (pTrav != NULL)
    {
//        if (pTrav->obj.objType == VK_OBJECT_TYPE_SWAP_CHAIN_IMAGE_WSI ||
//            pTrav->obj.objType == VK_OBJECT_TYPE_SWAP_CHAIN_MEMORY_WSI)
//        {
//            VKTRACE_VK_SNAPSHOT_LL_NODE *pDel = pTrav;
//            pTrav = pTrav->pNextGlobal;
//            snapshot_remove_object(&s_delta, (void*)(pDel->obj.pVkObject));
//        } else {
            char str[1024];
            sprintf(str, "OBJ ERROR : %s object %p has not been destroyed (was used %lu times).", string_VK_OBJECT_TYPE(pTrav->obj.objType), (void*)pTrav->obj.object, pTrav->obj.numUses);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, device, 0, VKTRACESNAPSHOT_OBJECT_LEAK, LAYER_ABBREV_STR, str);
            pTrav = pTrav->pNextGlobal;
//        }
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateLayers(VkPhysicalDevice physicalDevice, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved)
{
    if ((void*)physicalDevice != NULL)
    {
        VkBaseLayerObject* gpuw = (VkBaseLayerObject *) physicalDevice;
        loader_platform_thread_lock_mutex(&objLock);
        ll_increment_use_count(physicalDevice, VK_OBJECT_TYPE_PHYSICAL_DEVICE);
        loader_platform_thread_unlock_mutex(&objLock);
        pCurObj = gpuw;
        loader_platform_thread_once(&tabOnce, initVktraceSnapshot);
        VkResult result = nextTable.EnumerateLayers((VkPhysicalDevice)gpuw->nextObject, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
        return result;
    } else {
        if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)
        {
            return VK_ERROR_INVALID_POINTER;
        }
        // This layer compatible with all devices
        *pOutLayerCount = 1;
        strncpy((char *) pOutLayers[0], LAYER_NAME_STR, maxStringSize);
        return VK_SUCCESS;
    }
}
struct extProps {
    uint32_t version;
    const char * const name;
};

#define VKTRACE_SNAPSHOT_LAYER_EXT_ARRAY_SIZE 1
static const struct extProps mtExts[VKTRACE_SNAPSHOT_LAYER_EXT_ARRAY_SIZE] = {
    // TODO what is the version?
{ 0x10, LAYER_NAME_STR }
};

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionInfo(
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData)
{
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
            *count = VKTRACE_SNAPSHOT_LAYER_EXT_ARRAY_SIZE;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            if (extensionIndex >= VKTRACE_SNAPSHOT_LAYER_EXT_ARRAY_SIZE)
                return VK_ERROR_INVALID_VALUE;
            ext_props = (VkExtensionProperties *) pData;
            ext_props->version = mtExts[extensionIndex].version;
            strncpy(ext_props->extensionName, mtExts[extensionIndex].name,
                                        VK_MAX_EXTENSION_NAME_SIZE);
            ext_props->extensionName[VK_MAX_EXTENSION_NAME_SIZE - 1] = '\0';
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}

VK_LAYER_EXPORT void VKAPI vkGetDeviceQueue(VkDevice device, uint32_t queueNodeIndex, uint32_t queueIndex, VkQueue* pQueue)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueSubmit(VkQueue queue, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers, VkFence fence)
{
    set_status(fence, VK_OBJECT_TYPE_FENCE, OBJSTATUS_FENCE_IS_SUBMITTED);
    VkResult result = nextTable.QueueSubmit(queue, commandBufferCount, pCommandBuffers, fence);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueWaitIdle(VkQueue queue)
{
    VkResult result = nextTable.QueueWaitIdle(queue);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDeviceWaitIdle(VkDevice device)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.DeviceWaitIdle(device);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo, VkDeviceMemory* pMemory)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.AllocateMemory(device, pAllocateInfo, pMemory);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pMemory, VK_OBJECT_TYPE_DEVICE_MEMORY);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkFreeMemory(VkDevice device, VkDeviceMemory mem)
{
    nextTable.FreeMemory(device, mem);
    loader_platform_thread_lock_mutex(&objLock);
    snapshot_remove_object(&s_delta, mem);
    loader_platform_thread_unlock_mutex(&objLock);
}

VK_LAYER_EXPORT VkResult VKAPI vkSetMemoryPriority(VkDevice device, VkDeviceMemory mem, VkMemoryPriority priority)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(mem, VK_OBJECT_TYPE_DEVICE_MEMORY);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.SetMemoryPriority(device, mem, priority);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkMapMemory(VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(mem, VK_OBJECT_TYPE_DEVICE_MEMORY);
    loader_platform_thread_unlock_mutex(&objLock);
    set_status(mem, VK_OBJECT_TYPE_DEVICE_MEMORY, OBJSTATUS_GPU_MEM_MAPPED);
    VkResult result = nextTable.MapMemory(device, mem, offset, size, flags, ppData);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkUnmapMemory(VkDevice device, VkDeviceMemory mem)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(mem, VK_OBJECT_TYPE_DEVICE_MEMORY);
    loader_platform_thread_unlock_mutex(&objLock);
    reset_status(mem, VK_OBJECT_TYPE_DEVICE_MEMORY, OBJSTATUS_GPU_MEM_MAPPED);
    nextTable.UnmapMemory(device, mem);
}

VK_LAYER_EXPORT VkResult VKAPI vkPinSystemMemory(VkDevice device, const void* pSysMem, size_t memSize, VkDeviceMemory* pMemory)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.PinSystemMemory(device, pSysMem, memSize, pMemory);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetMultiDeviceCompatibility(VkPhysicalDevice physicalDevice0, VkPhysicalDevice physicalDevice1, VkPhysicalDeviceCompatibilityInfo* pInfo)
{
    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) physicalDevice0;
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(physicalDevice0, VK_OBJECT_TYPE_PHYSICAL_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initVktraceSnapshot);
    VkResult result = nextTable.GetMultiDeviceCompatibility((VkPhysicalDevice)gpuw->nextObject, physicalDevice1, pInfo);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenSharedMemory(VkDevice device, const VkMemoryOpenInfo* pOpenInfo, VkDeviceMemory* pMemory)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.OpenSharedMemory(device, pOpenInfo, pMemory);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenSharedSemaphore(VkDevice device, const VkSemaphoreOpenInfo* pOpenInfo, VkSemaphore* pSemaphore)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.OpenSharedSemaphore(device, pOpenInfo, pSemaphore);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenPeerMemory(VkDevice device, const VkPeerMemoryOpenInfo* pOpenInfo, VkDeviceMemory* pMemory)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.OpenPeerMemory(device, pOpenInfo, pMemory);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenPeerImage(VkDevice device, const VkPeerImageOpenInfo* pOpenInfo, VkImage* pImage, VkDeviceMemory* pMemory)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.OpenPeerImage(device, pOpenInfo, pImage, pMemory);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkDestroyObject(VkDevice device, VkObjectType objType, VkObject object)
{
    nextTable.DestroyObject(device, objType, object);
    loader_platform_thread_lock_mutex(&objLock);
    snapshot_remove_object(&s_delta, object);
    loader_platform_thread_unlock_mutex(&objLock);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetObjectInfo(VkDevice device, VkObjectType objType, VkObject object, VkObjectInfoType infoType, size_t* pDataSize, void* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(object, ll_get_obj_type(object));
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.GetObjectInfo(device, objType, object, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindObjectMemory(VkQueue queue, VkObjectType objType, VkObject object, uint32_t allocationIdx, VkDeviceMemory mem, VkDeviceSize offset)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(object, ll_get_obj_type(object));
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.QueueBindObjectMemory(queue, objType, object, allocationIdx, mem, offset);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindObjectMemoryRange(VkQueue queue, VkObjectType objType, VkObject object, uint32_t allocationIdx, VkDeviceSize rangeOffset, VkDeviceSize rangeSize, VkDeviceMemory mem, VkDeviceSize memoryOffset)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(object, ll_get_obj_type(object));
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.QueueBindObjectMemoryRange(queue, objType, object, allocationIdx, rangeOffset, rangeSize, mem, memoryOffset);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindImageMemoryRange(VkQueue queue, VkImage image, uint32_t allocationIdx, const VkImageMemoryBindInfo* pBindInfo, VkDeviceMemory mem, VkDeviceSize memoryOffset)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(image, VK_OBJECT_TYPE_IMAGE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.QueueBindImageMemoryRange(queue, image, allocationIdx, pBindInfo, mem, memoryOffset);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, VkFence* pFence)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateFence(device, pCreateInfo, pFence);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pFence, VK_OBJECT_TYPE_FENCE);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetFenceStatus(VkDevice device, VkFence fence)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(fence, VK_OBJECT_TYPE_FENCE);
    loader_platform_thread_unlock_mutex(&objLock);
    // Warn if submitted_flag is not set
    VkResult result = nextTable.GetFenceStatus(device, fence);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, bool32_t waitAll, uint64_t timeout)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, VkSemaphore* pSemaphore)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateSemaphore(device, pCreateInfo, pSemaphore);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pSemaphore, VK_OBJECT_TYPE_SEMAPHORE);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, VkEvent* pEvent)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateEvent(device, pCreateInfo, pEvent);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pEvent, VK_OBJECT_TYPE_EVENT);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetEventStatus(VkDevice device, VkEvent event)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(event, VK_OBJECT_TYPE_EVENT);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.GetEventStatus(device, event);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkSetEvent(VkDevice device, VkEvent event)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(event, VK_OBJECT_TYPE_EVENT);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.SetEvent(device, event);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetEvent(VkDevice device, VkEvent event)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(event, VK_OBJECT_TYPE_EVENT);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.ResetEvent(device, event);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, VkQueryPool* pQueryPool)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateQueryPool(device, pCreateInfo, pQueryPool);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pQueryPool, VK_OBJECT_TYPE_QUERY_POOL);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount, size_t* pDataSize, void* pData, VkQueryResultFlags flags)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(queryPool, VK_OBJECT_TYPE_QUERY_POOL);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.GetQueryPoolResults(device, queryPool, startQuery, queryCount, pDataSize, pData, flags);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetFormatInfo(VkDevice device, VkFormat format, VkFormatInfoType infoType, size_t* pDataSize, void* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.GetFormatInfo(device, format, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, VkBuffer* pBuffer)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateBuffer(device, pCreateInfo, pBuffer);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pBuffer, VK_OBJECT_TYPE_BUFFER);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, VkBufferView* pView)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateBufferView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pView, VK_OBJECT_TYPE_BUFFER_VIEW);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, VkImage* pImage)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateImage(device, pCreateInfo, pImage);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pImage, VK_OBJECT_TYPE_IMAGE);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetImageSubresourceInfo(VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceInfoType infoType, size_t* pDataSize, void* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(image, VK_OBJECT_TYPE_IMAGE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.GetImageSubresourceInfo(device, image, pSubresource, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, VkImageView* pView)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateImageView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pView, VK_OBJECT_TYPE_IMAGE_VIEW);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateColorAttachmentView(VkDevice device, const VkColorAttachmentViewCreateInfo* pCreateInfo, VkColorAttachmentView* pView)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateColorAttachmentView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pView, VK_OBJECT_TYPE_COLOR_ATTACHMENT_VIEW);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDepthStencilView(VkDevice device, const VkDepthStencilViewCreateInfo* pCreateInfo, VkDepthStencilView* pView)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateDepthStencilView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pView, VK_OBJECT_TYPE_DEPTH_STENCIL_VIEW);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipeline(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pPipeline, VK_OBJECT_TYPE_PIPELINE);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateComputePipeline(VkDevice device, const VkComputePipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateComputePipeline(device, pCreateInfo, pPipeline);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pPipeline, VK_OBJECT_TYPE_PIPELINE);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkStorePipeline(VkDevice device, VkPipeline pipeline, size_t* pDataSize, void* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(pipeline, VK_OBJECT_TYPE_PIPELINE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.StorePipeline(device, pipeline, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkLoadPipeline(VkDevice device, size_t dataSize, const void* pData, VkPipeline* pPipeline)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.LoadPipeline(device, dataSize, pData, pPipeline);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, VkSampler* pSampler)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateSampler(device, pCreateInfo, pSampler);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pSampler, VK_OBJECT_TYPE_SAMPLER);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorSetLayout( VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayout* pSetLayout)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateDescriptorSetLayout(device, pCreateInfo, pSetLayout);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBeginDescriptorPoolUpdate(VkDevice device, VkDescriptorUpdateMode updateMode)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.BeginDescriptorPoolUpdate(device, updateMode);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkEndDescriptorPoolUpdate(VkDevice device, VkCommandBuffer cmd)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.EndDescriptorPoolUpdate(device, cmd);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorPool(VkDevice device, VkDescriptorPoolUsage poolUsage, uint32_t maxSets, const VkDescriptorPoolCreateInfo* pCreateInfo, VkDescriptorPool* pDescriptorPool)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateDescriptorPool(device, poolUsage, maxSets, pCreateInfo, pDescriptorPool);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.ResetDescriptorPool(device, descriptorPool);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocateDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetUsage setUsage, uint32_t count, const VkDescriptorSetLayout* pSetLayouts, VkDescriptorSet* pDescriptorSets, uint32_t* pCount)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.AllocateDescriptorSets(device, descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets, pCount);
    if (result == VK_SUCCESS)
    {
        for (uint32_t i = 0; i < *pCount; i++) {
            loader_platform_thread_lock_mutex(&objLock);
            VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, pDescriptorSets[i], VK_OBJECT_TYPE_DESCRIPTOR_SET);
            pNode->obj.pStruct = NULL;
            loader_platform_thread_unlock_mutex(&objLock);
        }
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkClearDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t count, const VkDescriptorSet* pDescriptorSets)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.ClearDescriptorSets(device, descriptorPool, count, pDescriptorSets);
}

VK_LAYER_EXPORT void VKAPI vkUpdateDescriptors(VkDevice device, VkDescriptorSet descriptorSet, uint32_t updateCount, const void** ppUpdateArray)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(descriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.UpdateDescriptors(device, descriptorSet, updateCount, ppUpdateArray);
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pCreateInfo, VkCommandBuffer* pCommandBuffer)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.AllocateCommandBuffers(device, pCreateInfo, pCommandBuffer);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.BeginCommandBuffer(commandBuffer, pBeginInfo);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.EndCommandBuffer(commandBuffer);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetCommandBuffer(VkCommandBuffer commandBuffer)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.ResetCommandBuffer(commandBuffer);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t firstSet, uint32_t setCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdBindDescriptorSets(commandBuffer, pipelineBindPoint, firstSet, setCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindVertexBuffers(
    VkCommandBuffer                                 commandBuffer,
    uint32_t                                    startBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                         pOffsets)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdBindVertexBuffers(commandBuffer, startBinding, bindingCount, pBuffers, pOffsets);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

VK_LAYER_EXPORT void VKAPI vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdDraw(commandBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdDrawIndirect(commandBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdDrawIndexedIndirect(commandBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdDispatch(commandBuffer, x, y, z);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdDispatchIndirect(commandBuffer, buffer, offset);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCloneImageData(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdCloneImageData(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout);
}

VK_LAYER_EXPORT void VKAPI vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const uint32_t* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, VkClearColor color, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdClearColorImage(commandBuffer, image, imageLayout, color, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencil(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, float depth, uint32_t stencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdClearDepthStencil(commandBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t rectCount, const VkImageResolve* pRects)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, rectCount, pRects);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipeEvent pipeEvent)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdSetEvent(commandBuffer, event, pipeEvent);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipeEvent pipeEvent)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdResetEvent(commandBuffer, event, pipeEvent);
}

VK_LAYER_EXPORT void VKAPI vkCmdWaitEvents(
        VkCommandBuffer                                 commandBuffer,
        VkWaitEvent                                 waitEvent,
        uint32_t                                    eventCount,
        const VkEvent*                              pEvents,
        uint32_t                                    memoryBarrierCount,
        const void**                                ppMemoryBarriers)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdWaitEvents(commandBuffer, waitEvent, eventCount, pEvents, memoryBarrierCount, ppMemoryBarriers);
}

VK_LAYER_EXPORT void VKAPI vkCmdPipelineBarrier(
        VkCommandBuffer                                 commandBuffer,
        VkWaitEvent                                 waitEvent,
        uint32_t                                    pipeEventCount,
        const VkPipeEvent*                          pPipeEvents,
        uint32_t                                    memoryBarrierCount,
        const void**                                ppMemoryBarriers)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdPipelineBarrier(commandBuffer, waitEvent, pipeEventCount, pPipeEvents, memoryBarrierCount, ppMemoryBarriers);
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot, VkFlags flags)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdBeginQuery(commandBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdEndQuery(commandBuffer, queryPool, slot);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdResetQueryPool(commandBuffer, queryPool, startQuery, queryCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t slot)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, slot);
}

VK_LAYER_EXPORT void VKAPI vkCmdInitAtomicCounters(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, const uint32_t* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdInitAtomicCounters(commandBuffer, pipelineBindPoint, startCounter, counterCount, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdLoadAtomicCounters(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VkBuffer srcBuffer, VkDeviceSize srcOffset)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdLoadAtomicCounters(commandBuffer, pipelineBindPoint, startCounter, counterCount, srcBuffer, srcOffset);
}

VK_LAYER_EXPORT void VKAPI vkCmdSaveAtomicCounters(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VkBuffer dstBuffer, VkDeviceSize dstOffset)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdSaveAtomicCounters(commandBuffer, pipelineBindPoint, startCounter, counterCount, dstBuffer, dstOffset);
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, VkFramebuffer* pFramebuffer)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateFramebuffer(device, pCreateInfo, pFramebuffer);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pFramebuffer, VK_OBJECT_TYPE_FRAMEBUFFER);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, VkRenderPass* pRenderPass)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateRenderPass(device, pCreateInfo, pRenderPass);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pRenderPass, VK_OBJECT_TYPE_RENDER_PASS);
        pNode->obj.pStruct = NULL;
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBegin *pRenderPassBegin)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdBeginRenderPass(commandBuffer, pRenderPassBegin);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdEndRenderPass(commandBuffer, renderPass);
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgSetValidationLevel(VkDevice device, VkValidationLevel validationLevel)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.DbgSetValidationLevel(device, validationLevel);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgRegisterMsgCallback(VkInstance instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData)
{
    // This layer intercepts callbacks
    VK_LAYER_DBG_FUNCTION_NODE *pNewDbgFuncNode = (VK_LAYER_DBG_FUNCTION_NODE*)malloc(sizeof(VK_LAYER_DBG_FUNCTION_NODE));
    if (pNewDbgFuncNode  == NULL)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;
    pNewDbgFuncNode->pUserData = pUserData;
    pNewDbgFuncNode->pNext = g_pDbgFunctionHead;
    g_pDbgFunctionHead = pNewDbgFuncNode;
    // force callbacks if DebugAction hasn't been set already other than initial value
    if (g_actionIsDefault) {
        g_debugAction = VK_DBG_LAYER_ACTION_CALLBACK;
    }    VkResult result = nextTable.DbgRegisterMsgCallback(instance, pfnMsgCallback, pUserData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgUnregisterMsgCallback(VkInstance instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
{
    VK_LAYER_DBG_FUNCTION_NODE *pTrav = g_pDbgFunctionHead;
    VK_LAYER_DBG_FUNCTION_NODE *pPrev = pTrav;
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
            g_debugAction = VK_DBG_LAYER_ACTION_LOG_MSG;
        else
            g_debugAction &= ~VK_DBG_LAYER_ACTION_CALLBACK;
    }
    VkResult result = nextTable.DbgUnregisterMsgCallback(instance, pfnMsgCallback);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgSetMessageFilter(VkDevice device, int32_t msgCode, VK_DBG_MSG_FILTER filter)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.DbgSetMessageFilter(device, msgCode, filter);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgSetObjectTag(VkDevice device, VkObject object, size_t tagSize, const void* pTag)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(object, ll_get_obj_type(object));
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.DbgSetObjectTag(device, object, tagSize, pTag);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgSetGlobalOption(VkInstance instance, VK_DBG_GLOBAL_OPTION dbgOption, size_t dataSize, const void* pData)
{
    VkResult result = nextTable.DbgSetGlobalOption(instance, dbgOption, dataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgSetDeviceOption(VkDevice device, VK_DBG_DEVICE_OPTION dbgOption, size_t dataSize, const void* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.DbgSetDeviceOption(device, dbgOption, dataSize, pData);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerBegin(VkCommandBuffer commandBuffer, const char* pMarker)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdDbgMarkerBegin(commandBuffer, pMarker);
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerEnd(VkCommandBuffer commandBuffer)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    loader_platform_thread_unlock_mutex(&objLock);
    nextTable.CmdDbgMarkerEnd(commandBuffer);
}

VK_LAYER_EXPORT VkResult VKAPI xglGetDisplayInfoWSI(VkDisplayWSI display, VkDisplayInfoTypeWSI infoType, size_t* pDataSize, void* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(display, VK_OBJECT_TYPE_DISPLAY_WSI);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.GetDisplayInfoWSI(display, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI xglCreateSwapChainWSI(VkDevice device, const VkSwapChainCreateInfoWSI* pCreateInfo, VkSwapChainWSI* pSwapChain)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(device, VK_OBJECT_TYPE_DEVICE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.CreateSwapChainWSI(device, pCreateInfo, pSwapChain);
    if (result == VK_SUCCESS)
    {
        loader_platform_thread_lock_mutex(&objLock);

#if 0
        VKTRACE_VK_SNAPSHOT_LL_NODE* pNode = snapshot_insert_object(&s_delta, *pImage, VK_OBJECT_TYPE_IMAGE);
        pNode->obj.pStruct = NULL;

        VKTRACE_VK_SNAPSHOT_LL_NODE* pMemNode = snapshot_insert_object(&s_delta, *pMemory, VK_OBJECT_TYPE_PRESENTABLE_IMAGE_MEMORY);
        pMemNode->obj.pStruct = NULL;
#else
        snapshot_insert_object(&s_delta, *pSwapChain, VK_OBJECT_TYPE_SWAP_CHAIN_WSI);
#endif

        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;

}

VK_LAYER_EXPORT VkResult VKAPI xglDestroySwapChainWSI(VkSwapChainWSI swapChain)
{
    VkResult result = nextTable.DestroySwapChainWSI(swapChain);
    loader_platform_thread_lock_mutex(&objLock);
    snapshot_remove_object(&s_delta, swapChain);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI xglGetSwapChainInfoWSI(VkSwapChainWSI swapChain, VkSwapChainInfoTypeWSI infoType, size_t* pDataSize, void* pData)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(swapChain, VK_OBJECT_TYPE_SWAP_CHAIN_WSI);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.GetSwapChainInfoWSI(swapChain, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI xglQueuePresentWSI(VkQueue queue, const VkPresentInfoWSI* pPresentInfo)
{
    loader_platform_thread_lock_mutex(&objLock);
    ll_increment_use_count(queue, VK_OBJECT_TYPE_QUEUE);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = nextTable.QueuePresentWSI(queue, pPresentInfo);
    return result;
}

//=================================================================================================
// Exported methods
//=================================================================================================
void vktraceSnapshotStartTracking(void)
{
    assert(!"Not Implemented");
}

//=================================================================================================
VKTRACE_VK_SNAPSHOT vktraceSnapshotGetDelta(void)
{
    // copy the delta by merging it into an empty snapshot
    VKTRACE_VK_SNAPSHOT empty;
    memset(&empty, 0, sizeof(VKTRACE_VK_SNAPSHOT));

    return vktraceSnapshotMerge(&s_delta, &empty);
}

//=================================================================================================
VKTRACE_VK_SNAPSHOT vktraceSnapshotGetSnapshot(void)
{
    // copy the master snapshot by merging it into an empty snapshot
    VKTRACE_VK_SNAPSHOT empty;
    memset(&empty, 0, sizeof(VKTRACE_VK_SNAPSHOT));

    return vktraceSnapshotMerge(&s_snapshot, &empty);
}

//=================================================================================================
void vktraceSnapshotPrintDelta()
{
    char str[2048];
    VKTRACE_VK_SNAPSHOT_LL_NODE* pTrav = s_delta.pGlobalObjs;
    sprintf(str, "==== DELTA SNAPSHOT contains %lu objects, %lu devices, and %lu deleted objects", s_delta.globalObjCount, s_delta.deviceCount, s_delta.deltaDeletedObjectCount);
    layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, (VkObject)NULL, 0, VKTRACESNAPSHOT_SNAPSHOT_DATA, LAYER_ABBREV_STR, str);

    // print all objects
    if (s_delta.globalObjCount > 0)
    {
        sprintf(str, "======== DELTA SNAPSHOT Created Objects:");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, pTrav->obj.object, 0, VKTRACESNAPSHOT_SNAPSHOT_DATA, LAYER_ABBREV_STR, str);
        while (pTrav != NULL)
        {
            sprintf(str, "\t%s obj %p", string_VK_OBJECT_TYPE(pTrav->obj.objType), (void*)pTrav->obj.object);
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, pTrav->obj.object, 0, VKTRACESNAPSHOT_SNAPSHOT_DATA, LAYER_ABBREV_STR, str);
            pTrav = pTrav->pNextGlobal;
        }
    }

    // print devices
    if (s_delta.deviceCount > 0)
    {
        VKTRACE_VK_SNAPSHOT_LL_NODE* pDeviceNode = s_delta.pDevices;
        sprintf(str, "======== DELTA SNAPSHOT Devices:");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, (VkObject)NULL, 0, VKTRACESNAPSHOT_SNAPSHOT_DATA, LAYER_ABBREV_STR, str);
        while (pDeviceNode != NULL)
        {
            VKTRACE_VK_SNAPSHOT_DEVICE_NODE* pDev = (VKTRACE_VK_SNAPSHOT_DEVICE_NODE*)pDeviceNode->obj.pStruct;
            char * createInfoStr = vk_print_vkdevicecreateinfo(pDev->params.pCreateInfo, "\t\t");
            sprintf(str, "\t%s obj %p:\n%s", string_VK_OBJECT_TYPE(VK_OBJECT_TYPE_DEVICE), (void*)pDeviceNode->obj.object, createInfoStr);
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, pDeviceNode->obj.object, 0, VKTRACESNAPSHOT_SNAPSHOT_DATA, LAYER_ABBREV_STR, str);
            pDeviceNode = pDeviceNode->pNextObj;
        }
    }

    // print deleted objects
    if (s_delta.deltaDeletedObjectCount > 0)
    {
        VKTRACE_VK_SNAPSHOT_DELETED_OBJ_NODE* pDelObjNode = s_delta.pDeltaDeletedObjects;
        sprintf(str, "======== DELTA SNAPSHOT Deleted Objects:");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, (VkObject)NULL, 0, VKTRACESNAPSHOT_SNAPSHOT_DATA, LAYER_ABBREV_STR, str);
        while (pDelObjNode != NULL)
        {
            sprintf(str, "         %s obj %p", string_VK_OBJECT_TYPE(pDelObjNode->objType), (void*)pDelObjNode->object);
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, pDelObjNode->object, 0, VKTRACESNAPSHOT_SNAPSHOT_DATA, LAYER_ABBREV_STR, str);
            pDelObjNode = pDelObjNode->pNextObj;
        }
    }
}

void vktraceSnapshotStopTracking(void)
{
    assert(!"Not Implemented");
}

void vktraceSnapshotClear(void)
{
    assert(!"Not Implemented");
}

VKTRACE_VK_SNAPSHOT vktraceSnapshotMerge(const VKTRACE_VK_SNAPSHOT* const pDelta, const VKTRACE_VK_SNAPSHOT* const pSnapshot)
{
    assert(!"Not Implemented");
}




//=============================================================================
// Old Exported methods
//=============================================================================
uint64_t vktraceSnapshotGetObjectCount(VkObjectType type)
{
    uint64_t retVal = /*(type == VK_OBJECT_TYPE_ANY) ? s_delta.globalObjCount :*/ s_delta.numObjs[type];
    return retVal;
}

VkResult vktraceSnapshotGetObjects(VkObjectType type, uint64_t objCount, VKTRACE_VK_SNAPSHOT_OBJECT_NODE *pObjNodeArray)
{
    // This bool flags if we're pulling all objs or just a single class of objs
    bool32_t bAllObjs = false; /*(type == VK_OBJECT_TYPE_ANY);*/
    // Check the count first thing
    uint64_t maxObjCount = (bAllObjs) ? s_delta.globalObjCount : s_delta.numObjs[type];
    if (objCount > maxObjCount) {
        char str[1024];
        sprintf(str, "OBJ ERROR : Received objTrackGetObjects() request for %lu objs, but there are only %lu objs of type %s", objCount, maxObjCount, string_VK_OBJECT_TYPE(type));
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, 0, 0, VKTRACESNAPSHOT_OBJCOUNT_MAX_EXCEEDED, LAYER_ABBREV_STR, str);
        return VK_ERROR_INVALID_VALUE;
    }

    VKTRACE_VK_SNAPSHOT_LL_NODE* pTrav = (bAllObjs) ? s_delta.pGlobalObjs : s_delta.pObjectHead[type];

    for (uint64_t i = 0; i < objCount; i++) {
        if (!pTrav) {
            char str[1024];
            sprintf(str, "OBJ INTERNAL ERROR : Ran out of %s objs! Should have %lu, but only copied %lu and not the requested %lu.", string_VK_OBJECT_TYPE(type), maxObjCount, i, objCount);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, 0, 0, VKTRACESNAPSHOT_INTERNAL_ERROR, LAYER_ABBREV_STR, str);
            return VK_ERROR_UNKNOWN;
        }
        memcpy(&pObjNodeArray[i], pTrav, sizeof(VKTRACE_VK_SNAPSHOT_OBJECT_NODE));
        pTrav = (bAllObjs) ? pTrav->pNextGlobal : pTrav->pNextObj;
    }
    return VK_SUCCESS;
}

void vktraceSnapshotPrintObjects(void)
{
    vktraceSnapshotPrintDelta();
}

#include "vk_generic_intercept_proc_helper.h"
VK_LAYER_EXPORT void* VKAPI vkGetProcAddr(VkPhysicalDevice physicalDevice, const char* funcName)
{
    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) physicalDevice;
    if ((void*)physicalDevice == NULL)
        return NULL;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initVktraceSnapshot);

    // TODO: This needs to be changed, need only the entry points this layer intercepts
    //addr = layer_intercept_proc(funcName);
    //if (addr)
    //    return addr;
    //else
    if (!strncmp("vktraceSnapshotGetObjectCount", funcName, sizeof("vktraceSnapshotGetObjectCount")))
        return vktraceSnapshotGetObjectCount;
    else if (!strncmp("vktraceSnapshotGetObjects", funcName, sizeof("vktraceSnapshotGetObjects")))
        return vktraceSnapshotGetObjects;
    else if (!strncmp("vktraceSnapshotPrintObjects", funcName, sizeof("vktraceSnapshotPrintObjects")))
        return vktraceSnapshotPrintObjects;
    else if (!strncmp("vktraceSnapshotStartTracking", funcName, sizeof("vktraceSnapshotStartTracking")))
        return vktraceSnapshotStartTracking;
    else if (!strncmp("vktraceSnapshotGetDelta", funcName, sizeof("vktraceSnapshotGetDelta")))
        return vktraceSnapshotGetDelta;
    else if (!strncmp("vktraceSnapshotGetSnapshot", funcName, sizeof("vktraceSnapshotGetSnapshot")))
        return vktraceSnapshotGetSnapshot;
    else if (!strncmp("vktraceSnapshotPrintDelta", funcName, sizeof("vktraceSnapshotPrintDelta")))
        return vktraceSnapshotPrintDelta;
    else if (!strncmp("vktraceSnapshotStopTracking", funcName, sizeof("vktraceSnapshotStopTracking")))
        return vktraceSnapshotStopTracking;
    else if (!strncmp("vktraceSnapshotClear", funcName, sizeof("vktraceSnapshotClear")))
        return vktraceSnapshotClear;
    else if (!strncmp("vktraceSnapshotMerge", funcName, sizeof("vktraceSnapshotMerge")))
        return vktraceSnapshotMerge;
    else {
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((VkPhysicalDevice)gpuw->nextObject, funcName);
    }
}

