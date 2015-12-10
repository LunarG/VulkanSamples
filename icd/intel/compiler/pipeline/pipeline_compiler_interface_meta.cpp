/*
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
 *
 */


extern "C" {
#include "gpu.h"
#include "pipeline.h"
}
#include "compiler/shader/compiler_interface.h"
#include "compiler/pipeline/pipeline_compiler_interface.h"
#include "compiler/pipeline/brw_blorp_blit_eu.h"
#include "compiler/pipeline/brw_blorp.h"

enum sampler_param {
    SAMPLER_PARAM_HOLE,
    SAMPLER_PARAM_ZERO,
    SAMPLER_PARAM_X,
    SAMPLER_PARAM_Y,
    SAMPLER_PARAM_LAYER,
    SAMPLER_PARAM_LOD,
};

class intel_meta_compiler : public brw_blorp_eu_emitter
{
public:
    intel_meta_compiler(const struct intel_gpu *gpu,
                        struct brw_context *brw,
                        enum intel_dev_meta_shader id);
    void *compile(brw_blorp_prog_data *prog_data, uint32_t *code_size);

private:
    void alloc_regs()
    {
        int grf = base_grf;

        grf = alloc_pcb_regs(grf);
        grf = alloc_input_regs(grf);
        grf = alloc_temp_regs(grf);

        assert(grf <= 128);
    }

    int alloc_pcb_regs(int grf);
    int alloc_input_regs(int grf);
    int alloc_temp_regs(int grf);

    void emit_compute_frag_coord();
    void emit_sampler_payload(const struct brw_reg &mrf,
                              const enum sampler_param *params,
                              uint32_t param_count);

    void emit_vs_fill_mem();
    void emit_vs_copy_mem();
    void emit_vs_copy_img_to_mem();
    void emit_copy_mem();
    void emit_copy_img();
    void emit_copy_mem_to_img();

    void emit_clear_color();
    void emit_clear_depth();
    void *codegen(uint32_t *code_size);

    const struct intel_gpu *gpu;
    struct brw_context *brw;
    enum intel_dev_meta_shader id;

    const struct brw_reg poison;
    const struct brw_reg r0;
    const struct brw_reg r1;
    const int base_grf;
    const int base_mrf;

    /* pushed consts */
    struct brw_reg clear_vals[4];

    struct brw_reg src_offset_x;
    struct brw_reg src_offset_y;
    struct brw_reg src_layer;
    struct brw_reg src_lod;

    struct brw_reg dst_mem_offset;
    struct brw_reg dst_extent_width;

    /* inputs */
    struct brw_reg vid;

    /* temps */
    struct brw_reg frag_x;
    struct brw_reg frag_y;

    struct brw_reg texels[4];

    struct brw_reg temps[4];
};

intel_meta_compiler::intel_meta_compiler(const struct intel_gpu *gpu,
                                         struct brw_context *brw,
                                         enum intel_dev_meta_shader id)
    : brw_blorp_eu_emitter(brw), gpu(gpu), brw(brw), id(id),
      poison(brw_imm_ud(0x12345678)),
      r0(retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD)),
      r1(retype(brw_vec8_grf(1, 0), BRW_REGISTER_TYPE_UD)),
      base_grf(2), /* skipping r0 and r1 */
      base_mrf(1)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(clear_vals); i++)
        clear_vals[i] = poison;

    src_offset_x = poison;
    src_offset_y = poison;
    src_layer = poison;
    src_lod = poison;

    dst_mem_offset = poison;
    dst_extent_width = poison;

    vid = poison;
}

int intel_meta_compiler::alloc_pcb_regs(int grf)
{
    switch (id) {
    case INTEL_DEV_META_VS_FILL_MEM:
        dst_mem_offset = retype(brw_vec1_grf(grf, 0), BRW_REGISTER_TYPE_UD);
        clear_vals[0] = retype(brw_vec1_grf(grf, 1), BRW_REGISTER_TYPE_UD);
        break;
    case INTEL_DEV_META_VS_COPY_MEM:
    case INTEL_DEV_META_VS_COPY_MEM_UNALIGNED:
        dst_mem_offset = retype(brw_vec1_grf(grf, 0), BRW_REGISTER_TYPE_UD);
        src_offset_x = retype(brw_vec1_grf(grf, 1), BRW_REGISTER_TYPE_UD);
        break;
    case INTEL_DEV_META_VS_COPY_R8_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R16_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32G32_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32G32B32A32_TO_MEM:
        src_offset_x = retype(brw_vec1_grf(grf, 0), BRW_REGISTER_TYPE_UD);
        src_offset_y = retype(brw_vec1_grf(grf, 1), BRW_REGISTER_TYPE_UD);
        dst_extent_width = retype(brw_vec1_grf(grf, 2), BRW_REGISTER_TYPE_UD);
        dst_mem_offset = retype(brw_vec1_grf(grf, 3), BRW_REGISTER_TYPE_UD);
        break;
    case INTEL_DEV_META_FS_COPY_MEM:
    case INTEL_DEV_META_FS_COPY_1D:
    case INTEL_DEV_META_FS_COPY_1D_ARRAY:
    case INTEL_DEV_META_FS_COPY_2D:
    case INTEL_DEV_META_FS_COPY_2D_ARRAY:
    case INTEL_DEV_META_FS_COPY_2D_MS:
        src_offset_x = retype(brw_vec1_grf(grf, 0), BRW_REGISTER_TYPE_UD);
        src_offset_y = retype(brw_vec1_grf(grf, 1), BRW_REGISTER_TYPE_UD);
        src_layer = retype(brw_vec1_grf(grf, 2), BRW_REGISTER_TYPE_UD);
        src_lod = retype(brw_vec1_grf(grf, 3), BRW_REGISTER_TYPE_UD);
        break;
    case INTEL_DEV_META_FS_COPY_1D_TO_MEM:
    case INTEL_DEV_META_FS_COPY_1D_ARRAY_TO_MEM:
    case INTEL_DEV_META_FS_COPY_2D_TO_MEM:
    case INTEL_DEV_META_FS_COPY_2D_ARRAY_TO_MEM:
    case INTEL_DEV_META_FS_COPY_2D_MS_TO_MEM:
        src_offset_x = retype(brw_vec1_grf(grf, 0), BRW_REGISTER_TYPE_UD);
        src_offset_y = retype(brw_vec1_grf(grf, 1), BRW_REGISTER_TYPE_UD);
        src_layer = retype(brw_vec1_grf(grf, 2), BRW_REGISTER_TYPE_UD);
        src_lod = retype(brw_vec1_grf(grf, 3), BRW_REGISTER_TYPE_UD);
        dst_mem_offset = retype(brw_vec1_grf(grf, 4), BRW_REGISTER_TYPE_UD);
        dst_extent_width = retype(brw_vec1_grf(grf, 5), BRW_REGISTER_TYPE_UD);
        break;
    case INTEL_DEV_META_FS_COPY_MEM_TO_IMG:
        src_offset_x = retype(brw_vec1_grf(grf, 0), BRW_REGISTER_TYPE_UD);
        src_offset_y = retype(brw_vec1_grf(grf, 1), BRW_REGISTER_TYPE_UD);
        dst_extent_width = retype(brw_vec1_grf(grf, 2), BRW_REGISTER_TYPE_UD);
        break;
    case INTEL_DEV_META_FS_CLEAR_COLOR:
    case INTEL_DEV_META_FS_CLEAR_DEPTH:
        clear_vals[0] = retype(brw_vec1_grf(grf, 0), BRW_REGISTER_TYPE_UD);
        clear_vals[1] = retype(brw_vec1_grf(grf, 1), BRW_REGISTER_TYPE_UD);
        clear_vals[2] = retype(brw_vec1_grf(grf, 2), BRW_REGISTER_TYPE_UD);
        clear_vals[3] = retype(brw_vec1_grf(grf, 3), BRW_REGISTER_TYPE_UD);
        break;
    case INTEL_DEV_META_FS_RESOLVE_2X:
    case INTEL_DEV_META_FS_RESOLVE_4X:
    case INTEL_DEV_META_FS_RESOLVE_8X:
    case INTEL_DEV_META_FS_RESOLVE_16X:
        src_offset_x = retype(brw_vec1_grf(grf, 0), BRW_REGISTER_TYPE_UD);
        src_offset_y = retype(brw_vec1_grf(grf, 1), BRW_REGISTER_TYPE_UD);
        break;
    default:
        break;
    }

    return grf + 1;
}

int intel_meta_compiler::alloc_input_regs(int grf)
{
    switch (id) {
    case INTEL_DEV_META_VS_FILL_MEM:
    case INTEL_DEV_META_VS_COPY_MEM:
    case INTEL_DEV_META_VS_COPY_MEM_UNALIGNED:
    case INTEL_DEV_META_VS_COPY_R8_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R16_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32G32_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32G32B32A32_TO_MEM:
        vid = retype(brw_vec1_grf(grf, 0), BRW_REGISTER_TYPE_UD);
        break;
    default:
        break;
    }

    return grf + 1;
}

int intel_meta_compiler::alloc_temp_regs(int grf)
{
    int i;

    frag_x = retype(brw_vec8_grf(grf, 0), BRW_REGISTER_TYPE_UW);
    grf++;

    frag_y = retype(brw_vec8_grf(grf, 0), BRW_REGISTER_TYPE_UW);
    grf++;

    for (i = 0; i < ARRAY_SIZE(texels); i++) {
        texels[i] = retype(brw_vec8_grf(grf, 0), BRW_REGISTER_TYPE_UD);
        grf += 8;
    }

    for (i = 0; i < ARRAY_SIZE(temps); i++) {
        temps[i] = retype(brw_vec8_grf(grf, 0), BRW_REGISTER_TYPE_UD);
        grf += 2;
    }

    return grf;
}

void intel_meta_compiler::emit_compute_frag_coord()
{
    const struct brw_reg x = retype(suboffset(r1, 2), BRW_REGISTER_TYPE_UW);
    const struct brw_reg y = suboffset(x, 1);

    emit_add(frag_x, stride(x, 2, 4, 0), brw_imm_v(0x10101010));
    emit_add(frag_y, stride(y, 2, 4, 0), brw_imm_v(0x11001100));
}

void intel_meta_compiler::emit_sampler_payload(const struct brw_reg &mrf,
                                               const enum sampler_param *params,
                                               uint32_t param_count)
{
    int mrf_offset = 0;
    uint32_t i;

    for (i = 0; i < param_count; i++) {
        switch (params[i]) {
        case SAMPLER_PARAM_HOLE:
            break;
        case SAMPLER_PARAM_ZERO:
            emit_mov(offset(mrf, mrf_offset), brw_imm_ud(0));
            break;
        case SAMPLER_PARAM_X:
            emit_add(offset(mrf, mrf_offset), frag_x, src_offset_x);
            break;
        case SAMPLER_PARAM_Y:
            emit_add(offset(mrf, mrf_offset), frag_y, src_offset_y);
            break;
        case SAMPLER_PARAM_LAYER:
            emit_mov(offset(mrf, mrf_offset), src_layer);
            break;
        case SAMPLER_PARAM_LOD:
            emit_mov(offset(mrf, mrf_offset), src_lod);
            break;
        default:
            assert(!"unknown sampler parameter");
            break;
        }

        mrf_offset += 2;
    }
}

void intel_meta_compiler::emit_vs_fill_mem()
{
    const struct brw_reg mrf =
        retype(vec1(brw_message_reg(base_mrf)), BRW_REGISTER_TYPE_UD);
    int mrf_offset = 0;
    bool use_header;

    if (brw->gen >= 7) {
        use_header = false;
    } else {
        emit_mov_8(offset(vec8(mrf), mrf_offset), r0);
        mrf_offset += 1;

        use_header = true;
    }

    emit_add_8(offset(mrf, mrf_offset), dst_mem_offset, vid);
    mrf_offset += 1;

    emit_mov_8(offset(mrf, mrf_offset), clear_vals[0]);
    mrf_offset += 1;

    emit_scattered_write(SHADER_OPCODE_DWORD_SCATTERED_WRITE,
            mrf, base_mrf, mrf_offset, 1, use_header);

    emit_urb_write_eot(base_mrf);
}

void intel_meta_compiler::emit_vs_copy_mem()
{
    const struct brw_reg mrf =
        retype(vec1(brw_message_reg(base_mrf)), BRW_REGISTER_TYPE_UD);
    int mrf_offset = 0;
    enum opcode op_read, op_write;
    bool use_header;

    if (id == INTEL_DEV_META_VS_COPY_MEM) {
        op_read = SHADER_OPCODE_DWORD_SCATTERED_READ;
        op_write = SHADER_OPCODE_DWORD_SCATTERED_WRITE;
    } else {
        /* Byte Scattered Read/Write are Gen7+ only */
        if (brw->gen == 6) {
            emit_urb_write_eot(base_mrf);
            return;
        }
        op_read = SHADER_OPCODE_BYTE_SCATTERED_READ;
        op_write = SHADER_OPCODE_BYTE_SCATTERED_WRITE;
    }

    if (brw->gen >= 7) {
        use_header = false;
    } else {
        emit_mov_8(offset(vec8(mrf), mrf_offset), r0);
        mrf_offset += 1;

        use_header = true;
    }

    emit_add_8(offset(mrf, mrf_offset), src_offset_x, vid);
    mrf_offset += 1;
    emit_scattered_read(temps[0], op_read, mrf,
            base_mrf, mrf_offset, 1, use_header);

    /* prepare to set up dst offset */
    mrf_offset -= 1;

    emit_add_8(offset(mrf, mrf_offset), dst_mem_offset, vid);
    mrf_offset += 1;
    emit_mov_8(offset(mrf, mrf_offset), vec1(temps[0]));
    mrf_offset += 1;

    emit_scattered_write(op_write, mrf, base_mrf, mrf_offset, 1, use_header);

    emit_urb_write_eot(base_mrf);
}

void intel_meta_compiler::emit_vs_copy_img_to_mem()
{
    const struct brw_reg mrf =
        retype(brw_message_reg(base_mrf), BRW_REGISTER_TYPE_UD);
    int mrf_offset = 0;
    bool use_header;
    enum opcode op_write;
    int op_slot_count, i;

    switch (id) {
    case INTEL_DEV_META_VS_COPY_R8_TO_MEM:
        op_write = SHADER_OPCODE_BYTE_SCATTERED_WRITE;
        op_slot_count = 1;
        break;
    case INTEL_DEV_META_VS_COPY_R16_TO_MEM:
        op_write = SHADER_OPCODE_BYTE_SCATTERED_WRITE;
        op_slot_count = 2;
        break;
    case INTEL_DEV_META_VS_COPY_R32_TO_MEM:
        op_write = SHADER_OPCODE_DWORD_SCATTERED_WRITE;
        op_slot_count = 1;
        break;
    case INTEL_DEV_META_VS_COPY_R32G32_TO_MEM:
        op_write = SHADER_OPCODE_DWORD_SCATTERED_WRITE;
        op_slot_count = 2;
        break;
    case INTEL_DEV_META_VS_COPY_R32G32B32A32_TO_MEM:
        op_write = SHADER_OPCODE_DWORD_SCATTERED_WRITE;
        op_slot_count = 4;
        break;
    default:
        op_write = SHADER_OPCODE_DWORD_SCATTERED_WRITE;
        op_slot_count = 0;
        break;
    }

    if (!op_slot_count || (brw->gen == 6 &&
                           op_write == SHADER_OPCODE_BYTE_SCATTERED_WRITE)) {
        emit_urb_write_eot(base_mrf);
        return;
    }

    /* load image texel */
    emit_mov(temps[0], vid);
    emit_mov(temps[1], dst_extent_width);
    emit_irem(temps[2], temps[0], temps[1]);
    emit_idiv(temps[3], temps[0], temps[1]);
    if (brw->gen >= 7) {
        emit_add(offset(mrf, 0), vec1(src_offset_x), vec1(temps[2]));
        emit_mov(offset(mrf, 2), brw_imm_ud(0));
        emit_add(offset(mrf, 4), vec1(src_offset_y), vec1(temps[3]));
        mrf_offset = 6;
    } else {
        emit_add(offset(mrf, 0), vec1(src_offset_x), vec1(temps[2]));
        emit_add(offset(mrf, 2), vec1(src_offset_y), vec1(temps[3]));
        mrf_offset = 4;
    }
    emit_texture_lookup(vec1(texels[0]), SHADER_OPCODE_TXF, base_mrf, mrf_offset);

    mrf_offset = 0;
    if (brw->gen >= 7) {
        use_header = false;
    } else {
        emit_mov_8(offset(mrf, mrf_offset), r0);
        mrf_offset += 1;

        use_header = true;
    }

    /* offsets */
    if (op_slot_count == 1) {
        emit_add_8(offset(vec1(mrf), mrf_offset), dst_mem_offset, vid);
    } else {
        emit_mul_8(vec1(temps[0]), vid, brw_imm_ud(op_slot_count));
        emit_add_8(vec1(temps[0]), dst_mem_offset, vec1(temps[0]));

        emit_mov_8(offset(vec1(mrf), mrf_offset), vec1(temps[0]));
        for (i = 1; i < op_slot_count; i++) {
            emit_add_8(suboffset(offset(vec1(mrf), mrf_offset), i),
                    vec1(temps[0]), brw_imm_ud(i));
        }
    }
    mrf_offset += 1;

    /* values */
    if (id == INTEL_DEV_META_VS_COPY_R16_TO_MEM) {
        emit_mov(suboffset(offset(vec1(mrf), mrf_offset), 0),
                vec1(texels[0]));
        emit_shr(suboffset(offset(vec1(mrf), mrf_offset), 1),
                vec1(texels[0]), brw_imm_ud(8));
    } else {
        for (i = 0; i < op_slot_count; i++) {
            emit_mov_8(suboffset(offset(vec1(mrf), mrf_offset), i),
                    offset(vec1(texels[0]), 2 * i));
        }
    }
    mrf_offset += 1;

    emit_scattered_write(op_write, vec1(mrf), base_mrf, mrf_offset,
            op_slot_count, use_header);

    emit_urb_write_eot(base_mrf);
}

void intel_meta_compiler::emit_copy_mem()
{
    const struct brw_reg mrf =
        retype(brw_message_reg(base_mrf), BRW_REGISTER_TYPE_UD);

    emit_compute_frag_coord();
    emit_add(mrf, frag_x, src_offset_x);
    emit_texture_lookup(texels[0], SHADER_OPCODE_TXF, base_mrf, 2);

    emit_mov(offset(mrf, 0), offset(texels[0], 0));
    emit_mov(offset(mrf, 2), offset(texels[0], 2));
    emit_mov(offset(mrf, 4), offset(texels[0], 4));
    emit_mov(offset(mrf, 6), offset(texels[0], 6));
    emit_render_target_write(mrf, base_mrf, 8, false);
}

void intel_meta_compiler::emit_copy_img()
{
    const struct brw_reg mrf =
        retype(brw_message_reg(base_mrf), BRW_REGISTER_TYPE_UD);
    enum sampler_param params[8];
    uint32_t param_count = 0;

    if (brw->gen >= 7) {
        params[param_count++] = SAMPLER_PARAM_X;
        params[param_count++] = SAMPLER_PARAM_LOD;

        switch (id) {
        case INTEL_DEV_META_FS_COPY_1D:
            params[param_count++] = SAMPLER_PARAM_ZERO;
            break;
        case INTEL_DEV_META_FS_COPY_1D_ARRAY:
            params[param_count++] = SAMPLER_PARAM_LAYER;
            break;
        case INTEL_DEV_META_FS_COPY_2D:
            params[param_count++] = SAMPLER_PARAM_Y;
            params[param_count++] = SAMPLER_PARAM_ZERO;
            break;
        case INTEL_DEV_META_FS_COPY_2D_ARRAY:
            params[param_count++] = SAMPLER_PARAM_Y;
            params[param_count++] = SAMPLER_PARAM_LAYER;
            break;
        case INTEL_DEV_META_FS_COPY_2D_MS:
            /* TODO */
            break;
        default:
            break;
        }
    } else {
        params[param_count++] = SAMPLER_PARAM_X;

        switch (id) {
        case INTEL_DEV_META_FS_COPY_1D:
            params[param_count++] = SAMPLER_PARAM_ZERO;
            params[param_count++] = SAMPLER_PARAM_HOLE;
            break;
        case INTEL_DEV_META_FS_COPY_1D_ARRAY:
            params[param_count++] = SAMPLER_PARAM_LAYER;
            params[param_count++] = SAMPLER_PARAM_ZERO;
            break;
        case INTEL_DEV_META_FS_COPY_2D:
            params[param_count++] = SAMPLER_PARAM_Y;
            params[param_count++] = SAMPLER_PARAM_ZERO;
            break;
        case INTEL_DEV_META_FS_COPY_2D_ARRAY:
            params[param_count++] = SAMPLER_PARAM_Y;
            params[param_count++] = SAMPLER_PARAM_LAYER;
            break;
        case INTEL_DEV_META_FS_COPY_2D_MS:
            /* TODO */
            break;
        default:
            break;
        }

        params[param_count++] = SAMPLER_PARAM_LOD;
    }

    emit_compute_frag_coord();
    emit_sampler_payload(mrf, params, param_count);
    emit_texture_lookup(texels[0], SHADER_OPCODE_TXF,
            base_mrf, param_count * 2);

    emit_mov(offset(mrf, 0), offset(texels[0], 0));
    emit_mov(offset(mrf, 2), offset(texels[0], 2));
    emit_mov(offset(mrf, 4), offset(texels[0], 4));
    emit_mov(offset(mrf, 6), offset(texels[0], 6));
    emit_render_target_write(mrf, base_mrf, 10, false);
}

void intel_meta_compiler::emit_copy_mem_to_img()
{
    const struct brw_reg mrf =
        retype(brw_message_reg(base_mrf), BRW_REGISTER_TYPE_UD);

    emit_compute_frag_coord();
    emit_add(temps[0], frag_y, src_offset_y);
    emit_add(temps[1], frag_x, src_offset_x);

    /* convert (x, y) to linear offset */
    emit_mul(temps[2], temps[0], dst_extent_width);
    emit_add(mrf, temps[2], temps[1]);
    emit_texture_lookup(texels[0], SHADER_OPCODE_TXF, base_mrf, 2);

    emit_mov(offset(mrf, 0), offset(texels[0], 0));
    emit_mov(offset(mrf, 2), offset(texels[0], 2));
    emit_mov(offset(mrf, 4), offset(texels[0], 4));
    emit_mov(offset(mrf, 6), offset(texels[0], 6));
    emit_render_target_write(mrf, base_mrf, 8, false);
}

void intel_meta_compiler::emit_clear_color()
{
    const struct brw_reg mrf =
        retype(brw_message_reg(base_mrf), BRW_REGISTER_TYPE_UD);

    emit_mov(offset(mrf, 0), clear_vals[0]);
    emit_mov(offset(mrf, 2), clear_vals[1]);
    emit_mov(offset(mrf, 4), clear_vals[2]);
    emit_mov(offset(mrf, 6), clear_vals[3]);
    emit_render_target_write(mrf, base_mrf, 8, false);
}

void intel_meta_compiler::emit_clear_depth()
{
    const struct brw_reg mrf =
        retype(brw_message_reg(base_mrf), BRW_REGISTER_TYPE_UD);

    /* skip color and write oDepth only */
    emit_mov(offset(mrf, 8), clear_vals[0]);
    emit_render_target_write(mrf, base_mrf, 10, false);
}

void *intel_meta_compiler::codegen(uint32_t *code_size)
{
    const unsigned *prog;
    unsigned prog_size;
    void *code;

    prog = get_program(&prog_size, stderr);

    code = intel_alloc(gpu, prog_size, 0, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
    if (!code)
        return NULL;

    memcpy(code, prog, prog_size);
    if (code_size)
        *code_size = prog_size;

    return code;
}

void *intel_meta_compiler::compile(brw_blorp_prog_data *prog_data,
                                   uint32_t *code_size)
{
    memset(prog_data, 0, sizeof(*prog_data));
    prog_data->first_curbe_grf = base_grf;

    alloc_regs();

    switch (id) {
    case INTEL_DEV_META_VS_FILL_MEM:
        emit_vs_fill_mem();
        break;
    case INTEL_DEV_META_VS_COPY_MEM:
    case INTEL_DEV_META_VS_COPY_MEM_UNALIGNED:
        emit_vs_copy_mem();
        break;
    case INTEL_DEV_META_VS_COPY_R8_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R16_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32G32_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32G32B32A32_TO_MEM:
        emit_vs_copy_img_to_mem();
        break;
    case INTEL_DEV_META_FS_COPY_MEM:
        emit_copy_mem();
        break;
    case INTEL_DEV_META_FS_COPY_1D:
    case INTEL_DEV_META_FS_COPY_1D_ARRAY:
    case INTEL_DEV_META_FS_COPY_2D:
    case INTEL_DEV_META_FS_COPY_2D_ARRAY:
    case INTEL_DEV_META_FS_COPY_2D_MS:
        emit_copy_img();
        break;
    case INTEL_DEV_META_FS_COPY_1D_TO_MEM:
    case INTEL_DEV_META_FS_COPY_1D_ARRAY_TO_MEM:
    case INTEL_DEV_META_FS_COPY_2D_TO_MEM:
    case INTEL_DEV_META_FS_COPY_2D_ARRAY_TO_MEM:
    case INTEL_DEV_META_FS_COPY_2D_MS_TO_MEM:
        emit_clear_color();
        break;
    case INTEL_DEV_META_FS_COPY_MEM_TO_IMG:
        emit_copy_mem_to_img();
        break;
    case INTEL_DEV_META_FS_CLEAR_COLOR:
        emit_clear_color();
        break;
    case INTEL_DEV_META_FS_CLEAR_DEPTH:
        emit_clear_depth();
        break;
    case INTEL_DEV_META_FS_RESOLVE_2X:
    case INTEL_DEV_META_FS_RESOLVE_4X:
    case INTEL_DEV_META_FS_RESOLVE_8X:
    case INTEL_DEV_META_FS_RESOLVE_16X:
        emit_clear_color();
        break;
    default:
        emit_clear_color();
        break;
    }

    return codegen(code_size);
}

extern "C" {

VkResult intel_pipeline_shader_compile_meta(struct intel_pipeline_shader *sh,
                                              const struct intel_gpu *gpu,
                                              enum intel_dev_meta_shader id)
{
    struct brw_context *brw = intel_create_brw_context(gpu);

    intel_meta_compiler c(gpu, brw, id);
    brw_blorp_prog_data prog_data;

    sh->pCode = c.compile(&prog_data, &sh->codeSize);

    sh->in_count = 0;
    sh->out_count = 1;
    sh->uses = 0;
    sh->surface_count = BRW_BLORP_NUM_BINDING_TABLE_ENTRIES;
    sh->urb_grf_start = prog_data.first_curbe_grf;

    switch (id) {
    case INTEL_DEV_META_VS_FILL_MEM:
    case INTEL_DEV_META_VS_COPY_MEM:
    case INTEL_DEV_META_VS_COPY_MEM_UNALIGNED:
    case INTEL_DEV_META_VS_COPY_R8_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R16_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32G32_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32G32B32A32_TO_MEM:
        sh->in_count = 1;
        sh->uses |= INTEL_SHADER_USE_VID;
        break;
    case INTEL_DEV_META_FS_CLEAR_DEPTH:
        sh->uses |= INTEL_COMPUTED_DEPTH_MODE_ON;
        break;
    default:
        break;
    }

    ralloc_free(brw->shader_prog);
    ralloc_free(brw);

    return (sh->pCode) ? VK_SUCCESS : VK_ERROR_VALIDATION_FAILED_EXT;
}

} // extern "C"
