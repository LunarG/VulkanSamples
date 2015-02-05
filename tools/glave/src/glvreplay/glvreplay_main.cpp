/**************************************************************************
 *
 * Copyright 2014 Lunarg, Inc.
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

#include <stdio.h>
#include <string>
extern "C" {
#include "glv_common.h"
#include "glv_tracelog.h"
#include "glv_filelike.h"
#include "glv_trace_packet_utils.h"
}
#include "glvreplay_main.h"
#include "glvreplay_factory.h"
#include "glvreplay_seq.h"
#include "glvreplay_window.h"
#include "getopt/getopt.h"

glvreplay_settings g_defaultReplaySettings = { NULL, FALSE, 1, NULL };
GLVTRACER_EXPORT glvreplay_settings *g_pReplaySettings = &g_defaultReplaySettings;

glv_SettingInfo g_settings_info[] =
{
    { "t", "trace_file", GLV_SETTING_STRING, &g_pReplaySettings->pTraceFilePath, &g_defaultReplaySettings.pTraceFilePath, TRUE, "The trace file to replay."},
    { "l", "numLoops", GLV_SETTING_UINT, &g_pReplaySettings->numLoops, &g_defaultReplaySettings.numLoops, TRUE, "The number of times to replay the trace file."},
    { "b", "benchmark", GLV_SETTING_BOOL, &g_pReplaySettings->benchmark, &g_defaultReplaySettings.benchmark, TRUE, "(unsupported) Disables some debug features so that replaying happens as fast as possible."},
    { "s", "screenshotList", GLV_SETTING_STRING, &g_pReplaySettings->screenshotList, &g_defaultReplaySettings.screenshotList, TRUE, "Comma seperated list of frame numbers to take snapshots of"},
};

glv_SettingGroup g_replaySettingGroup =
{
    "glvreplay",
    sizeof(g_settings_info) / sizeof(g_settings_info[0]),
    &g_settings_info[0]
};

namespace glv_replay {
int main_loop(Sequencer &seq, glv_trace_packet_replay_library *replayerArray[], unsigned int numLoops)
{
    int err = 0;
    glv_trace_packet_header *packet;
    unsigned int res;
    glv_trace_packet_replay_library *replayer;
    glv_trace_packet_message* msgPacket;
    struct seqBookmark startingPacket;

    // record the location of starting trace packet
    seq.record_bookmark();
    seq.get_bookmark(startingPacket);

    while (numLoops > 0)
    {
        while ((packet = seq.get_next_packet()) != NULL)
        {
            switch (packet->packet_id) {
                case GLV_TPI_MESSAGE:
                    msgPacket = glv_interpret_body_as_trace_packet_message(packet);
                    glv_PrintTraceMessage(msgPacket->type, msgPacket->message);
                    break;
                case GLV_TPI_MARKER_CHECKPOINT:
                    break;
                case GLV_TPI_MARKER_API_BOUNDARY:
                    break;
                case GLV_TPI_MARKER_API_GROUP_BEGIN:
                    break;
                case GLV_TPI_MARKER_API_GROUP_END:
                    break;
                case GLV_TPI_MARKER_TERMINATE_PROCESS:
                    break;
                //TODO processing code for all the above cases
                default:
                {
                    if (packet->tracer_id >= GLV_MAX_TRACER_ID_ARRAY_SIZE  || packet->tracer_id == GLV_TID_RESERVED) {
                        glv_LogError("Tracer_id from packet num packet %d invalid.\n", packet->packet_id);
                        continue;
                    }
                    replayer = replayerArray[packet->tracer_id];
                    if (replayer == NULL) {
                        glv_LogWarn("Tracer_id %d has no valid replayer.\n", packet->tracer_id);
                        continue;
                    }
                    if (packet->packet_id >= GLV_TPI_BEGIN_API_HERE)
                    {
                        // replay the API packet
                        res = replayer->Replay(replayer->Interpret(packet));
                        if (res != GLV_REPLAY_SUCCESS)
                        {
                           glv_LogError("Failed to replay packet_id %d.\n",packet->packet_id);
                        }
                    } else {
                        glv_LogError("Bad packet type id=%d, index=%d.\n", packet->packet_id, packet->global_packet_index);
                        return -1;
                    }
                }
            }
        }
        numLoops--;
        seq.set_bookmark(startingPacket);
    }
    return err;
}
} // namespace glv_replay

using namespace glv_replay;

extern "C"
int main(int argc, char **argv)
{
    int err = 0;
    FILE* pSettingsFile = NULL;
    glv_SettingGroup* pAllSettings = NULL;
    unsigned int numAllSettings = 0;

    // Override options based on settings file
    pSettingsFile = fopen("glvreplay_settings.txt", "r");
    if (pSettingsFile == NULL)
    {
        glv_LogWarn("Failed to open glvreplay_settings.txt\n");
    }

    if (pSettingsFile != NULL)
    {
        if (glv_SettingGroup_Load_from_file(pSettingsFile, &pAllSettings, &numAllSettings) == -1)
        {
            glv_LogWarn("Failed to parse glvreplay_settings.txt\n");
        }

        // apply overrides from settings file
        if (pAllSettings != NULL && numAllSettings > 0)
        {
            glv_SettingGroup_Apply_Overrides(&g_replaySettingGroup, pAllSettings, numAllSettings);
        }
    }

    // apply settings from cmd-line args
    if (glv_SettingGroup_init_from_cmdline(&g_replaySettingGroup, argc, argv, &g_pReplaySettings->pTraceFilePath) != 0)
    {
        // invalid options specified
        if (pAllSettings != NULL)
        {
            glv_SettingGroup_Delete_Loaded(&pAllSettings, &numAllSettings);
        }
        return err;
    }

    // merge settings so that new settings will get written into the settings file
    glv_SettingGroup_merge(&g_replaySettingGroup, &pAllSettings, &numAllSettings);

    if (pSettingsFile != NULL)
    {
        fclose(pSettingsFile);
        // Do not set pSettingsFile = NULL !!
        // The non-NULL value is used farther down to determine 
        // if a settings file previously existed (and if not, it is 
        // written out after all the replayers' settings are merged in.
    }

    // open trace file and read in header
    char* pTraceFile = g_pReplaySettings->pTraceFilePath;
    glv_trace_file_header fileHeader;
    FILE *tracefp;

    if (pTraceFile != NULL && strlen(pTraceFile) > 0)
    {
        tracefp = fopen(pTraceFile, "rb");
        if (tracefp == NULL)
        {
            glv_LogError("Cannot open trace file: '%s'\n", pTraceFile);
            return 1;
        }
    }
    else
    {
        glv_LogError("No trace file specified.\n");
        glv_SettingGroup_print(&g_replaySettingGroup);
        if (pAllSettings != NULL)
        {
            glv_SettingGroup_Delete_Loaded(&pAllSettings, &numAllSettings);
        }
        return 1;
    }

    FileLike* traceFile = glv_FileLike_create_file(tracefp);
    if (glv_FileLike_ReadRaw(traceFile, &fileHeader, sizeof(fileHeader)) == false)
    {
        glv_LogError("Unable to read header from file.\n");
        if (pAllSettings != NULL)
        {
            glv_SettingGroup_Delete_Loaded(&pAllSettings, &numAllSettings);
        }
        GLV_DELETE(traceFile);
        return 1;
    }

    // Make sure trace file version is supported
    if (fileHeader.trace_file_version < GLV_TRACE_FILE_VERSION_MINIMUM_COMPATIBLE)
    {
        glv_LogError("Trace file version %u is older than minimum compatible version (%u).\nYou'll need to make a new trace file, or use an older replayer.\n", fileHeader.trace_file_version, GLV_TRACE_FILE_VERSION_MINIMUM_COMPATIBLE);
    }

    // load any API specific driver libraries and init replayer objects
    uint8_t tidApi = GLV_TID_RESERVED;
    glv_trace_packet_replay_library* replayer[GLV_MAX_TRACER_ID_ARRAY_SIZE];
    ReplayFactory makeReplayer;
    Display disp(1024, 768, 0, false);

    for (int i = 0; i < GLV_MAX_TRACER_ID_ARRAY_SIZE; i++)
    {
        replayer[i] = NULL;
    }

    for (int i = 0; i < fileHeader.tracer_count; i++)
    {
        uint8_t tracerId = fileHeader.tracer_id_array[i].id;
        tidApi = tracerId;

        const GLV_TRACER_REPLAYER_INFO* pReplayerInfo = &(gs_tracerReplayerInfo[tracerId]);

        if (pReplayerInfo->tracerId != tracerId)
        {
            glv_LogError("Replayer info for TracerId (%d) failed consistency check.\n", tracerId);
            assert(!"TracerId in GLV_TRACER_REPLAYER_INFO does not match the requested tracerId. The array needs to be corrected.");
        }
        else if (pReplayerInfo->needsReplayer == TRUE)
        {
            // Have our factory create the necessary replayer
            replayer[tracerId] = makeReplayer.Create(tracerId);

            if (replayer[tracerId] == NULL)
            {
                // replayer failed to be created
                if (pAllSettings != NULL)
                {
                    glv_SettingGroup_Delete_Loaded(&pAllSettings, &numAllSettings);
                }
                return err;
            }

            // merge the replayer's settings into the list of all settings so that we can output a comprehensive settings file later on.
            glv_SettingGroup_merge(replayer[tracerId]->GetSettings(), &pAllSettings, &numAllSettings);

            // update the replayer with the loaded settings
            replayer[tracerId]->UpdateFromSettings(pAllSettings, numAllSettings);

            // Initialize the replayer
            err = replayer[tracerId]->Initialize(&disp);
            if (err) {
                glv_LogError("Couldn't Initialize replayer for TracerId %d.\n", tracerId);
                if (pAllSettings != NULL)
                {
                    glv_SettingGroup_Delete_Loaded(&pAllSettings, &numAllSettings);
                }
                return err;
            }
        }
    }

    if (tidApi == GLV_TID_RESERVED) {
        glv_LogError("No API specified in tracefile for replaying.\n");
        if (pAllSettings != NULL)
        {
            glv_SettingGroup_Delete_Loaded(&pAllSettings, &numAllSettings);
        }
        return -1;
    }
 
    // if no settings file existed before, then write one out
    if (pSettingsFile == NULL)
    {
        pSettingsFile = fopen("glvreplay_settings.txt", "w");
        if (pSettingsFile == NULL)
        {
            glv_LogWarn("Failed to open glvreplay_settings.txt for writing.\n");
        }
        else
        {
            glv_SettingGroup_save(pAllSettings, numAllSettings, pSettingsFile);
            fclose(pSettingsFile);
        }
    }

    // process snapshot if present
    if (fileHeader.contains_state_snapshot) {
        //TODO
    }

    // main loop
    Sequencer sequencer(traceFile);
    err = glv_replay::main_loop(sequencer, replayer, g_pReplaySettings->numLoops);

    for (int i = 0; i < GLV_MAX_TRACER_ID_ARRAY_SIZE; i++)
    {
        if (replayer[i] != NULL)
        {
            replayer[i]->Deinitialize();
            makeReplayer.Destroy(&replayer[i]);
        }
    }

    if (pAllSettings != NULL)
    {
        glv_SettingGroup_Delete_Loaded(&pAllSettings, &numAllSettings);
    }
    return err;
}
