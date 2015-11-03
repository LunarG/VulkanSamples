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
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Peter Lohrmann <peterl@valvesoftware.com>
 **************************************************************************/
#pragma once

#include "vktrace_common.h"

#define VKTRACE_TRACE_FILE_VERSION_2 0x0002
#define VKTRACE_TRACE_FILE_VERSION_3 0x0003
#define VKTRACE_TRACE_FILE_VERSION VKTRACE_TRACE_FILE_VERSION_3
#define VKTRACE_TRACE_FILE_VERSION_MINIMUM_COMPATIBLE VKTRACE_TRACE_FILE_VERSION_3

#define VKTRACE_MAX_TRACER_ID_ARRAY_SIZE 14

typedef enum VKTRACE_TRACER_ID
{
    VKTRACE_TID_RESERVED = 0,
    VKTRACE_TID_GL_FPS,
    VKTRACE_TID_VULKAN
    // Max enum must be less than VKTRACE_MAX_TRACER_ID_ARRAY_SIZE
} VKTRACE_TRACER_ID;

typedef struct VKTRACE_TRACER_REPLAYER_INFO
{
    VKTRACE_TRACER_ID tracerId;
    BOOL needsReplayer;
    const char* const replayerLibraryName;
    const char* const debuggerLibraryname;
} VKTRACE_TRACER_REPLAYER_INFO;

// The index here should match the value of the VKTRACE_TRACER_ID
static const VKTRACE_TRACER_REPLAYER_INFO gs_tracerReplayerInfo[VKTRACE_MAX_TRACER_ID_ARRAY_SIZE] = {
    {VKTRACE_TID_RESERVED, FALSE, "", ""},
    {VKTRACE_TID_GL_FPS, FALSE, "", ""},
    {VKTRACE_TID_VULKAN, TRUE, VKTRACE_LIBRARY_NAME(vulkan_replay), VKTRACE_LIBRARY_NAME(vktracedebug_vk)},
    {VKTRACE_TID_RESERVED, FALSE, "", ""}, // this can be updated as new tracers are added
    {VKTRACE_TID_RESERVED, FALSE, "", ""}, // this can be updated as new tracers are added
    {VKTRACE_TID_RESERVED, FALSE, "", ""}, // this can be updated as new tracers are added
    {VKTRACE_TID_RESERVED, FALSE, "", ""}, // this can be updated as new tracers are added
    {VKTRACE_TID_RESERVED, FALSE, "", ""}, // this can be updated as new tracers are added
    {VKTRACE_TID_RESERVED, FALSE, "", ""}, // this can be updated as new tracers are added
    {VKTRACE_TID_RESERVED, FALSE, "", ""}, // this can be updated as new tracers are added
    {VKTRACE_TID_RESERVED, FALSE, "", ""}, // this can be updated as new tracers are added
    {VKTRACE_TID_RESERVED, FALSE, "", ""}, // this can be updated as new tracers are added
    {VKTRACE_TID_RESERVED, FALSE, "", ""}, // this can be updated as new tracers are added
    {VKTRACE_TID_RESERVED, FALSE, "", ""}, // this can be updated as new tracers are added
};

typedef enum _VKTRACE_TRACE_PACKET_ID
{
    VKTRACE_TPI_MESSAGE,
    VKTRACE_TPI_MARKER_CHECKPOINT,
    VKTRACE_TPI_MARKER_API_BOUNDARY,
    VKTRACE_TPI_MARKER_API_GROUP_BEGIN,
    VKTRACE_TPI_MARKER_API_GROUP_END,
    VKTRACE_TPI_MARKER_TERMINATE_PROCESS,
    VKTRACE_TPI_BEGIN_API_HERE // this enum should always be the last in the list. Feel free to insert new ID above this one.
} VKTRACE_TRACE_PACKET_ID;

typedef struct {
    uint8_t id;
    uint8_t is_64_bit;
} vktrace_tracer_info;

typedef struct {
    uint16_t trace_file_version;
    uint32_t uuid[4];
    uint64_t first_packet_offset;   // will be size of header including size of tracer_id_array and state_snapshot_path/binary
    uint8_t tracer_count;           // number of tracers referenced in this trace file
    vktrace_tracer_info tracer_id_array[VKTRACE_MAX_TRACER_ID_ARRAY_SIZE]; // array of tracer_ids and values which are referenced in the trace file
    uint64_t trace_start_time;
} vktrace_trace_file_header;

typedef struct {
    uint64_t size; // total size, including extra data, needed to get to the next packet_header
    uint64_t global_packet_index;
    uint8_t tracer_id; // TODO: need to uniquely identify tracers in a way that is known by the replayer
    uint16_t packet_id; // VKTRACE_TRACE_PACKET_ID (or one of the api-specific IDs)
    uint32_t thread_id;
    uint64_t vktrace_begin_time; // start of measuring vktrace's overhead related to this packet
    uint64_t entrypoint_begin_time;
    uint64_t entrypoint_end_time;
    uint64_t vktrace_end_time; // end of measuring vktrace's overhead related to this packet
    uint64_t next_buffers_offset; // used for tracking the addition of buffers to the trace packet
    uintptr_t pBody; // points to the body of the packet
} vktrace_trace_packet_header;

typedef struct {
    vktrace_trace_packet_header* pHeader;
    VktraceLogLevel type;
    uint32_t length;
    char* message;
} vktrace_trace_packet_message;

typedef struct {
    vktrace_trace_packet_header* pHeader;
    unsigned int length;
    char* label;
} vktrace_trace_packet_marker_checkpoint;

typedef vktrace_trace_packet_marker_checkpoint vktrace_trace_packet_marker_api_boundary;
typedef vktrace_trace_packet_marker_checkpoint vktrace_trace_packet_marker_api_group_begin;
typedef vktrace_trace_packet_marker_checkpoint vktrace_trace_packet_marker_api_group_end;

typedef VKTRACE_TRACER_ID (VKTRACER_CDECL *funcptr_VKTRACE_GetTracerId)();
