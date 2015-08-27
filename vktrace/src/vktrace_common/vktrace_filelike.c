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

#include "vktrace_filelike.h"
#include "vktrace_common.h"
#include "vktrace_interconnect.h"
#include <assert.h>
#include <stdlib.h>

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Checkpoint* vktrace_Checkpoint_create(const char* _str)
{
    Checkpoint* pCheckpoint = VKTRACE_NEW(Checkpoint);
    pCheckpoint->mToken = _str;
    pCheckpoint->mTokenLength = strlen(_str) + 1;
    return pCheckpoint;
}

// ------------------------------------------------------------------------------------------------
void vktrace_Checkpoint_write(Checkpoint* pCheckpoint, FileLike* _out)
{
    vktrace_FileLike_Write(_out, pCheckpoint->mToken, pCheckpoint->mTokenLength);
}

// ------------------------------------------------------------------------------------------------
BOOL vktrace_Checkpoint_read(Checkpoint* pCheckpoint, FileLike* _in)
{
    if (pCheckpoint->mTokenLength < 64) {
        char buffer[64];
        vktrace_FileLike_Read(_in, buffer, pCheckpoint->mTokenLength);
        if (strcmp(buffer, pCheckpoint->mToken) != 0) {
            return FALSE;
        }
    } else {
        char* buffer = VKTRACE_NEW_ARRAY(char, pCheckpoint->mTokenLength);
        vktrace_FileLike_Read(_in, buffer, pCheckpoint->mTokenLength);
        if (strcmp(buffer, pCheckpoint->mToken) != 0) {
            VKTRACE_DELETE(buffer);
            return FALSE;
        }
        VKTRACE_DELETE(buffer);
    }
    return TRUE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
FileLike* vktrace_FileLike_create_file(FILE* fp)
{
    FileLike* pFile = NULL;
    if (fp != NULL)
    {
        pFile = VKTRACE_NEW(FileLike);
        pFile->mMode = File;
        pFile->mFile = fp;
        pFile->mMessageStream = NULL;
    }
    return pFile;
}

// ------------------------------------------------------------------------------------------------
FileLike* vktrace_FileLike_create_msg(MessageStream* _msgStream)
{
    FileLike* pFile = NULL;
    if (_msgStream != NULL)
    {
        pFile = VKTRACE_NEW(FileLike);
        pFile->mMode = Socket;
        pFile->mFile = NULL;
        pFile->mMessageStream = _msgStream;
    }
    return pFile;
}

// ------------------------------------------------------------------------------------------------
size_t vktrace_FileLike_Read(FileLike* pFileLike, void* _bytes, size_t _len)
{
    size_t minSize = 0;
    size_t bytesInStream = 0;
    if (vktrace_FileLike_ReadRaw(pFileLike, &bytesInStream, sizeof(bytesInStream)) == FALSE)
        return 0;

    minSize = (_len < bytesInStream) ? _len: bytesInStream;
    if (bytesInStream > 0) {
        assert(_len >= bytesInStream);
        if (vktrace_FileLike_ReadRaw(pFileLike, _bytes, minSize) == FALSE)
            return 0;
    }

    return minSize;
}

// ------------------------------------------------------------------------------------------------
BOOL vktrace_FileLike_ReadRaw(FileLike* pFileLike, void* _bytes, size_t _len)
{
    BOOL result = TRUE;
    assert((pFileLike->mFile != 0) ^ (pFileLike->mMessageStream != 0));

    switch(pFileLike->mMode) {
    case File:
        {
            if (1 != fread(_bytes, _len, 1, pFileLike->mFile))
            {
                if (ferror(pFileLike->mFile) != 0)
                {
                    perror("fread error");
                }
                else if (feof(pFileLike->mFile) != 0)
                {
                    vktrace_LogWarning("Reached end of file.");
                }
                result = FALSE;
            } 
            break;
        }
    case Socket:
        {
            result = vktrace_MessageStream_BlockingRecv(pFileLike->mMessageStream, _bytes, _len);
            break;
        }

        default: 
            assert(!"Invalid mode in FileLike_ReadRaw");
            result = FALSE;
    }
    return result;
}

void vktrace_FileLike_Write(FileLike* pFileLike, const void* _bytes, size_t _len)
{
    vktrace_FileLike_WriteRaw(pFileLike, &_len, sizeof(_len));
    if (_len) {
        vktrace_FileLike_WriteRaw(pFileLike, _bytes, _len);
    }
}

// ------------------------------------------------------------------------------------------------
BOOL vktrace_FileLike_WriteRaw(FileLike* pFile, const void* _bytes, size_t _len)
{
    BOOL result = TRUE;
    assert((pFile->mFile != 0) ^ (pFile->mMessageStream != 0));
    switch (pFile->mMode)
    {
        case File:
            if (1 != fwrite(_bytes, _len, 1, pFile->mFile))
            {
                result = FALSE;
            }
            break;
        case Socket:
            result = vktrace_MessageStream_Send(pFile->mMessageStream, _bytes, _len);
            break;
        default:
            assert(!"Invalid mode in FileLike_WriteRaw");
            result = FALSE;
            break;
    }
    return result;
}
