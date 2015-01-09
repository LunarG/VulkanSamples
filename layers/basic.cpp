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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unordered_map>
#include "xgl_dispatch_table_helper.h"
#include "xglLayer.h"

static std::unordered_map<XGL_VOID *, XGL_LAYER_DISPATCH_TABLE *> tableMap;

static XGL_LAYER_DISPATCH_TABLE * initLayerTable(const XGL_BASE_LAYER_OBJECT *gpuw)
{
    xglGetProcAddrType fpGPA;
    XGL_LAYER_DISPATCH_TABLE *pTable;

    assert(gpuw);
    std::unordered_map<XGL_VOID *, XGL_LAYER_DISPATCH_TABLE *>::const_iterator it = tableMap.find((XGL_VOID *) gpuw);
    if (it == tableMap.end())
    {
        pTable =  new XGL_LAYER_DISPATCH_TABLE;
        tableMap[(XGL_VOID *) gpuw] = pTable;
    } else
    {
        return it->second;
    }

    layer_initialize_dispatch_table(pTable, gpuw->pGPA, (XGL_PHYSICAL_GPU) gpuw->nextObject);

    return pTable;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglLayerExtension1(XGL_DEVICE device)
{
    printf("In xglLayerExtension1() call w/ device: %p\n", (void*)device);
    printf("xglLayerExtension1 returning SUCCESS\n");
    return XGL_SUCCESS;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetExtensionSupport(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pExtName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_RESULT result;
    XGL_LAYER_DISPATCH_TABLE* pTable = initLayerTable(gpuw);

    printf("At start of wrapped xglGetExtensionSupport() call w/ gpu: %p\n", (void*)gpu);
    if (!strncmp(pExtName, "xglLayerExtension1", strlen("xglLayerExtension1")))
        result = XGL_SUCCESS;
    else
        result = pTable->GetExtensionSupport((XGL_PHYSICAL_GPU)gpuw->nextObject, pExtName);
    printf("Completed wrapped xglGetExtensionSupport() call w/ gpu: %p\n", (void*)gpu);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo, XGL_DEVICE* pDevice)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_LAYER_DISPATCH_TABLE* pTable = initLayerTable(gpuw);

    printf("At start of wrapped xglCreateDevice() call w/ gpu: %p\n", (void*)gpu);
    XGL_RESULT result = pTable->CreateDevice((XGL_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    // create a mapping for the device object into the dispatch table
    tableMap.emplace(*pDevice, pTable);
    printf("Completed wrapped xglCreateDevice() call w/ pDevice, Device %p: %p\n", (void*)pDevice, (void *) *pDevice);
    return result;
}
XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFormatInfo(XGL_DEVICE device, XGL_FORMAT format, XGL_FORMAT_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_LAYER_DISPATCH_TABLE* pTable = tableMap[device];

    printf("At start of wrapped xglGetFormatInfo() call w/ device: %p\n", (void*)device);
    XGL_RESULT result = pTable->GetFormatInfo(device, format, infoType, pDataSize, pData);
    printf("Completed wrapped xglGetFormatInfo() call w/ device: %p\n", (void*)device);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(XGL_PHYSICAL_GPU gpu, XGL_SIZE maxLayerCount, XGL_SIZE maxStringSize, XGL_SIZE* pOutLayerCount, XGL_CHAR* const* pOutLayers, XGL_VOID* pReserved)
{
    if (gpu != NULL)
    {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        XGL_LAYER_DISPATCH_TABLE* pTable = initLayerTable(gpuw);

        printf("At start of wrapped xglEnumerateLayers() call w/ gpu: %p\n", gpu);
        XGL_RESULT result = pTable->EnumerateLayers((XGL_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
        return result;
    } else
    {
        if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL || pReserved == NULL)
            return XGL_ERROR_INVALID_POINTER;

        // Example of a layer that is only compatible with Intel's GPUs
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT*) pReserved;
        xglGetGpuInfoType fpGetGpuInfo;
        XGL_PHYSICAL_GPU_PROPERTIES gpuProps;
        XGL_SIZE dataSize = sizeof(XGL_PHYSICAL_GPU_PROPERTIES);
        fpGetGpuInfo = (xglGetGpuInfoType) gpuw->pGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, "xglGetGpuInfo");
        fpGetGpuInfo((XGL_PHYSICAL_GPU) gpuw->nextObject, XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES, &dataSize, &gpuProps);
        if (gpuProps.vendorId == 0x8086)
        {
            *pOutLayerCount = 1;
            strncpy((char *) pOutLayers[0], "Basic", maxStringSize);
        } else
        {
            *pOutLayerCount = 0;
        }
        return XGL_SUCCESS;
    }
}

XGL_LAYER_EXPORT XGL_VOID * XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pName)
{
    if (gpu == NULL)
        return NULL;

    initLayerTable((const XGL_BASE_LAYER_OBJECT *) gpu);

    if (!strncmp("xglGetProcAddr", pName, sizeof("xglGetProcAddr")))
        return (XGL_VOID *) xglGetProcAddr;
    else if (!strncmp("xglCreateDevice", pName, sizeof ("xglCreateDevice")))
        return (XGL_VOID *) xglCreateDevice;
    else if (!strncmp("xglGetExtensionSupport", pName, sizeof ("xglGetExtensionSupport")))
        return (XGL_VOID *) xglGetExtensionSupport;
    else if (!strncmp("xglEnumerateLayers", pName, sizeof ("xglEnumerateLayers")))
        return (XGL_VOID *) xglEnumerateLayers;
    else if (!strncmp("xglGetFormatInfo", pName, sizeof ("xglGetFormatInfo")))
        return (XGL_VOID *) xglGetFormatInfo;
    else if (!strncmp("xglLayerExtension1", pName, sizeof("xglLayerExtension1")))
        return (XGL_VOID *) xglLayerExtension1;
    else {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, pName);
    }
}
