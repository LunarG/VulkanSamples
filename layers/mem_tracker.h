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
// Mem Tracker ERROR codes
typedef enum _MEM_TRACK_ERROR
{
    MEMTRACK_NONE                          = 0, // Used for INFO & other non-error messages
    MEMTRACK_INVALID_CB                    = 1, // Cmd Buffer invalid
    MEMTRACK_CB_MISSING_MEM_REF            = 2, // pMemRefs for CB is missing a mem ref
    MEMTRACK_INVALID_MEM_OBJ               = 3, // Invalid Memory Object
    MEMTRACK_INTERNAL_ERROR                = 4, // Bug in Mem Track Layer internal data structures
    MEMTRACK_CB_MISSING_FENCE              = 5, // Cmd Buffer does not have fence
    MEMTRACK_FREED_MEM_REF                 = 6, // MEM Obj freed while it still has obj and/or CB refs
    MEMTRACK_MEM_OBJ_CLEAR_EMPTY_BINDINGS  = 7, // Clearing bindings on mem obj that doesn't have any bindings
    MEMTRACK_MISSING_MEM_BINDINGS          = 8, // Trying to retrieve mem bindings, but none found (may be internal error)
    MEMTRACK_INVALID_OBJECT                = 9, // Attempting to reference generic XGL Object that is invalid
    MEMTRACK_FREE_MEM_ERROR                = 10, // Error while calling xglFreeMemory
    MEMTRACK_DESTROY_OBJECT_ERROR          = 11, // Destroying an object that has a memory reference
    MEMTRACK_MEMORY_BINDING_ERROR          = 12, // Error during one of many calls that bind memory to object or CB
    MEMTRACK_OUT_OF_MEMORY_ERROR           = 13, // malloc failed
} MEM_TRACK_ERROR;

