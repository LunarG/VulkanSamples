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
extern "C" {
#include "glv_common.h"
#include "glv_tracelog.h"
#include "glv_filelike.h"
#include "glv_trace_packet_utils.h"
}
#include "glvreplay_factory.h"
#include "glvreplay_seq.h"
#include "glvreplay_window.h"
#include "getopt/getopt.h"

const static char *shortOptions = "hb";

enum {
    DEBUG_OPT = 1
};
const static struct option longOptions[] = {
        {"help", no_argument, 0, 'h'},
        {"benchmark", no_argument, 0, 'b'},
        {"debug", required_argument, 0, DEBUG_OPT},
        //TODO add more options
        {0, 0, 0, 0}};

static void usage(const char *argv0)
{
   glv_LogInfo("%s [options] [trace file]\n",argv0);
   glv_LogInfo("-h | --help print usage\n");
   glv_LogInfo("-d debug_level 0-N higher is more debug features\n");
   //TODO add details
}

namespace glv_replay {
int main_loop(Sequencer &seq, glv_trace_packet_replay_library *replayerArray[])
{
    int err = 0;
    glv_trace_packet_header *packet;
    unsigned int res;
    glv_trace_packet_replay_library *replayer;
    glv_trace_packet_message* msgPacket;

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
                    res = replayer->Replay(packet);
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

    return err;
}
} // namespace glv_replay

using namespace glv_replay;

extern "C"
int main(int argc, char **argv)
{
    //bool hasSnapshot = false;
    //bool fastReplay = false;
    unsigned int debugLevel = 1;
    int err;

    // parse command line options
    int opt;
    while ((opt = getopt_long_only(argc, argv, shortOptions, longOptions, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            return 0;
        case 'b':
            //fastReplay = true;
            break;
        case DEBUG_OPT:
            debugLevel = atoi(optarg);
            break;
        default:
            glv_LogError("unknown option %d\n", opt);
            usage(argv[0]);
            return 1;
        }
    }

    // open trace file and read in header
    glv_trace_file_header fileHeader;
    FILE *tracefp;

    if (optind < argc  && strlen(argv[optind]) > 0) {
        tracefp = fopen(argv[optind], "rb");
        if (tracefp == NULL) {
            glv_LogError("Cannot open trace file: '%s'\n", argv[optind]);
            return 1;
        }
    }
    else {
        glv_LogError("No trace file specified\n");
        usage(argv[0]);
        return 1;
    }

    FileLike* traceFile = glv_FileLike_create_file(tracefp);
    if (glv_FileLike_ReadRaw(traceFile, &fileHeader, sizeof(fileHeader)) == false)
    {
        glv_LogError("Unable to read header from file.\n");
        GLV_DELETE(traceFile);
        return 1;
    }
    
    // Make sure trace file version is supported
    if (fileHeader.trace_file_version < GLV_TRACE_FILE_VERSION_MINIMUM_COMPATIBLE)
    {
        glv_LogError("Trace file version %su is older than minimum compatible version (%su).\nYou'll need to make a new trace file, or use an older replayer.\n", fileHeader.trace_file_version, GLV_TRACE_FILE_VERSION_MINIMUM_COMPATIBLE);
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
        uint8_t tracerId = fileHeader.tracer_id_array[i];
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
                return err;
            }

            // Initalize the replayer
            err = replayer[tracerId]->Initialize(&disp, debugLevel);
            if (err) {
                glv_LogError("Couldn't Initialize replayer for TracerId %d.\n", tracerId);
                return err;
            }
        }
    }

    if (tidApi == GLV_TID_RESERVED) {
        glv_LogError("No API specified in tracefile for replaying.\n");
        return -1;
    }
 
    // process snapshot if present
    if (fileHeader.contains_state_snapshot) {
        //TODO
    }

    // main loop
    Sequencer sequencer(traceFile);
    err = glv_replay::main_loop(sequencer, replayer);

    for (int i = 0; i < GLV_MAX_TRACER_ID_ARRAY_SIZE; i++)
    {
        if (replayer[i] != NULL)
        {
            replayer[i]->Deinitialize();
            makeReplayer.Destroy(&replayer[i]);
        }
    }
    return err;
}
