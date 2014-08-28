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

#include "dev.h"
#include "shader.h"

static XGL_RESULT shader_parse_bil(struct intel_shader *sh,
                                   const struct intel_gpu *gpu,
                                   const struct icd_bil_header *bil,
                                   XGL_SIZE size)
{
    struct intel_ir *ir;

    ir = icd_alloc(sizeof(*ir), 0, XGL_SYSTEM_ALLOC_INTERNAL_SHADER);
    if (!ir)
        return XGL_ERROR_OUT_OF_MEMORY;

    ir->size = size - sizeof(*bil);

    ir->kernel = icd_alloc(ir->size, 0, XGL_SYSTEM_ALLOC_INTERNAL_SHADER);
    if (!ir->kernel) {
        icd_free(ir);
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    memcpy(ir->kernel, bil + 1, ir->size);

    sh->ir = ir;
    switch (bil->gen_magic) {
    case 'v':
        sh->uses |= INTEL_SHADER_USE_VID;
        sh->in_count = 1;
        sh->out_count = 1;
        break;
    case 'w':
        sh->out_count = 1;
        break;
    default:
        break;
    }

    return XGL_SUCCESS;
}

static void shader_destroy(struct intel_obj *obj)
{
    struct intel_shader *sh = intel_shader_from_obj(obj);

    icd_free(sh->ir);
    intel_base_destroy(&sh->obj.base);
}

static XGL_RESULT shader_create(struct intel_dev *dev,
                                const XGL_SHADER_CREATE_INFO *info,
                                struct intel_shader **sh_ret)
{
    const struct icd_bil_header *bil =
        (const struct icd_bil_header *) info->pCode;
    struct intel_shader *sh;
    XGL_RESULT ret;

    if (info->codeSize < sizeof(*bil))
        return XGL_ERROR_INVALID_MEMORY_SIZE;
    if (bil->magic != ICD_BIL_MAGIC)
        return XGL_ERROR_BAD_SHADER_CODE;

    sh = (struct intel_shader *) intel_base_create(dev, sizeof(*sh),
            dev->base.dbg, XGL_DBG_OBJECT_SHADER, info, 0);
    if (!sh)
        return XGL_ERROR_OUT_OF_MEMORY;

    ret = shader_parse_bil(sh, dev->gpu, bil, info->codeSize);
    if (ret != XGL_SUCCESS) {
        shader_destroy(&sh->obj);
        return ret;
    }

    sh->obj.destroy = shader_destroy;

    *sh_ret = sh;

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI intelCreateShader(
        XGL_DEVICE                                  device,
        const XGL_SHADER_CREATE_INFO*               pCreateInfo,
        XGL_SHADER*                                 pShader)
{
    struct intel_dev *dev = intel_dev(device);

    return shader_create(dev, pCreateInfo, (struct intel_shader **) pShader);
}
