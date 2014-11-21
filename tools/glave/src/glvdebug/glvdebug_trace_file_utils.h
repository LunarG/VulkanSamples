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
#ifndef GLVDEBUG_TRACE_FILE_UTILS_H_
#define GLVDEBUG_TRACE_FILE_UTILS_H_

//#include <string>
#include <QString>

extern "C" {
#include "glv_trace_packet_identifiers.h"
}
#include "glvdebug_output.h"

struct glvdebug_trace_file_packet_offsets
{
    // the file offset to this particular packet
    unsigned int fileOffset;

    // Pointer to the packet header if it's been read from disk
    glv_trace_packet_header* pHeader;
};

struct glvdebug_trace_file_info
{
    // the trace file name & path
    char* filename;

    // the trace file
    FILE* pFile;

    // copy of the trace file header
    glv_trace_file_header header;

    // number of packets in file which should also be number of elements in pPacketOffsets array
    unsigned int packetCount;

    // array of packet offsets
    glvdebug_trace_file_packet_offsets* pPacketOffsets;
};

BOOL glvdebug_populate_trace_file_info(glvdebug_trace_file_info* pTraceFileInfo);

#endif //GLVDEBUG_TRACE_FILE_UTILS_H_
