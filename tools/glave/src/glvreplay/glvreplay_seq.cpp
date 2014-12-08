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
#include "glvreplay_seq.h"

extern "C" {
#include "glv_trace_packet_utils.h"
}

namespace glv_replay {

glv_trace_packet_header * Sequencer::get_next_packet()
{    
    glv_free(m_lastPacket);
    if (!m_pFile)
        return (NULL);
    m_lastPacket = glv_read_trace_packet(m_pFile);
    return(m_lastPacket);
}

void Sequencer::get_bookmark(seqBookmark &bookmark) {
    bookmark.file_offset = m_bookmark.file_offset;
}


void Sequencer::set_bookmark(const seqBookmark &bookmark) {
    fseek(m_pFile->mFile, m_bookmark.file_offset, SEEK_SET);
}

void Sequencer::record_bookmark()
{
    m_bookmark.file_offset = ftell(m_pFile->mFile);
}

} /* namespace glv_replay */
