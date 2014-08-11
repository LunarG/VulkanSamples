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

#ifndef DSET_H
#define DSET_H

#include "intel.h"
#include "obj.h"
#include "view.h"

enum intel_dset_slot_type {
    INTEL_DSET_SLOT_UNUSED,
    INTEL_DSET_SLOT_SAMPLER,
    INTEL_DSET_SLOT_IMG_VIEW,
    INTEL_DSET_SLOT_MEM_VIEW,
    INTEL_DSET_SLOT_NESTED,
};

struct intel_dset;

struct intel_dset_slot {
    enum intel_dset_slot_type type;

    union {
        const void *unused;
        const struct intel_sampler *sampler;
        const struct intel_img_view *img_view;

        struct intel_mem_view mem_view;

        struct {
            const struct intel_dset *dset;
            XGL_UINT slot_offset;
        } nested;

    } u;
};

struct intel_dset {
    struct intel_obj obj;

    struct intel_dev *dev;
    struct intel_dset_slot *slots;
};

static inline struct intel_dset *intel_dset(XGL_DESCRIPTOR_SET dset)
{
    return (struct intel_dset *) dset;
}

static inline struct intel_dset *intel_dset_from_obj(struct intel_obj *obj)
{
    return (struct intel_dset *) obj;
}

XGL_RESULT intel_dset_create(struct intel_dev *dev,
                             const XGL_DESCRIPTOR_SET_CREATE_INFO *info,
                             struct intel_dset **dset_ret);
void intel_dset_destroy(struct intel_dset *dset);

XGL_RESULT XGLAPI intelCreateDescriptorSet(
    XGL_DEVICE                                  device,
    const XGL_DESCRIPTOR_SET_CREATE_INFO*       pCreateInfo,
    XGL_DESCRIPTOR_SET*                         pDescriptorSet);

XGL_VOID XGLAPI intelBeginDescriptorSetUpdate(
    XGL_DESCRIPTOR_SET                          descriptorSet);

XGL_VOID XGLAPI intelEndDescriptorSetUpdate(
    XGL_DESCRIPTOR_SET                          descriptorSet);

XGL_VOID XGLAPI intelAttachSamplerDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_SAMPLER*                          pSamplers);

XGL_VOID XGLAPI intelAttachImageViewDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_IMAGE_VIEW_ATTACH_INFO*           pImageViews);

XGL_VOID XGLAPI intelAttachMemoryViewDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemViews);

XGL_VOID XGLAPI intelAttachNestedDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_DESCRIPTOR_SET_ATTACH_INFO*       pNestedDescriptorSets);

XGL_VOID XGLAPI intelClearDescriptorSetSlots(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount);

#endif /* DSET_H */
