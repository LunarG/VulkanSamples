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
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 */
#include "vulkan/vk_layer.h"
#include "vulkan/vk_lunarg_debug_report.h"
#include "vk_layer_logging.h"

/* Shader checker error codes */
typedef enum _SHADER_CHECKER_ERROR
{
    SHADER_CHECKER_NONE,
    SHADER_CHECKER_FS_MIXED_BROADCAST,      /* FS writes broadcast output AND custom outputs */
    SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, /* Type mismatch between shader stages or shader and pipeline */
    SHADER_CHECKER_OUTPUT_NOT_CONSUMED,     /* Entry appears in output interface, but missing in input */
    SHADER_CHECKER_INPUT_NOT_PRODUCED,      /* Entry appears in input interface, but missing in output */
    SHADER_CHECKER_NON_SPIRV_SHADER,        /* Shader image is not SPIR-V */
    SHADER_CHECKER_INCONSISTENT_SPIRV,      /* General inconsistency within a SPIR-V module */
    SHADER_CHECKER_UNKNOWN_STAGE,           /* Stage is not supported by analysis */
    SHADER_CHECKER_INCONSISTENT_VI,         /* VI state contains conflicting binding or attrib descriptions */
    SHADER_CHECKER_MISSING_DESCRIPTOR,      /* Shader attempts to use a descriptor binding not declared in the layout */
} SHADER_CHECKER_ERROR;
