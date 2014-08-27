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

#include "shader.h"
#include "shader_il.h"

static void shader_destroy(struct intel_obj *obj)
{
    struct intel_shader *shader = intel_shader_from_obj(obj);

    icd_free(shader->pCode);
    intel_base_destroy(&shader->obj.base);
}

XGL_RESULT XGLAPI intelCreateShader(
        XGL_DEVICE                                  device,
        const XGL_SHADER_CREATE_INFO*               pCreateInfo,
        XGL_SHADER*                                 pShader)
{
    struct intel_dev *dev = intel_dev(device);
    struct intel_shader *shader;
    const struct bil_header *pBIL = pCreateInfo->pCode;

    *pShader = NULL;

    if (pCreateInfo->codeSize == 0) {
        return XGL_ERROR_INVALID_MEMORY_SIZE;
    }

    // FYI: for shader allocs XGL_SYSTEM_ALLOC_INTERNAL_SHADER

//    typedef struct _XGL_SHADER_CREATE_INFO
//    {
//        XGL_STRUCTURE_TYPE                      sType;              // Must be XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO
//        const XGL_VOID*                         pNext;              // Pointer to next structure
//        XGL_SIZE                                codeSize;           // Specified in bytes
//        const XGL_VOID*                         pCode;
//        XGL_FLAGS                               flags;              // Reserved
//    } XGL_SHADER_CREATE_INFO;

    // TODO: really validate IL
    if (pBIL->bil_magic == BILMagicNumber) {

        shader = (struct intel_shader *) intel_base_create(dev, sizeof(*shader),
                                                           dev->base.dbg, XGL_DBG_OBJECT_SHADER, pCreateInfo, 0);
        if (!shader)
            return XGL_ERROR_OUT_OF_MEMORY;

        shader->dev = dev;

        shader->codeSize = pCreateInfo->codeSize;

        shader->pCode = icd_alloc(pCreateInfo->codeSize, 4, XGL_SYSTEM_ALLOC_INTERNAL_SHADER);
        if (shader->pCode == NULL) {
            intel_base_destroy(&shader->obj.base);
            return XGL_ERROR_OUT_OF_MEMORY;
        }

        shader->obj.destroy = shader_destroy;

        // TODO: Shader pre-processing

        memcpy(shader->pCode, pCreateInfo->pCode, pCreateInfo->codeSize);
        *pShader = shader;

        return XGL_SUCCESS;
    }

    return XGL_ERROR_BAD_SHADER_CODE;
}

