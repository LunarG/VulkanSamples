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

static void shader_module_destroy(struct intel_obj *obj)
{
    struct intel_shader_module *sm = intel_shader_module_from_obj(obj);

    free(sm->code);
    sm->code = 0;
    intel_base_destroy(&sm->obj.base);
}

static VkResult shader_module_create(struct intel_dev *dev,
                                     const VkShaderModuleCreateInfo *info,
                                     struct intel_shader_module **sm_ret)
{
    const struct icd_spv_header *spv =
        (const struct icd_spv_header *) info->pCode;
    struct intel_shader_module *sm;

    sm = (struct intel_shader_module *) intel_base_create(&dev->base.handle,
            sizeof(*sm), dev->base.dbg, VK_OBJECT_TYPE_SHADER_MODULE, info, 0);
    if (!sm)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    if (info->codeSize < sizeof(*spv))
        return VK_ERROR_INVALID_MEMORY_SIZE;
    if (spv->magic != ICD_SPV_MAGIC)
        return VK_ERROR_BAD_SHADER_CODE;

    sm->code_size = info->codeSize;
    sm->code = malloc(info->codeSize);
    if (!sm->code) {
        shader_module_destroy(&sm->obj);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    memcpy(sm->code, info->pCode, info->codeSize);
    sm->obj.destroy = shader_module_destroy;

    *sm_ret = sm;

    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateShaderModule(
    VkDevice                                    device,
    const VkShaderModuleCreateInfo*             pCreateInfo,
    VkShaderModule*                             pShaderModule)
{
    struct intel_dev *dev = intel_dev(device);

    return shader_module_create(dev, pCreateInfo, (struct intel_shader_module **) pShaderModule);
}

ICD_EXPORT VkResult VKAPI vkDestroyShaderModule(
    VkDevice                                device,
    VkShaderModule                          shaderModule)

 {
    struct intel_obj *obj = intel_obj(shaderModule.handle);

    obj->destroy(obj);
    return VK_SUCCESS;
 }


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
    struct intel_shader *sh;

    sh = (struct intel_shader *) intel_base_create(&dev->base.handle,
            sizeof(*sh), dev->base.dbg, VK_OBJECT_TYPE_SHADER, info, 0);
    if (!sh)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    struct intel_shader_module *sm = intel_shader_module(info->module);

    sh->ir = shader_create_ir(dev->gpu, sm->code, sm->code_size);
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

ICD_EXPORT VkResult VKAPI vkDestroyShader(
    VkDevice                                device,
    VkShader                                shader)

 {
    struct intel_obj *obj = intel_obj(shader.handle);

    obj->destroy(obj);
    return VK_SUCCESS;
 }
