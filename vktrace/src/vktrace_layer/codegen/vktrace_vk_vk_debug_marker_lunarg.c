/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * Vulkan
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

#include "vktrace_platform.h"
#include "vktrace_common.h"
#include "vktrace_vk_vk_debug_marker_lunarg.h"
#include "vktrace_vk_vk_debug_marker_lunarg_packets.h"
#include "vktrace_vk_packet_id.h"
#include "vk_struct_size_helper.h"
#include "vk_debug_marker_lunarg_struct_size_helper.h"
#include "vk_debug_marker_layer.h"

//TODO change this to support multiple devices
VkLayerDebugMarkerDispatchTable g_debugMarkerTable;

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdDbgMarkerBegin(
    VkCmdBuffer cmdBuffer,
    const char* pMarker)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdDbgMarkerBegin* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdDbgMarkerBegin, ((pMarker != NULL) ? strlen(pMarker) + 1 : 0));
    g_debugMarkerTable.CmdDbgMarkerBegin(cmdBuffer, pMarker);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdDbgMarkerBegin(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMarker), ((pMarker != NULL) ? strlen(pMarker) + 1 : 0), pMarker);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pMarker));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdDbgMarkerEnd(
    VkCmdBuffer cmdBuffer)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdDbgMarkerEnd* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdDbgMarkerEnd, 0);
    g_debugMarkerTable.CmdDbgMarkerEnd(cmdBuffer);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdDbgMarkerEnd(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkDbgSetObjectTag(
    VkDevice device,
    VkDbgObjectType objType,
    uint64_t object,
    size_t tagSize,
    const void* pTag)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkDbgSetObjectTag* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDbgSetObjectTag, sizeof(pTag));
    result = g_debugMarkerTable.DbgSetObjectTag(device, objType, object, tagSize, pTag);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDbgSetObjectTag(pHeader);
    pPacket->device = device;
    pPacket->objType = objType;
    pPacket->object = object;
    pPacket->tagSize = tagSize;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pTag), sizeof(void), pTag);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pTag));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkDbgSetObjectName(
    VkDevice device,
    VkDbgObjectType objType,
    uint64_t object,
    size_t nameSize,
    const char* pName)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkDbgSetObjectName* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDbgSetObjectName, ((pName != NULL) ? strlen(pName) + 1 : 0));
    result = g_debugMarkerTable.DbgSetObjectName(device, objType, object, nameSize, pName);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDbgSetObjectName(pHeader);
    pPacket->device = device;
    pPacket->objType = objType;
    pPacket->object = object;
    pPacket->nameSize = nameSize;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pName), ((pName != NULL) ? strlen(pName) + 1 : 0), pName);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pName));
    FINISH_TRACE_PACKET();
    return result;
}

