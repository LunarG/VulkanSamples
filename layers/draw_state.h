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
#include "xglLayer.h"
// Draw State ERROR codes
typedef enum _DRAW_STATE_ERROR
{
    DRAWSTATE_NONE                          = 0, // Used for INFO & other non-error messages
    DRAWSTATE_DESCRIPTOR_MAX_EXCEEDED       = 1, // Descriptor Count of DS Mapping exceeds MAX_SLOTS
    DRAWSTATE_SLOT_REMAPPING                = 2, // DS Slot being mapped to a different type than previously
    DRAWSTATE_NO_PIPELINE_BOUND             = 3, // Unable to identify a bound pipeline
    DRAWSTATE_NO_DS_BOUND                   = 4, // Unable to identify a bound DS
    DRAWSTATE_DS_SLOT_NUM_MISMATCH          = 5, // Number of slots in DS mapping exceeds actual DS slots
    DRAWSTATE_UNKNOWN_DS_MAPPING            = 6, // Shader slot mapping is not recognized
    DRAWSTATE_DS_MAPPING_MISMATCH           = 7, // DS Mapping mismatch
    DRAWSTATE_INVALID_DS                    = 8, // Invalid DS referenced
    DRAWSTATE_DS_END_WITHOUT_BEGIN          = 9, // EndDSUpdate called w/o corresponding BeginDSUpdate
    DRAWSTATE_DS_ATTACH_WITHOUT_BEGIN       = 10, // Attempt to attach descriptors to DS w/ calling BeginDSUpdate
    DRAWSTATE_DS_SAMPLE_ATTACH_FAILED       = 11, // Error while attempting to Attach Sampler mapping to DS Slot
    DRAWSTATE_DS_IMAGE_ATTACH_FAILED        = 12, // Error while attempting to Attach Image mapping to DS Slot
    DRAWSTATE_DS_MEMORY_ATTACH_FAILED       = 13, // Error while attempting to Attach Mem mapping to DS Slot
    DRAWSTATE_DS_NESTED_DS_ATTACH_FAILED    = 14, // Error while attempting to Attach Nested DS mapping to DS Slot
    DRAWSTATE_CLEAR_DS_FAILED               = 15, // Error while attempting ClearDS
    DRAWSTATE_INVALID_PIPELINE              = 16, // Invalid DS referenced
} DRAW_STATE_ERROR;

typedef enum _DRAW_TYPE
{
    DRAW                  = 0,
    DRAW_INDEXED          = 1,
    DRAW_INDIRECT         = 2,
    DRAW_INDEXED_INDIRECT = 3,
    DRAW_BEGIN_RANGE      = DRAW,
    DRAW_END_RANGE        = DRAW_INDEXED_INDIRECT,
    NUM_DRAW_TYPES        = (DRAW_END_RANGE - DRAW_BEGIN_RANGE + 1),
} DRAW_TYPE;
