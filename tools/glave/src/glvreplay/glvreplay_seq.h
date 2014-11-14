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
#pragma once

extern "C" {
#include "glv_filelike.h"
#include "glv_trace_packet_identifiers.h"
}

/* Class to handle fetching and sequencing packets from a tracefile.
 * Contains no knowledge of type of tracer needed to process packet.
 * Requires low level file/stream reading/seeking support. */
namespace glv_replay {


struct seqBookmark
{
    unsigned int file_offset;
    uint64_t next_index;
};


// replay Sequencer interface
 class AbstractSequencer
 {
 public:
    virtual ~AbstractSequencer() {}
    virtual glv_trace_packet_header *get_next_packet() = 0;
    virtual void get_bookmark(seqBookmark &bookmark) = 0;
    virtual void set_bookmark(const seqBookmark &bookmark) = 0;
 };

class Sequencer: public AbstractSequencer
{

public:
    Sequencer(FileLike* pFile) : m_lastPacket(NULL), m_pFile(pFile) {}
    ~Sequencer() { delete m_lastPacket;}
    
    glv_trace_packet_header *get_next_packet();
    void get_bookmark(seqBookmark &bookmark);
    void set_bookmark(const seqBookmark &bookmark);
    
private:
    glv_trace_packet_header *m_lastPacket;
    seqBookmark m_bookmark;
    FileLike *m_pFile;
    
};

} /* namespace glv_replay */


