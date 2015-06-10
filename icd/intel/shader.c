/*
 * Vulkan
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
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 *   Chia-I Wu <olv@lunarg.com>
 */

#include "dev.h"
#include "shader.h"
#include "compiler/shader/compiler_interface.h"

static void shader_destroy(struct intel_obj *obj)
{
    struct intel_shader *sh = intel_shader_from_obj(obj);

    if (sh->ir)
        shader_destroy_ir(sh->ir);
    intel_base_destroy(&sh->obj.base);
}

static VkResult shader_create(struct intel_dev *dev,
                                const VkShaderCreateInfo *info,
                                struct intel_shader **sh_ret)
{
    const struct icd_spv_header *spv =
        (const struct icd_spv_header *) info->pCode;
    struct intel_shader *sh;

    sh = (struct intel_shader *) intel_base_create(&dev->base.handle,
            sizeof(*sh), dev->base.dbg, VK_OBJECT_TYPE_SHADER, info, 0);
    if (!sh)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    if (info->codeSize < sizeof(*spv))
        return VK_ERROR_INVALID_MEMORY_SIZE;
    if (spv->magic != ICD_SPV_MAGIC)
        return VK_ERROR_BAD_SHADER_CODE;

    sh->ir = shader_create_ir(dev->gpu, info->pCode, info->codeSize);
    if (!sh->ir) {
        shader_destroy(&sh->obj);
        return VK_ERROR_BAD_SHADER_CODE;
    }

    sh->obj.destroy = shader_destroy;

    *sh_ret = sh;

    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateShader(
        VkDevice                                  device,
        const VkShaderCreateInfo*               pCreateInfo,
        VkShader*                                 pShader)
{
    struct intel_dev *dev = intel_dev(device);

    return shader_create(dev, pCreateInfo, (struct intel_shader **) pShader);
}
