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
 *
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unordered_map>
#include "xgl_dispatch_table_helper.h"
#include "xglLayer.h"

static void initLayerTable(const XGL_BASE_LAYER_OBJECT *gpuw, XGL_LAYER_DISPATCH_TABLE *pTable, const unsigned int layerNum);

/******************************** Layer multi1 functions **************************/
static std::unordered_map<XGL_VOID *, XGL_LAYER_DISPATCH_TABLE *> tableMap1;
static bool layer1_first_activated = false;

static XGL_LAYER_DISPATCH_TABLE * getLayer1Table(const XGL_BASE_LAYER_OBJECT *gpuw)
{
    XGL_LAYER_DISPATCH_TABLE *pTable;

    assert(gpuw);
    std::unordered_map<XGL_VOID *, XGL_LAYER_DISPATCH_TABLE *>::const_iterator it = tableMap1.find((XGL_VOID *) gpuw);
    if (it == tableMap1.end())
    {
        pTable =  new XGL_LAYER_DISPATCH_TABLE;
        tableMap1[(XGL_VOID *) gpuw] = pTable;
        initLayerTable(gpuw, pTable, 1);
        return pTable;
    } else
    {
        return it->second;
    }
}
#ifdef __cplusplus
extern "C" {
#endif


XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi1CreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo,
                                                      XGL_DEVICE* pDevice)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_LAYER_DISPATCH_TABLE* pTable = getLayer1Table(gpuw);

    printf("At start of multi1 layer xglCreateDevice()\n");
    XGL_RESULT result = pTable->CreateDevice((XGL_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    // create a mapping for the device object into the dispatch table
    tableMap1.emplace(*pDevice, pTable);
    printf("Completed multi1 layer xglCreateDevice()\n");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi1CreateGraphicsPipeline(XGL_DEVICE device, const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo,
                                                                XGL_PIPELINE* pPipeline)
{
    XGL_LAYER_DISPATCH_TABLE* pTable = tableMap1[device];

    printf("At start of multi1 layer xglCreateGraphicsPipeline()\n");
    XGL_RESULT result = pTable->CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    // create a mapping for the pipeline object into the dispatch table
    tableMap1.emplace(*pPipeline, pTable);
    printf("Completed multi1 layer xglCreateGraphicsPipeline()\n");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi1StorePipeline(XGL_PIPELINE pipeline, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_LAYER_DISPATCH_TABLE* pTable = tableMap1[pipeline];

    printf("At start of multi1 layer xglStorePipeline()\n");
    XGL_RESULT result = pTable->StorePipeline(pipeline, pDataSize, pData);
    printf("Completed multi1 layer xglStorePipeline()\n");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi1EnumerateLayers(XGL_PHYSICAL_GPU gpu, XGL_SIZE maxLayerCount, XGL_SIZE maxStringSize,
                                                         XGL_SIZE* pOutLayerCount, XGL_CHAR* const* pOutLayers,
                                                         XGL_VOID* pReserved)
{
    if (gpu == NULL)
        return xglEnumerateLayers(gpu, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);

    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_LAYER_DISPATCH_TABLE* pTable = getLayer1Table(gpuw);

    printf("At start of multi1 layer xglEnumerateLayers()\n");
    XGL_RESULT result = pTable->EnumerateLayers((XGL_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
    printf("Completed multi1 layer xglEnumerateLayers()\n");
    return result;
}

XGL_LAYER_EXPORT XGL_VOID * XGLAPI multi1GetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;

    if (gpu == NULL)
        return NULL;

    getLayer1Table(gpuw);

    if (!strncmp("xglCreateDevice", pName, sizeof ("xglCreateDevice")))
        return (XGL_VOID *) multi1CreateDevice;
    else if (!strncmp("xglEnumerateLayers", pName, sizeof ("xglEnumerateLayers")))
        return (XGL_VOID *) multi1EnumerateLayers;
    else if (!strncmp("xglCreateGraphicsPipeline", pName, sizeof ("xglCreateGraphicsPipeline")))
        return (XGL_VOID *) multi1CreateGraphicsPipeline;
    else if (!strncmp("xglStorePipeline", pName, sizeof ("xglStorePipeline")))
        return (XGL_VOID *) multi1StorePipeline;
    else {
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, pName);
    }
}

/******************************** Layer multi2 functions **************************/
static std::unordered_map<XGL_VOID *, XGL_LAYER_DISPATCH_TABLE *> tableMap2;
static bool layer2_first_activated = false;

static XGL_LAYER_DISPATCH_TABLE * getLayer2Table(const XGL_BASE_LAYER_OBJECT *gpuw)
{
    XGL_LAYER_DISPATCH_TABLE *pTable;

    assert(gpuw);
    std::unordered_map<XGL_VOID *, XGL_LAYER_DISPATCH_TABLE *>::const_iterator it = tableMap2.find((XGL_VOID *) gpuw);
    if (it == tableMap2.end())
    {
        pTable =  new XGL_LAYER_DISPATCH_TABLE;
        tableMap2[(XGL_VOID *) gpuw] = pTable;
        initLayerTable(gpuw, pTable, 2);
        return pTable;
    } else
    {
        return it->second;
    }
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi2CreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo,
                                                      XGL_DEVICE* pDevice)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_LAYER_DISPATCH_TABLE* pTable = getLayer2Table(gpuw);

    printf("At start of multi2 xglCreateDevice()\n");
    XGL_RESULT result = pTable->CreateDevice((XGL_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    // create a mapping for the device object into the dispatch table for layer2
    tableMap2.emplace(*pDevice, pTable);
    printf("Completed multi2 layer xglCreateDevice()\n");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi2CreateCommandBuffer(XGL_DEVICE device, const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo,
                                                             XGL_CMD_BUFFER* pCmdBuffer)
{
    XGL_LAYER_DISPATCH_TABLE* pTable = tableMap2[device];

    printf("At start of multi2 layer xglCreateCommandBuffer()\n");
    XGL_RESULT result = pTable->CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    // create a mapping for CmdBuffer object into the dispatch table for layer 2
    tableMap2.emplace(*pCmdBuffer, pTable);
    printf("Completed multi2 layer xglCreateCommandBuffer()\n");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi2BeginCommandBuffer( XGL_CMD_BUFFER cmdBuffer, XGL_FLAGS flags)
{
    XGL_LAYER_DISPATCH_TABLE* pTable = tableMap2[cmdBuffer];

    printf("At start of multi2 layer xglBeginCommandBuffer()\n");
    XGL_RESULT result = pTable->BeginCommandBuffer(cmdBuffer, flags);
    printf("Completed multi2 layer xglBeginCommandBuffer()\n");
    return result;

}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi2EnumerateLayers(XGL_PHYSICAL_GPU gpu, XGL_SIZE maxLayerCount, XGL_SIZE maxStringSize,
                                                         XGL_SIZE* pOutLayerCount, XGL_CHAR* const* pOutLayers,
                                                         XGL_VOID* pReserved)
{
    if (gpu == NULL)
        return xglEnumerateLayers(gpu, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);

    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_LAYER_DISPATCH_TABLE* pTable = getLayer2Table(gpuw);

    printf("At start of multi2 layer xglEnumerateLayers()\n");
    XGL_RESULT result = pTable->EnumerateLayers((XGL_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
    printf("Completed multi2 layer xglEnumerateLayers()\n");
    return result;
}

XGL_LAYER_EXPORT XGL_VOID * XGLAPI multi2GetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;

    if (gpu == NULL)
        return NULL;

    getLayer2Table(gpuw);

    if (!strncmp("xglCreateDevice", pName, sizeof ("xglCreateDevice")))
        return (XGL_VOID *) multi2CreateDevice;
    else if (!strncmp("xglEnumerateLayers", pName, sizeof ("xglEnumerateLayers")))
        return (XGL_VOID *) multi2EnumerateLayers;
    else if (!strncmp("xglCreateCommandBuffer", pName, sizeof ("xglCreateCommandBuffer")))
        return (XGL_VOID *) multi2CreateCommandBuffer;
    else if (!strncmp("xglBeginCommandBuffer", pName, sizeof ("xglBeginCommandBuffer")))
        return (XGL_VOID *) multi2BeginCommandBuffer;
    else {
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, pName);
    }
}

/********************************* Common functions ********************************/
XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(XGL_PHYSICAL_GPU gpu, XGL_SIZE maxLayerCount, XGL_SIZE maxStringSize,
                                                      XGL_SIZE* pOutLayerCount, XGL_CHAR* const* pOutLayers,
                                                      XGL_VOID* pReserved)
{
    if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL || pOutLayers[1] == NULL || pReserved == NULL)
        return XGL_ERROR_INVALID_POINTER;

    if (maxLayerCount < 2)
        return XGL_ERROR_INITIALIZATION_FAILED;
    *pOutLayerCount = 2;
    strncpy((char *) pOutLayers[0], "multi1", maxStringSize);
    strncpy((char *) pOutLayers[1], "multi2", maxStringSize);
    return XGL_SUCCESS;
}

XGL_LAYER_EXPORT XGL_VOID * XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pName)
{
    // to find each layers GPA routine Loader will search via "<layerName>GetProcAddr"
    if (!strncmp("multi1GetProcAddr", pName, sizeof("multi1GetProcAddr")))
        return (XGL_VOID *) multi1GetProcAddr;
    else if (!strncmp("multi2GetProcAddr", pName, sizeof("multi2GetProcAddr")))
        return (XGL_VOID *) multi2GetProcAddr;
    else if (!strncmp("xglGetProcAddr", pName, sizeof("xglGetProcAddr")))
        return (XGL_VOID *) xglGetProcAddr;

    // use first layer activated as GPA dispatch table activation happens in order
    else if (layer1_first_activated)
        return multi1GetProcAddr(gpu, pName);
    else if (layer2_first_activated)
        return multi2GetProcAddr(gpu, pName);
    else
        return NULL;

}

#ifdef __cplusplus
}    //extern "C"
#endif

static void initLayerTable(const XGL_BASE_LAYER_OBJECT *gpuw, XGL_LAYER_DISPATCH_TABLE *pTable, const unsigned int layerNum)
{
    if (layerNum == 2 && layer1_first_activated == false)
        layer2_first_activated = true;
    if (layerNum == 1 && layer2_first_activated == false)
        layer1_first_activated = true;

    layer_initialize_dispatch_table(pTable, gpuw->pGPA, (XGL_PHYSICAL_GPU) gpuw->nextObject);
}
