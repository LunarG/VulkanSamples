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
#include "glvtrace.h"

#include "glvtrace_process.h"

extern "C" {
#include "glv_common.h"
#include "glv_filelike.h"
#include "glv_interconnect.h"
#include "glv_trace_packet_identifiers.h"
#include "glv_trace_packet_utils.h"
}

glvtrace_settings g_settings;
glvtrace_settings g_default_settings;

glv_SettingInfo g_settings_info[] =
{
    // common command options
    { "p", "program", GLV_SETTING_STRING, &g_settings.program, &g_default_settings.program, TRUE, "The program to trace."},
    { "a", "arguments", GLV_SETTING_STRING, &g_settings.arguments, &g_default_settings.arguments, TRUE, "Cmd-line arguments to pass to trace program."},
    { "w", "working_dir", GLV_SETTING_STRING, &g_settings.working_dir, &g_default_settings.working_dir, TRUE, "The program's working directory."},
    { "o", "output_trace", GLV_SETTING_STRING, &g_settings.output_trace, &g_default_settings.output_trace, TRUE, "Path to the generated output trace file."},
    { "l0", "trace_library0", GLV_SETTING_STRING, &g_settings.trace_library[0], NULL, TRUE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l1", "trace_library1", GLV_SETTING_STRING, &g_settings.trace_library[1], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l2", "trace_library2", GLV_SETTING_STRING, &g_settings.trace_library[2], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l3", "trace_library3", GLV_SETTING_STRING, &g_settings.trace_library[3], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l4", "trace_library4", GLV_SETTING_STRING, &g_settings.trace_library[4], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l5", "trace_library5", GLV_SETTING_STRING, &g_settings.trace_library[5], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l6", "trace_library6", GLV_SETTING_STRING, &g_settings.trace_library[6], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l7", "trace_library7", GLV_SETTING_STRING, &g_settings.trace_library[7], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l8", "trace_library8", GLV_SETTING_STRING, &g_settings.trace_library[8], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l9", "trace_library9", GLV_SETTING_STRING, &g_settings.trace_library[9], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l10", "trace_library10", GLV_SETTING_STRING, &g_settings.trace_library[10], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l11", "trace_library11", GLV_SETTING_STRING, &g_settings.trace_library[11], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l12", "trace_library12", GLV_SETTING_STRING, &g_settings.trace_library[12], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l13", "trace_library13", GLV_SETTING_STRING, &g_settings.trace_library[13], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l14", "trace_library14", GLV_SETTING_STRING, &g_settings.trace_library[14], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l15", "trace_library15", GLV_SETTING_STRING, &g_settings.trace_library[15], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "tlf", "trace_log_file", GLV_SETTING_STRING, &g_settings.tracelogfile, &g_default_settings.tracelogfile, TRUE, "Log file to write into from trace application."},
    { "ptm", "print_trace_messages", GLV_SETTING_BOOL, &g_settings.print_trace_messages, &g_default_settings.print_trace_messages, TRUE, "Print trace messages to glvtrace console."},

    //{ "s", "pause", GLV_SETTING_BOOL, &g_settings.pause, &g_default_settings.pause, TRUE, "Wait for a key at startup (so a debugger can be attached)" },
    //{ "q", "quiet", GLV_SETTING_BOOL, &g_settings.quiet, &g_default_settings.quiet, TRUE, "Disable warning, verbose, and debug output" },
    //{ "v", "verbose", GLV_SETTING_BOOL, &g_settings.verbose, &g_default_settings.verbose, TRUE, "Enable verbose output" },
    //{ "d", "debug", GLV_SETTING_BOOL, &g_settings.debug, &g_default_settings.debug, TRUE, "Enable verbose debug information" },
};

// ------------------------------------------------------------------------------------------------
#if defined(WIN32)
void MessageLoop()
{
    MSG msg = { 0 };
    bool quit = false;
    while (!quit)
    {
        if (GetMessage(&msg, NULL, 0, 0) == FALSE)
        {
            quit = true;
        }
        else
        {
            quit = (msg.message == GLV_WM_COMPLETE);
        }
    }
}
#endif

// returns the number of tracers that need to be injected
int PrepareTracers(glvtrace_settings* pSettings, glv_process_capture_trace_thread_info** ppTracerInfo)
{
    // determine number of tracers to load and inject
    unsigned int num_tracers = 0;
    for (unsigned int i = 0; i < GLV_MAX_TRACER_ID_ARRAY_SIZE; i++)
    {
        if (g_settings.trace_library[i] != NULL)
        {
            ++num_tracers;
        }
    }

    assert(ppTracerInfo != NULL && *ppTracerInfo == NULL);
    *ppTracerInfo = GLV_NEW_ARRAY(glv_process_capture_trace_thread_info, num_tracers);
    memset(*ppTracerInfo, 0, sizeof(glv_process_capture_trace_thread_info) * num_tracers);

    // consolidate the list, but also reverse the order so that the entrypoints should be hooked in the expected order
    unsigned int tmpTracerIndex = num_tracers;
    for (unsigned int i = 0; i < GLV_MAX_TRACER_ID_ARRAY_SIZE; i++)
    {
        if (g_settings.trace_library[i] != NULL)
        {
            void* hLibrary = glv_platform_open_library(g_settings.trace_library[i]);
            if (hLibrary == NULL)
            {
                glv_LogError("Failed to load tracer: %s\n", g_settings.trace_library[i]);
            }
            else
            {
                funcptr_GLV_GetTracerId GLV_GetTracerId = NULL;
                GLV_GetTracerId = (funcptr_GLV_GetTracerId)glv_platform_get_library_entrypoint(hLibrary, "GLV_GetTracerId");

                if (GLV_GetTracerId != NULL)
                {
                    --tmpTracerIndex;
                    (*ppTracerInfo)[tmpTracerIndex].tracerPath = g_settings.trace_library[i];
                    (*ppTracerInfo)[tmpTracerIndex].tracerId = GLV_GetTracerId();
                }
                else
                {
                    glv_LogError("Missing entrypoint GLV_GetTracerId() from %s\n", g_settings.trace_library[i]);
                }
                glv_platform_close_library(hLibrary);
            }
        }
    }

    if (tmpTracerIndex > 0)
    {
        glv_LogError("One or more tracers could not be loaded. Please correct the issue and try again.\n");
    }

    return num_tracers;
}

bool InjectTracersIntoProcess(glv_process_info* pInfo)
{
    bool bRecordingThreadsCreated = true;
    for (unsigned int i = 0; i < pInfo->tracerCount; i++)
    {
        // inject tracers
        if (glv_platform_remote_load_library(pInfo->hProcess, pInfo->pCaptureThreads[i].tracerPath, &pInfo->pCaptureThreads[i].tracingThread, &pInfo->processLDPreload))
        {
            // prepare data for capture threads
            pInfo->pCaptureThreads[i].pProcessInfo = pInfo;
            pInfo->pCaptureThreads[i].recordingThread = GLV_NULL_THREAD;

            // create thread to record trace packets from the tracer
            pInfo->pCaptureThreads[i].recordingThread = glv_platform_create_thread(Process_RunRecordTraceThread, &(pInfo->pCaptureThreads[i]));
            if (pInfo->pCaptureThreads[i].recordingThread == GLV_NULL_THREAD)
            {
                glv_LogError("Failed to create trace recording thread.\n");
                bRecordingThreadsCreated = false;
            }

#if defined(WIN32)
            // wait for the hooking / tracing thread to complete now that its recording thread is listening
            if (WaitForSingleObject(pInfo->pCaptureThreads[i].tracingThread, INFINITE) != WAIT_OBJECT_0)
            {
                glv_LogError("Injected tracer's thread did not return successfully.\n");
                bRecordingThreadsCreated = false;
            }
#endif
        }
        else
        {
            // failed to inject a DLL
            bRecordingThreadsCreated = false;
            break;
        }
    }
    return bRecordingThreadsCreated;
}

// ------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    unsigned int num_settings = sizeof(g_settings_info) / sizeof(g_settings_info[0]);

    memset(&g_settings, 0, sizeof(glvtrace_settings));

    // setup defaults
    memset(&g_default_settings, 0, sizeof(glvtrace_settings));
    g_default_settings.output_trace = glv_allocate_and_copy("glvtrace_out.trace");
    g_default_settings.tracelogfile = glv_allocate_and_copy("glv_log.txt");
    g_default_settings.print_trace_messages = FALSE;

    if (glv_SettingInfo_init(g_settings_info, num_settings, "glvtrace_settings.txt", argc, argv, &g_settings.arguments) != 0)
    {
        // invalid cmd-line parameters
        glv_SettingInfo_delete(g_settings_info, num_settings);
        glv_free(g_default_settings.output_trace);
        glv_free(g_default_settings.tracelogfile);
        return -1;
    }
    else
    {
        BOOL validArgs = TRUE;
        if (g_settings.program == NULL || strlen(g_settings.program) == 0)
        {
            glv_LogInfo("No program (-p) parameter found: Running glvtrace as server.\n");
            g_settings.arguments = NULL;
        }
        else
        {
            glv_LogInfo("Running glvtrace as parent process will spawn child process: %s\n", g_settings.program);
            if (g_settings.working_dir == NULL || strlen(g_settings.working_dir) == 0)
            {
                CHAR buf[4096];
                glv_LogWarn("Missing parameter: No working directory specified, assuming executable's path.\n");
                glv_platform_full_path(g_settings.program, 4096, buf);
                g_settings.working_dir = glv_platform_extract_path(buf);
            }
        }

        if (g_settings.arguments != NULL && strlen(g_settings.arguments) > 0)
        {
            glv_LogInfo("Args to be passed to child process: '%s'\n", g_settings.arguments);
        }
    
        if (validArgs == FALSE)
        {
            glv_SettingInfo_print_all(g_settings_info, num_settings);
            return -1;
        }
    }

    if (g_settings.tracelogfile != NULL && strlen(g_settings.tracelogfile) > 0)
    {
        glv_tracelog_set_log_file(glv_FileLike_create_file(fopen(g_settings.tracelogfile, "w+")));
    }

    unsigned int serverIndex = 0;
    do {
        // Create and start the process or run in server mode

        BOOL procStarted = TRUE;
        glv_process_info procInfo;
        memset(&procInfo, 0, sizeof(glv_process_info));
        if (g_settings.program != NULL)
        {
            procInfo.exeName = glv_allocate_and_copy(g_settings.program);
            procInfo.processArgs = glv_allocate_and_copy(g_settings.arguments);
            procInfo.workingDirectory = glv_allocate_and_copy(g_settings.working_dir);
            procInfo.traceFilename = glv_allocate_and_copy(g_settings.output_trace);
        } else
        {
            char *pExtension = strrchr(g_settings.output_trace, '.');
            char *basename = glv_allocate_and_copy_n(g_settings.output_trace, (int) ((pExtension == NULL) ? strlen(g_settings.output_trace) : pExtension - g_settings.output_trace));
            char num[16];
#ifdef PLATFORM_LINUX
            snprintf(num, 16, "%u", serverIndex);
#elif defined(WIN32)
            _snprintf_s(num, 16, _TRUNCATE, "%u", serverIndex);
#endif
            procInfo.traceFilename = glv_copy_and_append(basename, num, pExtension);
        }
        procInfo.parentThreadId = glv_platform_get_thread_id();

        // setup tracers
        procInfo.tracerCount = PrepareTracers(&g_settings, &procInfo.pCaptureThreads);

#if defined(WIN32)
        if (g_settings.program != NULL)
            // call CreateProcess to launch the application
            procStarted = glv_process_spawn(&procInfo);
#endif

        if (procStarted == TRUE && procInfo.tracerCount > 0)
        {
            if (InjectTracersIntoProcess(&procInfo) == FALSE)
            {
                glv_LogError("Failed to setup tracer communication threads.\n");
            }
            else
            {
                // create trace file before injecting tracers
                procInfo.pTraceFile = glv_write_trace_file_header(&procInfo);
            }
        }

#if defined(PLATFORM_LINUX)
        // in linux we want to spawn the process AFTER setting up LD_PRELOAD (which happens in the loop above)
        if (g_settings.program != NULL)
            procStarted = glv_process_spawn(&procInfo);
#endif

        if (procStarted == FALSE)
        {
            glv_LogError("Failed to setup remote process.\n");
        }
        else
        {
            // create watchdog thread to monitor existence of remote process
            if (g_settings.program != NULL)
                procInfo.watchdogThread = glv_platform_create_thread(Process_RunWatchdogThread, &procInfo);

#if defined(PLATFORM_LINUX)
            // Sync wait for local threads and remote process to complete.
            for (unsigned int i = 0; i < procInfo.tracerCount; i++)
            {
                glv_platform_sync_wait_for_thread(&(procInfo.pCaptureThreads[i].recordingThread));
            }

            if (g_settings.program != NULL)
                glv_platform_sync_wait_for_thread(&procInfo.watchdogThread);
#else
            glv_platform_resume_thread(&procInfo.hThread);

            // Now into the main message loop, listen for hotkeys to send over.
            MessageLoop();
#endif
        }

        glv_process_info_delete(&procInfo);
        serverIndex++;
    } while (g_settings.program == NULL);

    glv_SettingInfo_delete(g_settings_info, num_settings);
    glv_free(g_default_settings.output_trace);
    glv_free(g_default_settings.tracelogfile);
    glv_tracelog_delete_log_file();

    return 0;
}

