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
#include "vktrace.h"

#include "vktrace_process.h"

extern "C" {
#include "vktrace_common.h"
#include "vktrace_filelike.h"
#include "vktrace_interconnect.h"
#include "vktrace_trace_packet_identifiers.h"
#include "vktrace_trace_packet_utils.h"
}

#include <sys/types.h>
#include <sys/stat.h>

vktrace_settings g_settings;
vktrace_settings g_default_settings;

vktrace_SettingInfo g_settings_info[] =
{
    // common command options
    { "p", "Program", VKTRACE_SETTING_STRING, &g_settings.program, &g_default_settings.program, TRUE, "The program to trace."},
    { "a", "Arguments", VKTRACE_SETTING_STRING, &g_settings.arguments, &g_default_settings.arguments, TRUE, "Cmd-line arguments to pass to trace program."},
    { "w", "WorkingDir", VKTRACE_SETTING_STRING, &g_settings.working_dir, &g_default_settings.working_dir, TRUE, "The program's working directory."},
    { "o", "OutputTrace", VKTRACE_SETTING_STRING, &g_settings.output_trace, &g_default_settings.output_trace, TRUE, "Path to the generated output trace file."},
    { "s", "ScreenShot", VKTRACE_SETTING_STRING, &g_settings.screenshotList, &g_default_settings.screenshotList, TRUE, "Comma separated list of frame numbers on which to take a screen snapshot."},
    { "l0", "TraceLibrary0", VKTRACE_SETTING_STRING, &g_settings.trace_library[0], &g_default_settings.trace_library[0], TRUE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l1", "TraceLibrary1", VKTRACE_SETTING_STRING, &g_settings.trace_library[1], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l2", "TraceLibrary2", VKTRACE_SETTING_STRING, &g_settings.trace_library[2], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l3", "TraceLibrary3", VKTRACE_SETTING_STRING, &g_settings.trace_library[3], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l4", "TraceLibrary4", VKTRACE_SETTING_STRING, &g_settings.trace_library[4], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l5", "TraceLibrary5", VKTRACE_SETTING_STRING, &g_settings.trace_library[5], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l6", "TraceLibrary6", VKTRACE_SETTING_STRING, &g_settings.trace_library[6], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l7", "TraceLibrary7", VKTRACE_SETTING_STRING, &g_settings.trace_library[7], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l8", "TraceLibrary8", VKTRACE_SETTING_STRING, &g_settings.trace_library[8], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l9", "TraceLibrary9", VKTRACE_SETTING_STRING, &g_settings.trace_library[9], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l10", "TraceLibrary10", VKTRACE_SETTING_STRING, &g_settings.trace_library[10], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l11", "TraceLibrary11", VKTRACE_SETTING_STRING, &g_settings.trace_library[11], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l12", "TraceLibrary12", VKTRACE_SETTING_STRING, &g_settings.trace_library[12], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l13", "TraceLibrary13", VKTRACE_SETTING_STRING, &g_settings.trace_library[13], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l14", "TraceLibrary14", VKTRACE_SETTING_STRING, &g_settings.trace_library[14], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "l15", "TraceLibrary15", VKTRACE_SETTING_STRING, &g_settings.trace_library[15], NULL, FALSE, "Path to the dynamic tracer library to be injected, may use [0-15]."},
    { "ptm", "PrintTraceMessages", VKTRACE_SETTING_BOOL, &g_settings.print_trace_messages, &g_default_settings.print_trace_messages, TRUE, "Print trace messages to vktrace console."},

    //{ "z", "pauze", VKTRACE_SETTING_BOOL, &g_settings.pause, &g_default_settings.pause, TRUE, "Wait for a key at startup (so a debugger can be attached)" },
    //{ "q", "quiet", VKTRACE_SETTING_BOOL, &g_settings.quiet, &g_default_settings.quiet, TRUE, "Disable warning, verbose, and debug output" },
    //{ "v", "verbose", VKTRACE_SETTING_BOOL, &g_settings.verbose, &g_default_settings.verbose, TRUE, "Enable verbose output" },
    //{ "d", "debug", VKTRACE_SETTING_BOOL, &g_settings.debug, &g_default_settings.debug, TRUE, "Enable verbose debug information" },
};

vktrace_SettingGroup g_settingGroup =
{
    "vktrace",
    sizeof(g_settings_info) / sizeof(g_settings_info[0]),
    &g_settings_info[0]
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
            quit = (msg.message == VKTRACE_WM_COMPLETE);
        }
    }
}
#endif

// returns the number of tracers that need to be injected
int PrepareTracers(vktrace_settings* pSettings, vktrace_process_capture_trace_thread_info** ppTracerInfo)
{
    // determine number of tracers to load and inject
    unsigned int num_tracers = 0;
    for (unsigned int i = 0; i < VKTRACE_MAX_TRACER_ID_ARRAY_SIZE; i++)
    {
        if (g_settings.trace_library[i] != NULL)
        {
            ++num_tracers;
        }
    }

    assert(ppTracerInfo != NULL && *ppTracerInfo == NULL);
    *ppTracerInfo = VKTRACE_NEW_ARRAY(vktrace_process_capture_trace_thread_info, num_tracers);
    memset(*ppTracerInfo, 0, sizeof(vktrace_process_capture_trace_thread_info) * num_tracers);

    // consolidate the list, but also reverse the order so that the entrypoints should be hooked in the expected order
    unsigned int tmpTracerIndex = num_tracers;
    for (unsigned int i = 0; i < VKTRACE_MAX_TRACER_ID_ARRAY_SIZE; i++)
    {
        if (g_settings.trace_library[i] != NULL)
        {
            void* hLibrary = vktrace_platform_open_library(g_settings.trace_library[i]);
            if (hLibrary == NULL)
            {
                vktrace_LogError("Failed to load tracer: %s", g_settings.trace_library[i]);
            }
            else
            {
                funcptr_VKTRACE_GetTracerId VKTRACE_GetTracerId = NULL;
                VKTRACE_GetTracerId = (funcptr_VKTRACE_GetTracerId)vktrace_platform_get_library_entrypoint(hLibrary, "VKTRACE_GetTracerId");

                if (VKTRACE_GetTracerId != NULL)
                {
                    --tmpTracerIndex;
                    (*ppTracerInfo)[tmpTracerIndex].tracerPath = g_settings.trace_library[i];
                    (*ppTracerInfo)[tmpTracerIndex].tracerId = VKTRACE_GetTracerId();
                }
                else
                {
                    vktrace_LogError("Missing entrypoint VKTRACE_GetTracerId() from %s", g_settings.trace_library[i]);
                }
                vktrace_platform_close_library(hLibrary);
            }
        }
    }

    if (tmpTracerIndex > 0)
    {
        vktrace_LogError("One or more tracers could not be loaded. Please correct the issue and try again.");
        VKTRACE_DELETE(*ppTracerInfo);
        *ppTracerInfo = NULL;
        return 0;
    }

    return num_tracers;
}

bool InjectTracersIntoProcess(vktrace_process_info* pInfo)
{
    bool bRecordingThreadsCreated = true;
    for (unsigned int i = 0; i < pInfo->tracerCount; i++)
    {
        // inject tracers
        if (vktrace_platform_remote_load_library(pInfo->hProcess, pInfo->pCaptureThreads[i].tracerPath, &pInfo->pCaptureThreads[i].tracingThread, NULL))
        {
            // prepare data for capture threads
            pInfo->pCaptureThreads[i].pProcessInfo = pInfo;
            pInfo->pCaptureThreads[i].recordingThread = VKTRACE_NULL_THREAD;

            // create thread to record trace packets from the tracer
            pInfo->pCaptureThreads[i].recordingThread = vktrace_platform_create_thread(Process_RunRecordTraceThread, &(pInfo->pCaptureThreads[i]));
            if (pInfo->pCaptureThreads[i].recordingThread == VKTRACE_NULL_THREAD)
            {
                vktrace_LogError("Failed to create trace recording thread.");
                bRecordingThreadsCreated = false;
            }

#if defined(WIN32)
            // wait for the hooking / tracing thread to complete now that its recording thread is listening
            if (WaitForSingleObject(pInfo->pCaptureThreads[i].tracingThread, INFINITE) != WAIT_OBJECT_0)
            {
                vktrace_LogError("Injected tracer's thread did not return successfully.");
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

void loggingCallback(VktraceLogLevel level, const char* pMessage)
{
    switch(level)
    {
    case VKTRACE_LOG_ALWAYS: printf("%s\n", pMessage); break;
    case VKTRACE_LOG_DEBUG: printf("Debug: %s\n", pMessage); break;
    case VKTRACE_LOG_ERROR: printf("Error: %s\n", pMessage); break;
    case VKTRACE_LOG_WARNING: printf("Warning: %s\n", pMessage); break;
    case VKTRACE_LOG_VERBOSE: printf("Verbose: %s\n", pMessage); break;
    default:
        printf("%s\n", pMessage); break;
    }

#if defined(WIN32)
#if _DEBUG
    OutputDebugString(pMessage);
#endif
#endif
}

// ------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    memset(&g_settings, 0, sizeof(vktrace_settings));

    vktrace_LogSetCallback(loggingCallback);
    vktrace_LogSetLevel(VKTRACE_LOG_LEVEL_MAXIMUM);

    // get vktrace binary directory
    char* execDir = vktrace_platform_get_current_executable_directory();

    // setup defaults
    memset(&g_default_settings, 0, sizeof(vktrace_settings));
    g_default_settings.output_trace = vktrace_copy_and_append(execDir, VKTRACE_PATH_SEPARATOR, "vktrace_out.vktrace");
    g_default_settings.print_trace_messages = FALSE;
    g_default_settings.screenshotList = NULL;
#if defined(WIN32)
    g_default_settings.trace_library[0] = vktrace_copy_and_append(execDir, VKTRACE_PATH_SEPARATOR, "vulkan_trace.dll");
#elif defined(PLATFORM_LINUX)
    g_default_settings.trace_library[0] = vktrace_copy_and_append(execDir, VKTRACE_PATH_SEPARATOR, "libvktrace_layer.so");
#endif

    // free binary directory string
    vktrace_free(execDir);

    if (vktrace_SettingGroup_init(&g_settingGroup, NULL, argc, argv, &g_settings.arguments) != 0)
    {
        // invalid cmd-line parameters
        vktrace_SettingGroup_delete(&g_settingGroup);
        vktrace_free(g_default_settings.output_trace);
        vktrace_free(g_default_settings.trace_library[0]);
        return -1;
    }
    else
    {
        // Validate vktrace inputs
        BOOL validArgs = TRUE;
        if (g_settings.trace_library[0] == NULL)
        {
            vktrace_LogError("Missing required -l0 parameter to specify the tracer library.");
            validArgs = FALSE;
        }
        if (!strcmp(g_settings.trace_library[0], g_default_settings.trace_library[0]))
        {
            // look for default trace library in CWD then system directories
            void *handle;
            if ((handle = vktrace_platform_open_library(g_settings.trace_library[0])) == NULL)
            {
                char *filename = strrchr(g_settings.trace_library[0], VKTRACE_PATH_SEPARATOR[0]);
                if (!filename || (handle = vktrace_platform_open_library(filename+1)) == NULL)
                {
                    vktrace_LogError("No -l0 arg and default tracer library file can't be found.");
                    validArgs = FALSE;
                }
                else
                {
                    g_settings.trace_library[0] = filename+1;
                    vktrace_platform_close_library(handle);
                }
            }
            else
            {
                vktrace_platform_close_library(handle);
            }
        }
        if (g_settings.output_trace == NULL || strlen (g_settings.output_trace) == 0)
        {
            vktrace_LogError("No output trace file (-o) parameter found: Please specify a valid trace file to generate.");
            validArgs = FALSE;
        }
        else
        {
            size_t len = strlen(g_settings.output_trace);
            if (strncmp(&g_settings.output_trace[len-8], ".vktrace", 8) != 0)
            {
                // output trace filename does not end in .vktrace
                vktrace_LogError("Output trace file specified with -o parameter must have a '.vktrace' extension.");
                validArgs = FALSE;
            }
        }

        if (validArgs == FALSE)
        {
            vktrace_SettingGroup_print(&g_settingGroup);
            return -1;
        }

        if (g_settings.program == NULL || strlen(g_settings.program) == 0)
        {
            vktrace_LogAlways("No program (-p) parameter found: Running vktrace as server.");
            g_settings.arguments = NULL;
        }
        else
        {
            if (g_settings.working_dir == NULL || strlen(g_settings.working_dir) == 0)
            {
                CHAR* buf = VKTRACE_NEW_ARRAY(CHAR, 4096);
                vktrace_LogWarning("No working directory (-w) parameter found: Assuming executable's path as working directory.");
                vktrace_platform_full_path(g_settings.program, 4096, buf);
                g_settings.working_dir = vktrace_platform_extract_path(buf);
                VKTRACE_DELETE(buf);
            }

            vktrace_LogAlways("Running vktrace as parent process will spawn child process: %s", g_settings.program);
            if (g_settings.arguments != NULL && strlen(g_settings.arguments) > 0)
            {
                vktrace_LogAlways("Args to be passed to child process: '%s'", g_settings.arguments);
            }
        }
    }

    if (g_settings.screenshotList)
    {

        // Export list to screenshot layer
        vktrace_set_global_var("_VK_SCREENSHOT", g_settings.screenshotList);

    }
    else
    {
        vktrace_set_global_var("_VK_SCREENSHOT","");
    }


    unsigned int serverIndex = 0;
    do {
        // Create and start the process or run in server mode

        BOOL procStarted = TRUE;
        vktrace_process_info procInfo;
        memset(&procInfo, 0, sizeof(vktrace_process_info));
        if (g_settings.program != NULL)
        {
            procInfo.exeName = vktrace_allocate_and_copy(g_settings.program);
            procInfo.processArgs = vktrace_allocate_and_copy(g_settings.arguments);
            procInfo.fullProcessCmdLine = vktrace_copy_and_append(g_settings.program, " ", g_settings.arguments);
            procInfo.workingDirectory = vktrace_allocate_and_copy(g_settings.working_dir);
            procInfo.traceFilename = vktrace_allocate_and_copy(g_settings.output_trace);
        } else
        {
            char *pExtension = strrchr(g_settings.output_trace, '.');
            char *basename = vktrace_allocate_and_copy_n(g_settings.output_trace, (int) ((pExtension == NULL) ? strlen(g_settings.output_trace) : pExtension - g_settings.output_trace));
            char num[16];
#ifdef PLATFORM_LINUX
            snprintf(num, 16, "%u", serverIndex);
#elif defined(WIN32)
            _snprintf_s(num, 16, _TRUNCATE, "%u", serverIndex);
#endif
            procInfo.traceFilename = vktrace_copy_and_append(basename, num, pExtension);
         }

        procInfo.parentThreadId = vktrace_platform_get_thread_id();

        // setup tracers
        procInfo.tracerCount = PrepareTracers(&g_settings, &procInfo.pCaptureThreads);

        if (procInfo.tracerCount == 0)
        {
            vktrace_SettingGroup_print(&g_settingGroup);
            vktrace_process_info_delete(&procInfo);
            return -1;
        }

        if (g_settings.program != NULL)
            // call CreateProcess to launch the application
            procStarted = vktrace_process_spawn(&procInfo);

        if (procStarted == FALSE)
        {
            vktrace_LogError("Failed to setup remote process.");
        }
        else
        {
            if (InjectTracersIntoProcess(&procInfo) == FALSE)
            {
                vktrace_LogError("Failed to setup tracer communication threads.");
                return -1;
            }

            // create watchdog thread to monitor existence of remote process
            if (g_settings.program != NULL)
                procInfo.watchdogThread = vktrace_platform_create_thread(Process_RunWatchdogThread, &procInfo);

#if defined(PLATFORM_LINUX)
            // Sync wait for local threads and remote process to complete.
            for (unsigned int i = 0; i < procInfo.tracerCount; i++)
            {
                vktrace_platform_sync_wait_for_thread(&(procInfo.pCaptureThreads[i].recordingThread));
            }

            if (g_settings.program != NULL)
                vktrace_platform_sync_wait_for_thread(&procInfo.watchdogThread);
#else
            vktrace_platform_resume_thread(&procInfo.hThread);

            // Now into the main message loop, listen for hotkeys to send over.
            MessageLoop();
#endif
        }

        vktrace_process_info_delete(&procInfo);
        serverIndex++;
    } while (g_settings.program == NULL);

    vktrace_SettingGroup_delete(&g_settingGroup);
    vktrace_free(g_default_settings.output_trace);

    return 0;
}

