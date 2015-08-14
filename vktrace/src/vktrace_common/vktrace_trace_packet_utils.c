/**************************************************************************
 *
 * Copyright 2014 Valve Software
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/
#include "vktrace_trace_packet_utils.h"
#include "vktrace_interconnect.h"
#include "vktrace_filelike.h"
#ifdef WIN32
#include <rpc.h>
#pragma comment (lib, "Rpcrt4.lib")
#endif

#if defined(PLATFORM_LINUX)
#include <fcntl.h>
#include <time.h>
#endif

static uint64_t g_packet_index = 0;
static int g_reliable_rdtsc = -1;

void glv_gen_uuid(uint32_t* pUuid)
{
    uint32_t buf[] = { 0xABCDEF, 0x12345678, 0xFFFECABC, 0xABCDDEF0 };
    glv_platform_rand_s(buf, sizeof(buf)/sizeof(uint32_t));
    
    pUuid[0] = buf[0];
    pUuid[1] = buf[1];
    pUuid[2] = buf[2];
    pUuid[3] = buf[3];
}

BOOL glv_init_time()
{
#if defined(PLATFORM_LINUX)
    if (g_reliable_rdtsc == -1)
    {
        g_reliable_rdtsc = 0;

        FILE *file = fopen("/sys/devices/system/clocksource/clocksource0/current_clocksource", "r");
        if (file)
        {
            char buf[64];

            if (fgets(buf, sizeof(buf), file))
            {
                if (buf[0] == 't' && buf[1] == 's' && buf[2] == 'c')
                    g_reliable_rdtsc = 1;
            }

            fclose(file);
        }
    }

    // return true for reliable rdtsc.
    return !!g_reliable_rdtsc;
#else
    return TRUE;
#endif
}

uint64_t glv_get_time()
{
#if defined(GLV_USE_LINUX_API)
    extern int g_reliable_rdtsc;
    if (g_reliable_rdtsc == -1)
        init_rdtsc();
    if (g_reliable_rdtsc == 0)
    {
        //$ TODO: Should just use SDL_GetPerformanceCounter?
        struct timespec time;
        clock_gettime(CLOCK_MONOTONIC, &time);
        return ((uint64_t)time.tv_sec * 1000000000) + time.tv_nsec;
    }
#elif defined(COMPILER_GCCLIKE)
    unsigned int hi, lo;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#else
    return __rdtsc();
#endif
}

//=============================================================================
// trace file header

glv_trace_file_header* glv_create_trace_file_header()
{
    glv_trace_file_header* pHeader = GLV_NEW(glv_trace_file_header);
    memset(pHeader, 0, sizeof(glv_trace_file_header));
    pHeader->trace_file_version = GLV_TRACE_FILE_VERSION;
    glv_gen_uuid(pHeader->uuid);
    pHeader->trace_start_time = glv_get_time();

    return pHeader;
}

void glv_delete_trace_file_header(glv_trace_file_header** ppHeader)
{
    glv_free(*ppHeader);
    *ppHeader = NULL;
}

//=============================================================================
// Methods for creating, populating, and writing trace packets

glv_trace_packet_header* glv_create_trace_packet(uint8_t tracer_id, uint16_t packet_id, uint64_t packet_size, uint64_t additional_buffers_size)
{
    // Always allocate at least enough space for the packet header
    uint64_t total_packet_size = sizeof(glv_trace_packet_header) + packet_size + additional_buffers_size;
    void* pMem = glv_malloc((size_t)total_packet_size);
    memset(pMem, 0, (size_t)total_packet_size);

    glv_trace_packet_header* pHeader = (glv_trace_packet_header*)pMem;
    pHeader->size = total_packet_size;
    pHeader->global_packet_index = g_packet_index++;
    pHeader->tracer_id = tracer_id;
    pHeader->thread_id = glv_platform_get_thread_id();
    pHeader->packet_id = packet_id;
    pHeader->glave_begin_time = glv_get_time();
    pHeader->entrypoint_begin_time = pHeader->glave_begin_time;
    pHeader->entrypoint_end_time = 0;
    pHeader->glave_end_time = 0;
    pHeader->next_buffers_offset = sizeof(glv_trace_packet_header) + packet_size; // initial offset is from start of header to after the packet body
    if (total_packet_size > sizeof(glv_trace_packet_header))
    {
        pHeader->pBody = (uintptr_t)(((char*)pMem) + sizeof(glv_trace_packet_header));
    }
    return pHeader;
}

void glv_delete_trace_packet(glv_trace_packet_header** ppHeader)
{
    if (ppHeader == NULL)
        return;
    if (*ppHeader == NULL)
        return;

    GLV_DELETE(*ppHeader);
    *ppHeader = NULL;
}

void* glv_trace_packet_get_new_buffer_address(glv_trace_packet_header* pHeader, uint64_t byteCount)
{
    void* pBufferStart;
    assert(byteCount > 0);
    assert(pHeader->size >= pHeader->next_buffers_offset + byteCount);
    if (pHeader->size < pHeader->next_buffers_offset + byteCount || byteCount == 0)
    {
        // not enough memory left in packet to hold buffer
        // or request is for 0 bytes
        return NULL;
    }

    pBufferStart = (void*)((char*)pHeader + pHeader->next_buffers_offset);
    pHeader->next_buffers_offset += byteCount;
    return pBufferStart;
}

void glv_add_buffer_to_trace_packet(glv_trace_packet_header* pHeader, void** ptr_address, uint64_t size, const void* pBuffer)
{
    assert(ptr_address != NULL);
    if (pBuffer == NULL || size == 0)
    {
        *ptr_address = NULL;
    }
    else
    {
        // set ptr to the location of the added buffer
        *ptr_address = glv_trace_packet_get_new_buffer_address(pHeader, size);

        // copy buffer to the location
        memcpy(*ptr_address, pBuffer, (size_t)size);
    }
}

void glv_finalize_buffer_address(glv_trace_packet_header* pHeader, void** ptr_address)
{
    assert(ptr_address != NULL);

    if (*ptr_address != NULL)
    {
        // turn ptr into an offset from the packet body
        uint64_t offset = (uint64_t)*ptr_address - (uint64_t) (pHeader->pBody);
        *ptr_address = (void*)offset;
    }
}

void glv_set_packet_entrypoint_end_time(glv_trace_packet_header* pHeader)
{
    pHeader->entrypoint_end_time = glv_get_time();
}

void glv_finalize_trace_packet(glv_trace_packet_header* pHeader)
{
    if (pHeader->entrypoint_end_time == 0)
    {
        glv_set_packet_entrypoint_end_time(pHeader);
    }
    pHeader->glave_end_time = glv_get_time();
}

void glv_write_trace_packet(const glv_trace_packet_header* pHeader, FileLike* pFile)
{
    BOOL res = glv_FileLike_WriteRaw(pFile, pHeader, (size_t)pHeader->size);
    if (!res)
        glv_LogError("Failed to send trace packet index %u packetId %u size %u.", pHeader->global_packet_index, pHeader->packet_id, pHeader->size);
}

//=============================================================================
// Methods for Reading and interpretting trace packets

glv_trace_packet_header* glv_read_trace_packet(FileLike* pFile)
{
    // read size
    // allocate space
    // offset to after size
    // read the rest of the packet
    uint64_t total_packet_size = 0;
    void* pMem;
    glv_trace_packet_header* pHeader;

    if (glv_FileLike_ReadRaw(pFile, &total_packet_size, sizeof(uint64_t)) == FALSE)
    {
        //glv_LogError("Failed to read trace packet size.");
        return NULL;
    }

    // allocate space
    pMem = glv_malloc((size_t)total_packet_size);
    pHeader = (glv_trace_packet_header*)pMem;

    if (pHeader != NULL)
    {
        pHeader->size = total_packet_size;
        if (glv_FileLike_ReadRaw(pFile, (char*)pHeader + sizeof(uint64_t), (size_t)total_packet_size - sizeof(uint64_t)) == FALSE)
        {
            glv_LogError("Failed to read trace packet with size of %u.", total_packet_size);
            return NULL;
        }

        pHeader->pBody = (uintptr_t)pHeader + sizeof(glv_trace_packet_header);
    }
    else {
        glv_LogError("Malloc failed in glv_read_trace_packet of size %u.", total_packet_size);
    }

    return pHeader;
}

void* glv_trace_packet_interpret_buffer_pointer(glv_trace_packet_header* pHeader, intptr_t ptr_variable)
{
    // the pointer variable actually contains a byte offset from the packet body to the start of the buffer.
    uint64_t offset = ptr_variable;
    void* buffer_location;

    // if the offset is 0, then we know the pointer to the buffer was NULL, so no buffer exists and we return NULL.
    if (offset == 0)
        return NULL;

    buffer_location = (char*)(pHeader->pBody) + offset;
    return buffer_location;
}
