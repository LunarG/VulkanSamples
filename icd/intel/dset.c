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
#include "sampler.h"
#include "dset.h"

static void dset_destroy(struct intel_obj *obj)
{
    struct intel_dset *dset = intel_dset_from_obj(obj);

    intel_dset_destroy(dset);
}

XGL_RESULT intel_dset_create(struct intel_dev *dev,
                             const XGL_DESCRIPTOR_SET_CREATE_INFO *info,
                             struct intel_dset **dset_ret)
{
    struct intel_dset *dset;

    dset = (struct intel_dset *) intel_base_create(dev,
            sizeof(*dset), dev->base.dbg, XGL_DBG_OBJECT_SAMPLER, info, 0);
    if (!dset)
        return XGL_ERROR_OUT_OF_MEMORY;

    dset->dev = dev;
    dset->slots = icd_alloc(sizeof(dset->slots[0]) * info->slots,
            0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!dset->slots) {
        intel_dset_destroy(dset);
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    dset->obj.destroy = dset_destroy;

    *dset_ret = dset;

    return XGL_SUCCESS;
}

void intel_dset_destroy(struct intel_dset *dset)
{
    intel_base_destroy(&dset->obj.base);
}

XGL_RESULT XGLAPI intelCreateDescriptorSet(
    XGL_DEVICE                                  device,
    const XGL_DESCRIPTOR_SET_CREATE_INFO*       pCreateInfo,
    XGL_DESCRIPTOR_SET*                         pDescriptorSet)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_dset_create(dev, pCreateInfo,
            (struct intel_dset **) pDescriptorSet);
}

XGL_VOID XGLAPI intelBeginDescriptorSetUpdate(
    XGL_DESCRIPTOR_SET                          descriptorSet)
{
    /* no-op */
}

XGL_VOID XGLAPI intelEndDescriptorSetUpdate(
    XGL_DESCRIPTOR_SET                          descriptorSet)
{
    /* no-op */
}

XGL_VOID XGLAPI intelAttachSamplerDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_SAMPLER*                          pSamplers)
{
    struct intel_dset *dset = intel_dset(descriptorSet);
    XGL_UINT i;

    for (i = 0; i < slotCount; i++) {
        struct intel_dset_slot *slot = &dset->slots[startSlot + i];
        struct intel_sampler *sampler = intel_sampler(pSamplers[i]);

        slot->type = INTEL_DSET_SLOT_SAMPLER;
        slot->u.sampler = sampler;
    }
}

XGL_VOID XGLAPI intelAttachImageViewDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_IMAGE_VIEW_ATTACH_INFO*           pImageViews)
{
    struct intel_dset *dset = intel_dset(descriptorSet);
    XGL_UINT i;

    for (i = 0; i < slotCount; i++) {
        struct intel_dset_slot *slot = &dset->slots[startSlot + i];
        const XGL_IMAGE_VIEW_ATTACH_INFO *info = &pImageViews[i];
        struct intel_img_view *view = intel_img_view(info->view);

        slot->type = INTEL_DSET_SLOT_IMG_VIEW;
        slot->u.img_view = view;
    }
}

XGL_VOID XGLAPI intelAttachMemoryViewDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemViews)
{
    struct intel_dset *dset = intel_dset(descriptorSet);
    XGL_UINT i;

    for (i = 0; i < slotCount; i++) {
        struct intel_dset_slot *slot = &dset->slots[startSlot + i];
        const XGL_MEMORY_VIEW_ATTACH_INFO *info = &pMemViews[i];

        slot->type = INTEL_DSET_SLOT_MEM_VIEW;
        intel_mem_view_init(&slot->u.mem_view, dset->dev, info);
    }
}

XGL_VOID XGLAPI intelAttachNestedDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_DESCRIPTOR_SET_ATTACH_INFO*       pNestedDescriptorSets)
{
    struct intel_dset *dset = intel_dset(descriptorSet);
    XGL_UINT i;

    for (i = 0; i < slotCount; i++) {
        struct intel_dset_slot *slot = &dset->slots[startSlot + i];
        const XGL_DESCRIPTOR_SET_ATTACH_INFO *info = &pNestedDescriptorSets[i];
        struct intel_dset *nested = intel_dset(info->descriptorSet);

        slot->type = INTEL_DSET_SLOT_NESTED;
        slot->u.nested.dset = nested;
        slot->u.nested.slot_offset = info->slotOffset;
    }
}

XGL_VOID XGLAPI intelClearDescriptorSetSlots(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount)
{
    struct intel_dset *dset = intel_dset(descriptorSet);
    XGL_UINT i;

    for (i = 0; i < slotCount; i++) {
        struct intel_dset_slot *slot = &dset->slots[startSlot + i];

        slot->type = INTEL_DSET_SLOT_UNUSED;
        slot->u.unused = NULL;
    }
}
