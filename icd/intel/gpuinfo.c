/*
 * XGL 3-D graphics library
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
 *    Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#include <string.h>
#include "driver.h"
#include "GitSHA1.h"

// TODO: Really want to eliminate these
#define FAKE_PROPERTY 42

// TODO: What command queues can we support?
#define NUM_QUEUES 1

XGL_RESULT XGLAPI gen7_GetGpuProperties(struct _xgl_device *pdev,
                                        XGL_SIZE * pDataSize,
                                        XGL_PHYSICAL_GPU_PROPERTIES * pData)
{
    const char *chipset;

    *pDataSize = sizeof(XGL_PHYSICAL_GPU_PROPERTIES);
    pData->apiVersion = XGL_API_VERSION;
    // TODO: Should have driver version string
    // TODO: What should version number look like?
    pData->driverVersion = 1;

    pData->vendorId = pdev->ven_id;
    pData->deviceId = pdev->dev_id;
    pData->gpuType = XGL_GPU_TYPE_INTEGRATED;
    switch (pdev->dev_id) {
#undef CHIPSET
#define CHIPSET(id, symbol, str) case id: chipset = str; break;
#include "i965_pci_ids.h"
    default:
       chipset = "Unknown Intel Chipset";
       break;
    }
    strncpy((char *) pData->gpuName, chipset, XGL_MAX_PHYSICAL_GPU_NAME);

    // TODO: Need real values here.
    pData->maxMemRefsPerSubmission = FAKE_PROPERTY;
    pData->virtualMemPageSize = FAKE_PROPERTY;
    pData->maxInlineMemoryUpdateSize = FAKE_PROPERTY;
    pData->maxBoundDescriptorSets = FAKE_PROPERTY;
    pData->maxThreadGroupSize = FAKE_PROPERTY;
    pData->timestampFrequency = FAKE_PROPERTY;
    pData->multiColorAttachmentClears = FAKE_PROPERTY;
    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI gen7_GetGpuPerformance(struct _xgl_device *pdev,
                                        XGL_SIZE * pDataSize,
                                        XGL_PHYSICAL_GPU_PERFORMANCE * pData)
{
    *pDataSize = sizeof(XGL_PHYSICAL_GPU_PERFORMANCE);
    pData->maxGpuClock = FAKE_PROPERTY;
    pData->aluPerClock = FAKE_PROPERTY;
    pData->texPerClock = FAKE_PROPERTY;
    pData->primsPerClock = FAKE_PROPERTY;
    pData->pixelsPerClock = FAKE_PROPERTY;

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI gen7_GetGpuQueueProperties(struct _xgl_device *pdev,
                                             XGL_SIZE * pDataSize,
                                             XGL_PHYSICAL_GPU_QUEUE_PROPERTIES * pData)
{
    *pDataSize = NUM_QUEUES * sizeof(XGL_PHYSICAL_GPU_QUEUE_PROPERTIES);
    pData[0].structSize = sizeof(XGL_PHYSICAL_GPU_QUEUE_PROPERTIES);
    pData[0].queueFlags = XGL_QUEUE_GRAPHICS_BIT;
    pData[0].queueCount = 1;  // TODO: What is this counting?
    pData[0].supportsTimestamps = XGL_TRUE;

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI gen7_GetGpuMemoryProperties(struct _xgl_device *pdev,
                                             XGL_SIZE * pDataSize,
                                             XGL_PHYSICAL_GPU_MEMORY_PROPERTIES * pData)
{
    *pDataSize = sizeof(XGL_PHYSICAL_GPU_MEMORY_PROPERTIES);

    // TODO: Need to set the flags properly
    pData[0].supportsMigration = XGL_FALSE;
    pData[0].supportsPinning = XGL_TRUE;
    pData[0].supportsVirtualMemoryRemapping = XGL_FALSE;

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI intelGetGpuInfo(
    XGL_PHYSICAL_GPU                            gpu,
    XGL_PHYSICAL_GPU_INFO_TYPE                  infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    XGL_RESULT ret = XGL_SUCCESS;
    struct _xgl_device *pdev = (struct _xgl_device *) gpu;

    switch (infoType) {
    case XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES:
        if (pData == NULL) {
            return XGL_ERROR_INVALID_POINTER;
        }
        return gen7_GetGpuProperties(pdev, pDataSize, pData);
        break;

    case XGL_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE:
        if (pData == NULL) {
            return XGL_ERROR_INVALID_POINTER;
        }
        return gen7_GetGpuPerformance(pdev, pDataSize, pData);
        break;

    case XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES:
        /*
         * XGL Programmers guide, page 33:
         * to determine the data size an application calls
         * xglGetGpuInfo() with a NULL data pointer. The
         * expected data size for all queue property structures
         * is returned in pDataSize
         */
        if (pData == NULL) {
            // TODO: Count queues and return space needed
            *pDataSize = NUM_QUEUES * sizeof(XGL_PHYSICAL_GPU_QUEUE_PROPERTIES);
        }
        return gen7_GetGpuQueueProperties(pdev, pDataSize, pData);
        break;

    case XGL_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES:
        if (pData == NULL) {
            return XGL_ERROR_INVALID_POINTER;
        }
        return gen7_GetGpuMemoryProperties(pdev, pDataSize, pData);
        break;

    default:
        ret = XGL_ERROR_INVALID_VALUE;
    }

    return ret;
}
