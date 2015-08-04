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
#include "glv_process.h"


BOOL glv_process_spawn(glv_process_info* pInfo)
{
    assert(pInfo != NULL);

#if defined(WIN32)
    {
    unsigned long processCreateFlags = CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED;
    char fullExePath[_MAX_PATH];
    PROCESS_INFORMATION processInformation;
    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);

    memset(&processInformation, 0, sizeof(PROCESS_INFORMATION));
    memset(fullExePath, 0, sizeof(char)*_MAX_PATH);
    fullExePath[0] = 0;

    SetLastError(0);
    SearchPath(NULL, pInfo->exeName, ".exe", ARRAYSIZE(fullExePath), fullExePath, NULL);

    if (!CreateProcess(fullExePath, pInfo->fullProcessCmdLine, NULL, NULL, TRUE,
        processCreateFlags, NULL, pInfo->workingDirectory,
        &si, &processInformation))
    {
        glv_LogError("Failed to inject ourselves into target process--couldn't spawn '%s'.", fullExePath);
        return FALSE;
    }

    pInfo->hProcess = processInformation.hProcess;
    pInfo->hThread = processInformation.hThread;
    pInfo->processId = processInformation.dwProcessId;
    // TODO : Do we need to do anything with processInformation.dwThreadId?
    }
#elif defined(PLATFORM_LINUX)
    pInfo->processId = fork();
    if (pInfo->processId == -1)
    {
        glv_LogError("Failed to spawn process.");
        return FALSE;
    }
    else if (pInfo->processId == 0)
    {
        char *envp[5];
        envp[0] = pInfo->processLDPreload;
        envp[4] = (char *) (NULL);
        static char default_disp[] = "DISPLAY=:0";
        char * libvk_drivers_path = (char *) (NULL);
        char * env_display = (char *) NULL;
        char * ld_library_path = (char *) NULL;
        char * args[128];
        const char delim[]= " \t";
        unsigned int idx;
        // inside new process
        // change process name so the the tracer DLLs will behave as expected when loaded.
        // NOTE: Must be 15 characters or less.
        const char * tmpProcName = "glvChildProcess";
        prctl(PR_SET_NAME, (unsigned long)tmpProcName, 0, 0, 0);

        // change working directory
        if (chdir(pInfo->workingDirectory) == -1)
        {
            glv_LogError("Failed to set working directory.");
        }

        env_display = getenv("DISPLAY");
        if (env_display != NULL)
        {
            char *disp = malloc(strlen(env_display) + strlen("DISPLAY=") + 1);
            if (disp == NULL)
                glv_LogError("Failed to malloc for env_display in glv_process_spawn().");
            snprintf(disp, strlen(env_display) + strlen("DISPLAY=") + 1, "DISPLAY=%s", env_display);

            envp[1] = disp;
        } else
        {
            envp[1] = default_disp;
        }

        // TODO this needs to be generalized for other drivers
        libvk_drivers_path = getenv("VK_ICD_FILENAMES");
        if (libvk_drivers_path == NULL)
        {
            glv_LogWarning("VK_ICD_FILENAMES env var was not set. We recommend that you set it.");
            envp[2] = NULL;
        }
        else
        {
            char *dPath = malloc(strlen(libvk_drivers_path) + strlen("VK_ICD_FILENAMES=") + 1);
            if (dPath == NULL)
                glv_LogError("Failed to malloc in glv_process_spawn().");
            snprintf(dPath, strlen(libvk_drivers_path) + strlen("VK_ICD_FILENAMES=") + 1, "VK_ICD_FILENAMES=%s", libvk_drivers_path);
            envp[2] = dPath;
        }

        ld_library_path = getenv("LD_LIBRARY_PATH");
        if (ld_library_path != NULL)
        {
            char *ld_lib = malloc(strlen(ld_library_path) + strlen("LD_LIBRARY_PATH=") + 1);
            if (ld_lib == NULL)
                glv_LogError("Failed to malloc for LD_LIBRARY_PATH in glv_process_spawn().");
            snprintf(ld_lib, strlen(ld_library_path) + strlen("LD_LIBRARY_PATH=") + 1, "LD_LIBRARY_PATH=%s", ld_library_path);

            envp[3] = ld_lib;
        } else
        {
            envp[3] = NULL;
        }

        args[0] = pInfo->exeName;
        args[127] = NULL;
        idx = 1;
        args[idx] = strtok(pInfo->processArgs, delim);
        while ( args[idx] != NULL && idx < 128)
        {
            idx++;
            args[idx] = strtok(NULL, delim);
        }
        glv_LogDebug("exec process=%s argc=%u \n     env0:%s\n     env2:%s\n     env3:%s",
                pInfo->exeName, idx, (envp[0] == NULL) ? "" : envp[0],
                (envp[2] == NULL) ? "" : envp[2],
                (envp[3] == NULL) ? "" : envp[3]);
        extern char **environ;
        environ = envp;

        if (execv(pInfo->exeName, args) < 0)
        {
            glv_LogError("Failed to spawn process.");
            return FALSE;
        }
    }
#endif

    return TRUE;
}

void glv_process_info_delete(glv_process_info* pInfo)
{
    unsigned int i = 0;
    if (pInfo->pCaptureThreads != NULL)
    {
        for (i = 0; i < pInfo->tracerCount; i++)
        {
            glv_platform_delete_thread(&(pInfo->pCaptureThreads[i].recordingThread));
        }
        GLV_DELETE(pInfo->pCaptureThreads);
    }

#ifdef WIN32
    glv_platform_delete_thread(&(pInfo->watchdogThread));
#endif

    GLV_DELETE(pInfo->traceFilename);
    GLV_DELETE(pInfo->workingDirectory);
    GLV_DELETE(pInfo->processArgs);
    GLV_DELETE(pInfo->fullProcessCmdLine);
    GLV_DELETE(pInfo->exeName);

    if (pInfo->pTraceFile != NULL)
    {
        fclose(pInfo->pTraceFile);
    }
    glv_delete_critical_section(&(pInfo->traceFileCriticalSection));
}
