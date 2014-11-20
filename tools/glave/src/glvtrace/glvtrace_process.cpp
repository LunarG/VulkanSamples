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

#include <string>
#include "glvtrace_process.h"
#include "glvtrace.h"

#if defined(PLATFORM_LINUX)
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

extern "C" {
#include "glv_filelike.h"
#include "glv_interconnect.h"
#include "glv_trace_packet_utils.h"
}

const unsigned long kWatchDogPollTime = 250;

#if defined(WIN32)
void SafeCloseHandle(HANDLE& _handle)
{
    if (_handle) {
        CloseHandle(_handle);
        _handle = NULL;
    }
}
#endif

// ------------------------------------------------------------------------------------------------
GLV_THREAD_ROUTINE_RETURN_TYPE Process_RunWatchdogThread(LPVOID _procInfoPtr)
{
    glv_process_info* pProcInfo = (glv_process_info*)_procInfoPtr;

#if defined(WIN32)

    while (WaitForSingleObject(pProcInfo->hProcess, kWatchDogPollTime) == WAIT_TIMEOUT)
    {
        if (pProcInfo->serverRequestsTermination)
        {
            glv_LogInfo("GLVTrace has requested exit.\n");
            return 0;
        }
    }

    glv_LogInfo("Child Process has terminated.\n");

    PostThreadMessage(pProcInfo->parentThreadId, GLV_WM_COMPLETE, 0, 0);
    pProcInfo->serverRequestsTermination = TRUE;
    
#elif defined(PLATFORM_LINUX)
    int status = 0;
    int options = 0;
    while (waitpid(pProcInfo->processId, &status, options) != -1)
    {
        if (WIFEXITED(status))
        {
            glv_LogInfo("Child process exited\n");
            break;
        }
        else if (WCOREDUMP(status))
        {
            glv_LogInfo("Child process crashed\n");
            break;
        }
        else if (WIFSIGNALED(status))
            glv_LogInfo("Child process was signaled\n");
        else if (WIFSTOPPED(status))
            glv_LogInfo("Child process was stopped\n");
        else if (WIFCONTINUED(status))
            glv_LogInfo("Child process was continued\n");
    }
#endif
    return 0;
}

// ------------------------------------------------------------------------------------------------
GLV_THREAD_ROUTINE_RETURN_TYPE Process_RunRecordTraceThread(LPVOID _threadInfo)
{
    glv_process_capture_trace_thread_info* pInfo = (glv_process_capture_trace_thread_info*)_threadInfo;

    MessageStream* pMessageStream = glv_MessageStream_create(TRUE, "", GLV_BASE_PORT + pInfo->tracerId);
    if (pMessageStream == NULL)
    {
        glv_LogError("Thread_CaptureTrace() cannot create message stream\n");
        return 1;
    }

    FileLike* fileLikeSocket = glv_FileLike_create_msg(pMessageStream);
    unsigned int total_packet_count = 0;
    glv_trace_packet_header* pHeader = NULL;
    size_t bytes_written;

    while (pInfo->pProcessInfo->serverRequestsTermination == FALSE)
    {
        // get a packet
        //glv_LogDebug("Waiting for a packet...");

        // read entire packet in
        pHeader = glv_read_trace_packet(fileLikeSocket);
        ++total_packet_count;
        if (pHeader == NULL)
        {
            if (pMessageStream->mErrorNum == WSAECONNRESET)
            {
                glv_LogError("Network Connection Reset\n");
            }
            else
            {
                glv_LogError("Network Connection Failed\n");
            }
            break;
        }

        //glv_LogDebug("Received packet id: %hu\n", pHeader->packet_id);
        
        if (pHeader->pBody == (uintptr_t) NULL)
        {
            glv_LogWarn("Received empty packet body for id: %hu\n", pHeader->packet_id);
        }
        else
        {
            // handle special case packets
            if (pHeader->packet_id == GLV_TPI_MESSAGE)
            {
                if (g_settings.print_trace_messages == TRUE)
                {
                    glv_trace_packet_message* pPacket = glv_interpret_body_as_trace_packet_message(pHeader);
                    glv_PrintTraceMessage(pPacket->type, pPacket->message);
                    printf("packet index: %lu\n", pHeader->global_packet_index);
                    glv_finalize_buffer_address(pHeader, (void **) &(pPacket->message));
                }
            }

            if (pHeader->packet_id == GLV_TPI_MARKER_TERMINATE_PROCESS)
            {
                pInfo->pProcessInfo->serverRequestsTermination = true;
                glv_delete_trace_packet(&pHeader);
                printf("Thread_CaptureTrace is exiting\n");
                break;
            }

            if (pInfo->pProcessInfo->pTraceFile != NULL)
            {
                glv_enter_critical_section(&pInfo->pProcessInfo->traceFileCriticalSection);
                bytes_written = fwrite(pHeader, 1, (size_t)pHeader->size, pInfo->pProcessInfo->pTraceFile);
                fflush(pInfo->pProcessInfo->pTraceFile);
                glv_leave_critical_section(&pInfo->pProcessInfo->traceFileCriticalSection);
                if (bytes_written != pHeader->size)
                {
                    glv_LogError("Failed to write the packet for packet_id = %hu\n", pHeader->packet_id);
                }
            }
        }

        // clean up
        glv_delete_trace_packet(&pHeader);
    }

    GLV_DELETE(fileLikeSocket);
    glv_MessageStream_destroy(&pMessageStream);

    return 0;
}
