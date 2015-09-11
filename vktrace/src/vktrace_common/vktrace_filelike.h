/*
 * Copyright (c) 2013, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2014, Valve Software. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#endif

#include "vktrace_common.h"
#include "vktrace_interconnect.h"

typedef struct MessageStream MessageStream;

struct FileLike;
typedef struct FileLike FileLike;
typedef struct FileLike
{
    enum { File, Socket } mMode;
    FILE* mFile;
    MessageStream* mMessageStream;
} FileLike;

// For creating checkpoints (consistency checks) in the various streams we're interacting with.
typedef struct Checkpoint
{
    const char* mToken;
    size_t mTokenLength;
} Checkpoint;

#ifdef __cplusplus
extern "C" {
#endif

Checkpoint* vktrace_Checkpoint_create(const char* _str);
void vktrace_Checkpoint_write(Checkpoint* pCheckpoint, FileLike* _out);
BOOL vktrace_Checkpoint_read(Checkpoint* pCheckpoint, FileLike* _in);

// An interface for interacting with sockets, files, and memory streams with a file-like interface.
// This is a simple file-like interface--it doesn't support rewinding or anything fancy, just fifo 
// reads and writes.

// create a filelike interface for file streaming
FileLike* vktrace_FileLike_create_file(FILE* fp);

// create a filelike interface for network streaming
FileLike* vktrace_FileLike_create_msg(MessageStream* _msgStream);

// read a size and then a buffer of that size
size_t vktrace_FileLike_Read(FileLike* pFileLike, void* _bytes, size_t _len);

// Normally, Read expects the size to live in the stream prefixing the data to be read.
// With ReadRaw, no size is expected first, and the bytes are directly read.
BOOL vktrace_FileLike_ReadRaw(FileLike* pFileLike, void* _bytes, size_t _len);

// write _len and then the buffer of size _len
void vktrace_FileLike_Write(FileLike* pFileLike, const void* _bytes, size_t _len);

// Normally, Write outputs the _len to the stream first--with WriteRaw the bytes are simply written, 
// no size parameter first.
BOOL vktrace_FileLike_WriteRaw(FileLike* pFile, const void* _bytes, size_t _len);

#ifdef __cplusplus
}
#endif