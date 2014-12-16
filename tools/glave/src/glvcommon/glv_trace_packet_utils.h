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
#pragma once

#include "glv_trace_packet_identifiers.h"
#include "glv_filelike.h"
#include "glv_memory.h"
#include "glv_process.h"

// pUuid is expected to be an array of 4 unsigned ints
void glv_gen_uuid(uint32_t* pUuid);

BOOL glv_init_time();
uint64_t glv_get_time();

//=============================================================================
// trace file header

// there is a file header at the start of every trace file
glv_trace_file_header* glv_create_trace_file_header();

// deletes the trace file header and sets pointer to NULL
void glv_delete_trace_file_header(glv_trace_file_header** ppHeader);

static FILE* glv_write_trace_file_header(glv_process_info* pProcInfo)
{
    FILE* tracefp = NULL;
    unsigned int index = 0;
    glv_trace_file_header* pHeader = NULL;
    size_t items_written = 0;
    assert(pProcInfo != NULL);

    // open trace file
    tracefp = fopen(pProcInfo->traceFilename, "wb");
    if (tracefp == NULL)
    {
        glv_LogError("Cannot open trace file for writing %s\n", pProcInfo->traceFilename);
        return tracefp;
    }

    // populate header information
    pHeader = glv_create_trace_file_header();
    pHeader->first_packet_offset = sizeof(glv_trace_file_header);
    pHeader->tracer_count = pProcInfo->tracerCount;

    for (index = 0; index < pProcInfo->tracerCount; index++)
    {
        pHeader->tracer_id_array[index].id = pProcInfo->pCaptureThreads[index].tracerId;
        pHeader->tracer_id_array[index].is_64_bit = (sizeof(intptr_t) == 8) ? 1 : 0;
    }

    pHeader->contains_state_snapshot = FALSE;
    pHeader->state_snapshot_size = 0;
    pHeader->state_snapshot_binary = (intptr_t) NULL;
    pHeader->state_snapshot_path = (intptr_t) NULL;

    // create critical section
    glv_create_critical_section(&pProcInfo->traceFileCriticalSection);

    // write header into file
    glv_enter_critical_section(&pProcInfo->traceFileCriticalSection);
    items_written = fwrite(pHeader, sizeof(glv_trace_file_header), 1, tracefp);
    glv_leave_critical_section(&pProcInfo->traceFileCriticalSection);
    if (items_written != 1)
    {
        glv_LogError("Failed to write trace file header\n");
    }
    glv_delete_trace_file_header(&pHeader);
    return tracefp;
};


//=============================================================================
// trace packets
// There is a trace_packet_header before every trace_packet_body.
// Additional buffers will come after the trace_packet_body.

//=============================================================================
// Methods for creating, populating, and writing trace packets

// \param packet_size should include the total bytes for the specific type of packet, and any additional buffers needed by the packet.
//        The size of the header will be added automatically within the function.
glv_trace_packet_header* glv_create_trace_packet(uint8_t tracer_id, uint16_t packet_id, uint64_t packet_size, uint64_t additional_buffers_size);

// deletes a trace packet and sets pointer to NULL
void glv_delete_trace_packet(glv_trace_packet_header** ppHeader);

// gets the next address available to write a buffer into the packet
void* glv_trace_packet_get_new_buffer_address(glv_trace_packet_header* pHeader, uint64_t byteCount);

// copies buffer data into a trace packet at the specified offset (from the end of the header).
// it is up to the caller to ensure that buffers do not overlap.
void glv_add_buffer_to_trace_packet(glv_trace_packet_header* pHeader, void** ptr_address, uint64_t size, const void* pBuffer);

// converts buffer pointers into byte offset so that pointer can be interpretted after being read into memory
void glv_finalize_buffer_address(glv_trace_packet_header* pHeader, void** ptr_address);

//void initialize_trace_packet_header(glv_trace_packet_header* pHeader, uint8_t tracer_id, uint16_t packet_id, uint64_t total_packet_size);
void glv_finalize_trace_packet(glv_trace_packet_header* pHeader);

// Write the trace packet to the filelike thing.
// This has no knowledge of the details of the packet other than its size.
void glv_write_trace_packet(const glv_trace_packet_header* pHeader, FileLike* pFile);

//=============================================================================
// Methods for Reading and interpretting trace packets

// Reads in the trace packet header, the body of the packet, and additional buffers
glv_trace_packet_header* glv_read_trace_packet(FileLike* pFile);

// converts a pointer variable that is currently byte offset into a pointer to the actual offset location
void* glv_trace_packet_interpret_buffer_pointer(glv_trace_packet_header* pHeader, intptr_t ptr_variable);

//=============================================================================
// trace packet message
// Interpretting a trace_packet_message should be done only when:
// 1) a trace_packet is first created and most of the contents are empty.
// 2) immediately after the packet was read from memory
// All other conversions of the trace packet body from the header should 
// be performed using a C-style cast.
static glv_trace_packet_message* glv_interpret_body_as_trace_packet_message(glv_trace_packet_header* pHeader)
{
    glv_trace_packet_message* pPacket = (glv_trace_packet_message*)pHeader->pBody;
    // update pointers
    pPacket->pHeader = pHeader;
    pPacket->message = (char*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->message);
    return pPacket;
}
