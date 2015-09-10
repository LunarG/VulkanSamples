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
#include "vktrace_vk_vk_debug_report_lunarg.h"
#include "vktrace_vk_vk_debug_report_lunarg_packets.h"
#include "vktrace_vk_packet_id.h"
#include "vk_struct_size_helper.h"
#include "vk_debug_report_lunarg_struct_size_helper.h"
#include "vktrace_lib_helpers.h"

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkDbgCreateMsgCallback(
    VkInstance instance,
    VkFlags msgFlags,
    const PFN_vkDbgMsgCallback pfnMsgCallback,
    void* pUserData,
    VkDbgMsgCallback* pMsgCallback)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkDbgCreateMsgCallback* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDbgCreateMsgCallback, sizeof(pUserData) + sizeof(VkDbgMsgCallback));
    result = g_instTable.DbgCreateMsgCallback(instance, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDbgCreateMsgCallback(pHeader);
    pPacket->instance = instance;
    pPacket->msgFlags = msgFlags;
    PFN_vkDbgMsgCallback* pNonConstCallback = (PFN_vkDbgMsgCallback*)&pPacket->pfnMsgCallback;
    *pNonConstCallback = pfnMsgCallback;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pUserData), sizeof(void), pUserData);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMsgCallback), sizeof(VkDbgMsgCallback), pMsgCallback);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pUserData));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pMsgCallback));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkDbgDestroyMsgCallback(
    VkInstance instance,
    VkDbgMsgCallback msgCallback)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkDbgDestroyMsgCallback* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDbgDestroyMsgCallback, 0);
    result = g_instTable.DbgDestroyMsgCallback(instance, msgCallback);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDbgDestroyMsgCallback(pHeader);
    pPacket->instance = instance;
    pPacket->msgCallback = msgCallback;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

