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
#include "glvdebug_trace_file_utils.h"
#include "glv_memory.h"

BOOL glvdebug_populate_trace_file_info(glvdebug_trace_file_info* pTraceFileInfo)
{
    assert(pTraceFileInfo != NULL);
    assert(pTraceFileInfo->pFile != NULL);

    // read trace file header
    if (1 != fread(&(pTraceFileInfo->header), sizeof(glv_trace_file_header), 1, pTraceFileInfo->pFile))
    {
        glvdebug_output_error("Unable to read header from file.");
        return FALSE;
    }

    // Find out how many trace packets there are.

    // Seek to first packet
    long first_offset = pTraceFileInfo->header.first_packet_offset;
    int seekResult = fseek(pTraceFileInfo->pFile, first_offset, SEEK_SET);
    if (seekResult != 0)
    {
        glvdebug_output_warning("Failed to seek to the first packet offset in the trace file.");
    }

    uint64_t fileOffset = pTraceFileInfo->header.first_packet_offset;
    uint64_t packetSize = 0;
    while(1 == fread(&packetSize, sizeof(uint64_t), 1, pTraceFileInfo->pFile))
    {
        // success!
        pTraceFileInfo->packetCount++;
        fileOffset += packetSize;

        fseek(pTraceFileInfo->pFile, fileOffset, SEEK_SET);
    }

    if (pTraceFileInfo->packetCount == 0)
    {
        if (ferror(pTraceFileInfo->pFile) != 0)
        {
            perror("File Read error:");
            glvdebug_output_warning("There was an error reading the trace file.");
            return FALSE;
        }
        else if (feof(pTraceFileInfo->pFile) != 0)
        {
            glvdebug_output_warning("Reached the end of the file.");
        }
        glvdebug_output_warning("There are no trace packets in this trace file.");
        pTraceFileInfo->pPacketOffsets = NULL;
    }
    else
    {
        pTraceFileInfo->pPacketOffsets = GLV_NEW_ARRAY(glvdebug_trace_file_packet_offsets, pTraceFileInfo->packetCount);

        // rewind to first packet and this time, populate the packet offsets
        if (fseek(pTraceFileInfo->pFile, first_offset, SEEK_SET) != 0)
        {
            glvdebug_output_error("Unable to rewind trace file to gather packet offsets.");
            return FALSE;
        }

        unsigned int packetIndex = 0;
        fileOffset = first_offset;
        while(1 == fread(&packetSize, sizeof(uint64_t), 1, pTraceFileInfo->pFile))
        {
            // the fread confirms that this packet exists
            // NOTE: We do not actually read the entire packet into memory right now.
            pTraceFileInfo->pPacketOffsets[packetIndex].fileOffset = fileOffset;

            // rewind slightly
            fseek(pTraceFileInfo->pFile, -1*(long)sizeof(uint64_t), SEEK_CUR);

            // allocate space for the packet and read it in
            pTraceFileInfo->pPacketOffsets[packetIndex].pHeader = (glv_trace_packet_header*)glv_malloc(packetSize);
            if (1 != fread(pTraceFileInfo->pPacketOffsets[packetIndex].pHeader, packetSize, 1, pTraceFileInfo->pFile))
            {
                glvdebug_output_error("Unable to read in a trace packet.");
                return FALSE;
            }

            // adjust pointer to body of the packet
            pTraceFileInfo->pPacketOffsets[packetIndex].pHeader->pBody = (uintptr_t)pTraceFileInfo->pPacketOffsets[packetIndex].pHeader + sizeof(glv_trace_packet_header);

            // now seek to what should be the next packet
            fileOffset += packetSize;
            packetIndex++;
        }

        if (fseek(pTraceFileInfo->pFile, first_offset, SEEK_SET) != 0)
        {
            glvdebug_output_error("Unable to rewind trace file to restore position.");
            return FALSE;
        }
    }

    return TRUE;
}
