/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2015 Valve Corporation
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
 * Author: Chia-I Wu <olvaffe@gmail.com>
 * Author: Chia-I Wu <olv@lunarg.com>
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include "compiler/pipeline/pipeline_compiler_interface.h"
#include "genhw/genhw.h"
#include "kmd/winsys.h"
#include "cmd_priv.h"

static const uint32_t *
writer_pointer(const struct intel_cmd *cmd,
               enum intel_cmd_writer_type which,
               unsigned offset)
{
    const struct intel_cmd_writer *writer = &cmd->writers[which];
    return (const uint32_t *) ((const char *) writer->ptr + offset);
}

static uint32_t
writer_dw(const struct intel_cmd *cmd,
          enum intel_cmd_writer_type which,
          unsigned offset, unsigned dw_index,
          const char *format, ...)
{
    const uint32_t *dw = writer_pointer(cmd, which, offset);
    va_list ap;
    char desc[16];
    int len;

    fprintf(stderr, "0x%08x:      0x%08x: ",
            offset + (dw_index << 2), dw[dw_index]);

    va_start(ap, format);
    len = vsnprintf(desc, sizeof(desc), format, ap);
    va_end(ap);

    if (len >= sizeof(desc)) {
        len = sizeof(desc) - 1;
        desc[len] = '\0';
    }

    if (desc[len - 1] == '\n') {
        desc[len - 1] = '\0';
        fprintf(stderr, "%8s: \n", desc);
    } else {
        fprintf(stderr, "%8s: ", desc);
    }

    return dw[dw_index];
}

static void
writer_decode_blob(const struct intel_cmd *cmd,
                   enum intel_cmd_writer_type which,
                   const struct intel_cmd_item *item)
{
    const unsigned state_size = sizeof(uint32_t);
    const unsigned count = item->size / state_size;
    unsigned offset = item->offset;
    unsigned i;

    for (i = 0; i < count; i += 4) {
        const uint32_t *dw = writer_pointer(cmd, which, offset);

        writer_dw(cmd, which, offset, 0, "BLOB%d", i / 4);

        switch (count - i) {
        case 1:
            fprintf(stderr, "(%10.4f, %10c, %10c, %10c) "
                            "(0x%08x, %10c, %10c, %10c)\n",
                            u_uif(dw[0]), 'X', 'X', 'X',
                            dw[0],        'X', 'X', 'X');
            break;
        case 2:
            fprintf(stderr, "(%10.4f, %10.4f, %10c, %10c) "
                            "(0x%08x, 0x%08x, %10c, %10c)\n",
                            u_uif(dw[0]), u_uif(dw[1]), 'X', 'X',
                                  dw[0],        dw[1],  'X', 'X');
            break;
        case 3:
            fprintf(stderr, "(%10.4f, %10.4f, %10.4f, %10c) "
                            "(0x%08x, 0x%08x, 0x%08x, %10c)\n",
                            u_uif(dw[0]), u_uif(dw[1]), u_uif(dw[2]), 'X',
                                  dw[0],        dw[1],        dw[2],  'X');
            break;
        default:
            fprintf(stderr, "(%10.4f, %10.4f, %10.4f, %10.4f) "
                            "(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
                            u_uif(dw[0]), u_uif(dw[1]), u_uif(dw[2]), u_uif(dw[3]),
                                  dw[0],        dw[1],        dw[2],        dw[3]);
            break;
        }

        offset += state_size * 4;
    }
}

static void
writer_decode_clip_viewport(const struct intel_cmd *cmd,
                            enum intel_cmd_writer_type which,
                            const struct intel_cmd_item *item)
{
    const unsigned state_size = sizeof(uint32_t) * 4;
    const unsigned count = item->size / state_size;
    unsigned offset = item->offset;
    unsigned i;

    for (i = 0; i < count; i++) {
        uint32_t dw;

        dw = writer_dw(cmd, which, offset, 0, "CLIP VP%d", i);
        fprintf(stderr, "xmin = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 1, "CLIP VP%d", i);
        fprintf(stderr, "xmax = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 2, "CLIP VP%d", i);
        fprintf(stderr, "ymin = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 3, "CLIP VP%d", i);
        fprintf(stderr, "ymax = %f\n", u_uif(dw));

        offset += state_size;
    }
}

static void
writer_decode_sf_clip_viewport_gen7(const struct intel_cmd *cmd,
                                    enum intel_cmd_writer_type which,
                                    const struct intel_cmd_item *item)
{
    const unsigned state_size = sizeof(uint32_t) * 16;
    const unsigned count = item->size / state_size;
    unsigned offset = item->offset;
    unsigned i;

    for (i = 0; i < count; i++) {
        uint32_t dw;

        dw = writer_dw(cmd, which, offset, 0, "SF_CLIP VP%d", i);
        fprintf(stderr, "m00 = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 1, "SF_CLIP VP%d", i);
        fprintf(stderr, "m11 = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 2, "SF_CLIP VP%d", i);
        fprintf(stderr, "m22 = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 3, "SF_CLIP VP%d", i);
        fprintf(stderr, "m30 = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 4, "SF_CLIP VP%d", i);
        fprintf(stderr, "m31 = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 5, "SF_CLIP VP%d", i);
        fprintf(stderr, "m32 = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 8, "SF_CLIP VP%d", i);
        fprintf(stderr, "guardband xmin = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 9, "SF_CLIP VP%d", i);
        fprintf(stderr, "guardband xmax = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 10, "SF_CLIP VP%d", i);
        fprintf(stderr, "guardband ymin = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 11, "SF_CLIP VP%d", i);
        fprintf(stderr, "guardband ymax = %f\n", u_uif(dw));

        offset += state_size;
    }
}

static void
writer_decode_sf_viewport_gen6(const struct intel_cmd *cmd,
                               enum intel_cmd_writer_type which,
                               const struct intel_cmd_item *item)
{
    const unsigned state_size = sizeof(uint32_t) * 8;
    const unsigned count = item->size / state_size;
    unsigned offset = item->offset;
    unsigned i;

    for (i = 0; i < count; i++) {
        uint32_t dw;

        dw = writer_dw(cmd, which, offset, 0, "SF VP%d", i);
        fprintf(stderr, "m00 = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 1, "SF VP%d", i);
        fprintf(stderr, "m11 = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 2, "SF VP%d", i);
        fprintf(stderr, "m22 = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 3, "SF VP%d", i);
        fprintf(stderr, "m30 = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 4, "SF VP%d", i);
        fprintf(stderr, "m31 = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 5, "SF VP%d", i);
        fprintf(stderr, "m32 = %f\n", u_uif(dw));

        offset += state_size;
    }
}

static void
writer_decode_sf_viewport(const struct intel_cmd *cmd,
                          enum intel_cmd_writer_type which,
                          const struct intel_cmd_item *item)
{
    if (cmd_gen(cmd) >= INTEL_GEN(7))
        writer_decode_sf_clip_viewport_gen7(cmd, which, item);
    else
        writer_decode_sf_viewport_gen6(cmd, which, item);
}

static void
writer_decode_scissor_rect(const struct intel_cmd *cmd,
                           enum intel_cmd_writer_type which,
                           const struct intel_cmd_item *item)
{
    const unsigned state_size = sizeof(uint32_t) * 2;
    const unsigned count = item->size / state_size;
    unsigned offset = item->offset;
    unsigned i;

    for (i = 0; i < count; i++) {
        uint32_t dw;

        dw = writer_dw(cmd, which, offset, 0, "SCISSOR%d", i);
        fprintf(stderr, "xmin %d, ymin %d\n",
                GEN_EXTRACT(dw, GEN6_SCISSOR_DW0_MIN_X),
                GEN_EXTRACT(dw, GEN6_SCISSOR_DW0_MIN_Y));

        dw = writer_dw(cmd, which, offset, 1, "SCISSOR%d", i);
        fprintf(stderr, "xmax %d, ymax %d\n",
                GEN_EXTRACT(dw, GEN6_SCISSOR_DW1_MAX_X),
                GEN_EXTRACT(dw, GEN6_SCISSOR_DW1_MAX_Y));

        offset += state_size;
    }
}

static void
writer_decode_cc_viewport(const struct intel_cmd *cmd,
                          enum intel_cmd_writer_type which,
                          const struct intel_cmd_item *item)
{
    const unsigned state_size = sizeof(uint32_t) * 2;
    const unsigned count = item->size / state_size;
    unsigned offset = item->offset;
    unsigned i;

    for (i = 0; i < count; i++) {
        uint32_t dw;

        dw = writer_dw(cmd, which, offset, 0, "CC VP%d", i);
        fprintf(stderr, "min_depth = %f\n", u_uif(dw));

        dw = writer_dw(cmd, which, offset, 1, "CC VP%d", i);
        fprintf(stderr, "max_depth = %f\n", u_uif(dw));

        offset += state_size;
    }
}

static void
writer_decode_color_calc(const struct intel_cmd *cmd,
                         enum intel_cmd_writer_type which,
                         const struct intel_cmd_item *item)
{
    uint32_t dw;

    dw = writer_dw(cmd, which, item->offset, 0, "CC");
    fprintf(stderr, "alpha test format %s, round disable %d, "
            "stencil ref %d, bf stencil ref %d\n",
            GEN_EXTRACT(dw, GEN6_CC_DW0_ALPHATEST) ? "FLOAT32" : "UNORM8",
            (bool) (dw & GEN6_CC_DW0_ROUND_DISABLE_DISABLE),
            GEN_EXTRACT(dw, GEN6_CC_DW0_STENCIL0_REF),
            GEN_EXTRACT(dw, GEN6_CC_DW0_STENCIL1_REF));

    writer_dw(cmd, which, item->offset, 1, "CC\n");

    dw = writer_dw(cmd, which, item->offset, 2, "CC");
    fprintf(stderr, "constant red %f\n", u_uif(dw));

    dw = writer_dw(cmd, which, item->offset, 3, "CC");
    fprintf(stderr, "constant green %f\n", u_uif(dw));

    dw = writer_dw(cmd, which, item->offset, 4, "CC");
    fprintf(stderr, "constant blue %f\n", u_uif(dw));

    dw = writer_dw(cmd, which, item->offset, 5, "CC");
    fprintf(stderr, "constant alpha %f\n", u_uif(dw));
}

static void
writer_decode_depth_stencil(const struct intel_cmd *cmd,
                            enum intel_cmd_writer_type which,
                            const struct intel_cmd_item *item)
{
    uint32_t dw;

    dw = writer_dw(cmd, which, item->offset, 0, "D_S");
    fprintf(stderr, "stencil %sable, func %d, write %sable\n",
            (dw & GEN6_ZS_DW0_STENCIL_TEST_ENABLE) ? "en" : "dis",
            GEN_EXTRACT(dw, GEN6_ZS_DW0_STENCIL0_FUNC),
            (dw & GEN6_ZS_DW0_STENCIL_WRITE_ENABLE) ? "en" : "dis");

    dw = writer_dw(cmd, which, item->offset, 1, "D_S");
    fprintf(stderr, "stencil test mask 0x%x, write mask 0x%x\n",
            GEN_EXTRACT(dw, GEN6_ZS_DW1_STENCIL0_VALUEMASK),
            GEN_EXTRACT(dw, GEN6_ZS_DW1_STENCIL0_WRITEMASK));

    dw = writer_dw(cmd, which, item->offset, 2, "D_S");
    fprintf(stderr, "depth test %sable, func %d, write %sable\n",
            (dw & GEN6_ZS_DW2_DEPTH_TEST_ENABLE) ? "en" : "dis",
            GEN_EXTRACT(dw, GEN6_ZS_DW2_DEPTH_FUNC),
            (dw & GEN6_ZS_DW2_DEPTH_WRITE_ENABLE) ? "en" : "dis");
}

static void
writer_decode_blend(const struct intel_cmd *cmd,
                    enum intel_cmd_writer_type which,
                    const struct intel_cmd_item *item)
{
    const unsigned state_size = sizeof(uint32_t) * 2;
    const unsigned count = item->size / state_size;
    unsigned offset = item->offset;
    unsigned i;

    for (i = 0; i < count; i++) {
        writer_dw(cmd, which, offset, 0, "BLEND%d\n", i);
        writer_dw(cmd, which, offset, 1, "BLEND%d\n", i);

        offset += state_size;
    }
}

static void
writer_decode_sampler(const struct intel_cmd *cmd,
                      enum intel_cmd_writer_type which,
                      const struct intel_cmd_item *item)
{
    const unsigned state_size = sizeof(uint32_t) * 4;
    const unsigned count = item->size / state_size;
    unsigned offset = item->offset;
    unsigned i;

    for (i = 0; i < count; i++) {
        writer_dw(cmd, which, offset, 0, "WM SAMP%d", i);
        fprintf(stderr, "filtering\n");

        writer_dw(cmd, which, offset, 1, "WM SAMP%d", i);
        fprintf(stderr, "wrapping, lod\n");

        writer_dw(cmd, which, offset, 2, "WM SAMP%d", i);
        fprintf(stderr, "default color pointer\n");

        writer_dw(cmd, which, offset, 3, "WM SAMP%d", i);
        fprintf(stderr, "chroma key, aniso\n");

        offset += state_size;
    }
}

static void
writer_decode_surface_gen7(const struct intel_cmd *cmd,
                           enum intel_cmd_writer_type which,
                           const struct intel_cmd_item *item)
{
    uint32_t dw;

    dw = writer_dw(cmd, which, item->offset, 0, "SURF");
    fprintf(stderr, "type 0x%x, format 0x%x, tiling %d, %s array\n",
            GEN_EXTRACT(dw, GEN7_SURFACE_DW0_TYPE),
            GEN_EXTRACT(dw, GEN7_SURFACE_DW0_FORMAT),
            GEN_EXTRACT(dw, GEN7_SURFACE_DW0_TILING),
            (dw & GEN7_SURFACE_DW0_IS_ARRAY) ? "is" : "not");

    writer_dw(cmd, which, item->offset, 1, "SURF");
    fprintf(stderr, "offset\n");

    dw = writer_dw(cmd, which, item->offset, 2, "SURF");
    fprintf(stderr, "%dx%d size\n",
            GEN_EXTRACT(dw, GEN7_SURFACE_DW2_WIDTH),
            GEN_EXTRACT(dw, GEN7_SURFACE_DW2_HEIGHT));

    dw = writer_dw(cmd, which, item->offset, 3, "SURF");
    fprintf(stderr, "depth %d, pitch %d\n",
            GEN_EXTRACT(dw, GEN7_SURFACE_DW3_DEPTH),
            GEN_EXTRACT(dw, GEN7_SURFACE_DW3_PITCH));

    dw = writer_dw(cmd, which, item->offset, 4, "SURF");
    fprintf(stderr, "min array element %d, array extent %d\n",
            GEN_EXTRACT(dw, GEN7_SURFACE_DW4_MIN_ARRAY_ELEMENT),
            GEN_EXTRACT(dw, GEN7_SURFACE_DW4_RT_VIEW_EXTENT));

    dw = writer_dw(cmd, which, item->offset, 5, "SURF");
    fprintf(stderr, "mip base %d, mips %d, x,y offset: %d,%d\n",
            GEN_EXTRACT(dw, GEN7_SURFACE_DW5_MIN_LOD),
            GEN_EXTRACT(dw, GEN7_SURFACE_DW5_MIP_COUNT_LOD),
            GEN_EXTRACT(dw, GEN7_SURFACE_DW5_X_OFFSET),
            GEN_EXTRACT(dw, GEN7_SURFACE_DW5_Y_OFFSET));

    writer_dw(cmd, which, item->offset, 6, "SURF\n");
    writer_dw(cmd, which, item->offset, 7, "SURF\n");
}

static void
writer_decode_surface_gen6(const struct intel_cmd *cmd,
                           enum intel_cmd_writer_type which,
                           const struct intel_cmd_item *item)
{
    uint32_t dw;

    dw = writer_dw(cmd, which, item->offset, 0, "SURF");
    fprintf(stderr, "type 0x%x, format 0x%x\n",
            GEN_EXTRACT(dw, GEN6_SURFACE_DW0_TYPE),
            GEN_EXTRACT(dw, GEN6_SURFACE_DW0_FORMAT));

    writer_dw(cmd, which, item->offset, 1, "SURF");
    fprintf(stderr, "offset\n");

    dw = writer_dw(cmd, which, item->offset, 2, "SURF");
    fprintf(stderr, "%dx%d size, %d mips\n",
            GEN_EXTRACT(dw, GEN6_SURFACE_DW2_WIDTH),
            GEN_EXTRACT(dw, GEN6_SURFACE_DW2_HEIGHT),
            GEN_EXTRACT(dw, GEN6_SURFACE_DW2_MIP_COUNT_LOD));

    dw = writer_dw(cmd, which, item->offset, 3, "SURF");
    fprintf(stderr, "pitch %d, tiling %d\n",
            GEN_EXTRACT(dw, GEN6_SURFACE_DW3_PITCH),
            GEN_EXTRACT(dw, GEN6_SURFACE_DW3_TILING));

    dw = writer_dw(cmd, which, item->offset, 4, "SURF");
    fprintf(stderr, "mip base %d\n",
            GEN_EXTRACT(dw, GEN6_SURFACE_DW4_MIN_LOD));

    dw = writer_dw(cmd, which, item->offset, 5, "SURF");
    fprintf(stderr, "x,y offset: %d,%d\n",
            GEN_EXTRACT(dw, GEN6_SURFACE_DW5_X_OFFSET),
            GEN_EXTRACT(dw, GEN6_SURFACE_DW5_Y_OFFSET));
}

static void
writer_decode_surface(const struct intel_cmd *cmd,
                      enum intel_cmd_writer_type which,
                      const struct intel_cmd_item *item)
{
    if (cmd_gen(cmd) >= INTEL_GEN(7))
        writer_decode_surface_gen7(cmd, which, item);
    else
        writer_decode_surface_gen6(cmd, which, item);
}

static void
writer_decode_binding_table(const struct intel_cmd *cmd,
                            enum intel_cmd_writer_type which,
                            const struct intel_cmd_item *item)
{
    const unsigned state_size = sizeof(uint32_t) * 1;
    const unsigned count = item->size / state_size;
    unsigned offset = item->offset;
    unsigned i;

    for (i = 0; i < count; i++) {
        writer_dw(cmd, which, offset, 0, "BIND");
        fprintf(stderr, "BINDING_TABLE_STATE[%d]\n", i);

        offset += state_size;
    }
}

static void
writer_decode_kernel(const struct intel_cmd *cmd,
                     enum intel_cmd_writer_type which,
                     const struct intel_cmd_item *item)
{
    const void *kernel = (const void *)
        writer_pointer(cmd, which, item->offset);

    fprintf(stderr, "0x%08zx:\n", item->offset);
    intel_disassemble_kernel(cmd->dev->gpu, kernel, item->size);
}

static const struct {
    void (*func)(const struct intel_cmd *cmd,
                 enum intel_cmd_writer_type which,
                 const struct intel_cmd_item *item);
} writer_decode_table[INTEL_CMD_ITEM_COUNT] = {
    [INTEL_CMD_ITEM_BLOB]                = { writer_decode_blob },
    [INTEL_CMD_ITEM_CLIP_VIEWPORT]       = { writer_decode_clip_viewport },
    [INTEL_CMD_ITEM_SF_VIEWPORT]         = { writer_decode_sf_viewport },
    [INTEL_CMD_ITEM_SCISSOR_RECT]        = { writer_decode_scissor_rect },
    [INTEL_CMD_ITEM_CC_VIEWPORT]         = { writer_decode_cc_viewport },
    [INTEL_CMD_ITEM_COLOR_CALC]          = { writer_decode_color_calc },
    [INTEL_CMD_ITEM_DEPTH_STENCIL]       = { writer_decode_depth_stencil },
    [INTEL_CMD_ITEM_BLEND]               = { writer_decode_blend },
    [INTEL_CMD_ITEM_SAMPLER]             = { writer_decode_sampler },
    [INTEL_CMD_ITEM_SURFACE]             = { writer_decode_surface },
    [INTEL_CMD_ITEM_BINDING_TABLE]       = { writer_decode_binding_table },
    [INTEL_CMD_ITEM_KERNEL]              = { writer_decode_kernel },
};

static void cmd_writer_decode_items(struct intel_cmd *cmd,
                                    enum intel_cmd_writer_type which)
{
    struct intel_cmd_writer *writer = &cmd->writers[which];
    int i;

    if (!writer->item_used)
        return;

    writer->ptr = intel_bo_map(writer->bo, false);
    if (!writer->ptr)
        return;

    for (i = 0; i < writer->item_used; i++) {
        const struct intel_cmd_item *item = &writer->items[i];

        writer_decode_table[item->type].func(cmd, which, item);
    }

    intel_bo_unmap(writer->bo);
    writer->ptr = NULL;
}

static void cmd_writer_decode(struct intel_cmd *cmd,
                              enum intel_cmd_writer_type which,
                              bool decode_inst_writer)
{
    struct intel_cmd_writer *writer = &cmd->writers[which];

    assert(writer->bo && !writer->ptr);

    switch (which) {
    case INTEL_CMD_WRITER_BATCH:
        fprintf(stderr, "decoding batch buffer: %zu bytes\n", writer->used);
        if (writer->used) {
            intel_winsys_decode_bo(cmd->dev->winsys,
                    writer->bo, writer->used);
        }
        break;
    case INTEL_CMD_WRITER_SURFACE:
        fprintf(stderr, "decoding surface state buffer: %d states\n",
                writer->item_used);
        cmd_writer_decode_items(cmd, which);
        break;
    case INTEL_CMD_WRITER_STATE:
        fprintf(stderr, "decoding dynamic state buffer: %d states\n",
                writer->item_used);
        cmd_writer_decode_items(cmd, which);
        break;
    case INTEL_CMD_WRITER_INSTRUCTION:
        if (decode_inst_writer) {
            fprintf(stderr, "decoding instruction buffer: %d kernels\n",
                    writer->item_used);

            cmd_writer_decode_items(cmd, which);
        } else {
            fprintf(stderr, "skipping instruction buffer: %d kernels\n",
                    writer->item_used);
        }
        break;
    default:
        break;
    }
}

/**
 * Decode according to the recorded items.  This can be called only after a
 * successful intel_cmd_end().
 */
void intel_cmd_decode(struct intel_cmd *cmd, bool decode_inst_writer)
{
    int i;

    assert(cmd->result == VK_SUCCESS);

    for (i = 0; i < INTEL_CMD_WRITER_COUNT; i++)
        cmd_writer_decode(cmd, i, decode_inst_writer);
}
