// Copyright 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Verify XGL driver initialization
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <xgl.h>

#include "gtest/gtest.h"
#include "common.h"


// Tests Factorial().

// Tests factorial of negative numbers.
TEST(Initialization, xglInitAndEnumerateGpus) {
    static const XGL_APPLICATION_INFO app_info = {
        .sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pAppName = (const XGL_CHAR *) "init",
        .appVersion = 1,
        .pEngineName = (const XGL_CHAR *) "xglInitAndEnumerateGpus",
        .engineVersion = 1,
        .apiVersion = XGL_MAKE_VERSION(0, 22, 0),
    };
    struct app_gpu gpus[MAX_GPUS];
    struct app_gpu *gpu;
    XGL_PHYSICAL_GPU objs[MAX_GPUS];
    XGL_UINT gpu_count, i;
    XGL_SIZE size;
    XGL_RESULT err;

    err = xglInitAndEnumerateGpus(&app_info, NULL,
            MAX_GPUS, &gpu_count, objs);
    ASSERT_EQ(XGL_SUCCESS, err) << xgl_result_string(err);

    gpu = &gpus[0];
    memset(gpu, 0, sizeof(*gpu));

    gpu->id = 0;
    gpu->obj = objs[0];

    err = xglGetGpuInfo(gpu->obj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES,
                        &size, &gpu->props);
}
