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

#include "glv_common.h"
#include "glv_tracelog.h"

#define GLV_TRACE_FILE_VERSION_1 0x0001
#define GLV_TRACE_FILE_VERSION GLV_TRACE_FILE_VERSION_1
#define GLV_TRACE_FILE_VERSION_MINIMUM_COMPATIBLE GLV_TRACE_FILE_VERSION_1

#define GLV_MAX_TRACER_ID_ARRAY_SIZE 16

typedef enum GLV_TRACER_ID
{
    GLV_TID_RESERVED = 0,
    GLV_TID_GL_FPS,
    GLV_TID_MANTLE,
    GLV_TID_XGL,
    GLV_TID_MANTLE_PERF
    // Max enum must be less than GLV_MAX_TRACER_ID_ARRAY_SIZE
} GLV_TRACER_ID;

typedef struct GLV_TRACER_REPLAYER_INFO
{
    GLV_TRACER_ID tracerId;
    BOOL needsReplayer;
    const char* const replayerLibraryName;
} GLV_TRACER_REPLAYER_INFO;

// The index here should match the value of the GLV_TRACER_ID
static const GLV_TRACER_REPLAYER_INFO gs_tracerReplayerInfo[GLV_MAX_TRACER_ID_ARRAY_SIZE] = {
    {GLV_TID_RESERVED, FALSE, ""},
    {GLV_TID_GL_FPS, FALSE, ""},
    {GLV_TID_MANTLE, TRUE, GLV_LIBRARY_NAME(glvreplay_mantle)},
    {GLV_TID_XGL, TRUE, GLV_LIBRARY_NAME(glvreplay_xgl)},
    {GLV_TID_MANTLE_PERF, FALSE, ""},
    {GLV_TID_RESERVED, FALSE, ""}, // this can be updated as new tracers are added
    {GLV_TID_RESERVED, FALSE, ""}, // this can be updated as new tracers are added
    {GLV_TID_RESERVED, FALSE, ""}, // this can be updated as new tracers are added
    {GLV_TID_RESERVED, FALSE, ""}, // this can be updated as new tracers are added
    {GLV_TID_RESERVED, FALSE, ""}, // this can be updated as new tracers are added
    {GLV_TID_RESERVED, FALSE, ""}, // this can be updated as new tracers are added
    {GLV_TID_RESERVED, FALSE, ""}, // this can be updated as new tracers are added
    {GLV_TID_RESERVED, FALSE, ""}, // this can be updated as new tracers are added
    {GLV_TID_RESERVED, FALSE, ""}, // this can be updated as new tracers are added
    {GLV_TID_RESERVED, FALSE, ""}, // this can be updated as new tracers are added
    {GLV_TID_RESERVED, FALSE, ""}, // this can be updated as new tracers are added
};

typedef enum _GLV_TRACE_PACKET_ID
{
    GLV_TPI_MESSAGE,
    GLV_TPI_MARKER_CHECKPOINT,
    GLV_TPI_MARKER_API_BOUNDARY,
    GLV_TPI_MARKER_API_GROUP_BEGIN,
    GLV_TPI_MARKER_API_GROUP_END,
    GLV_TPI_MARKER_TERMINATE_PROCESS,
    GLV_TPI_BEGIN_API_HERE // this enum should always be the last in the list. Feel free to insert new ID above this one.
} GLV_TRACE_PACKET_ID;

typedef struct {
    uint16_t trace_file_version;
    uint32_t uuid[4];
    uint64_t first_packet_offset;   // will be size of header including size of tracer_id_array and state_snapshot_path/binary
    uint8_t tracer_count;           // number of tracers referenced in this trace file
    uint8_t tracer_id_array[GLV_MAX_TRACER_ID_ARRAY_SIZE]; // array of tracer_ids which are referenced in the trace file
    uint8_t contains_state_snapshot; // bool (0 = false, 1 = true)
    uint32_t state_snapshot_size;   // either length of state_snapshot_path or bytecount of state_snapshot_binary
    intptr_t state_snapshot_path;   // char* array text path to snapshot file
    intptr_t state_snapshot_binary; // byte array binary blob
    // unsigned int pointer_sizes; // indicates whether app is 32-bit or 64-bit
} glv_trace_file_header;

typedef struct {
    uint64_t size; // total size, including extra data, needed to get to the next packet_header
    uint64_t global_packet_index;
    uint8_t tracer_id; // TODO: need to uniquely identify tracers in a way that is known by the replayer
    uint16_t packet_id; // GLV_TRACE_PACKET_ID (or one of the api-specific IDs)
    uint64_t entrypoint_begin_time;
    uint64_t entrypoint_end_time;
    uint64_t next_buffers_offset; // used for tracking the addition of buffers to the trace packet
    uintptr_t pBody; // points to the body of the packet
} glv_trace_packet_header;

typedef struct {
    glv_trace_packet_header* pHeader;
    TraceLogLevel type;
    uint32_t length;
    char* message;
} glv_trace_packet_message;

typedef struct {
    glv_trace_packet_header* pHeader;
    unsigned int length;
    char* label;
} glv_trace_packet_marker_checkpoint;

typedef glv_trace_packet_marker_checkpoint glv_trace_packet_marker_api_boundary;
typedef glv_trace_packet_marker_checkpoint glv_trace_packet_marker_api_group_begin;
typedef glv_trace_packet_marker_checkpoint glv_trace_packet_marker_api_group_end;

typedef GLV_TRACER_ID (GLVTRACER_CDECL *funcptr_GLV_GetTracerId)();
