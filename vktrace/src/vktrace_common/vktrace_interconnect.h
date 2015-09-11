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

#pragma once

#include <errno.h>
#include "vktrace_common.h"

#if defined(PLATFORM_POSIX)
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <sys/socket.h>
    #define SOCKET int
    #define INVALID_SOCKET 0
    #define SOCKET_ERROR -1
    #define closesocket close
    #define VKTRACE_WSAGetLastError() errno
    #define WSAEWOULDBLOCK EWOULDBLOCK
    #define WSAEAGAIN EAGAIN
    #define WSAECONNRESET ECONNRESET
#elif defined(WIN32)
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment (lib, "Ws2_32.lib")
    #define VKTRACE_WSAGetLastError() WSAGetLastError()
#endif

static const unsigned int VKTRACE_BASE_PORT = 34199;
struct SSerializeDataPacket;

struct SimpleBuffer;

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
typedef struct MessageStream
{
    SOCKET mSocket;
    struct addrinfo* mHostAddressInfo;
    size_t mNextPacketId;
    struct SimpleBuffer* mSendBuffer;

    // Used if someone asks for a receive of a small string.
    char mSmallBuffer[64];

    char mAddress[64];

    char mPort[8];

    BOOL mHost;
    int mErrorNum;
} MessageStream;

#ifdef __cplusplus
extern "C" {
#endif
MessageStream* vktrace_MessageStream_create_port_string(BOOL _isHost, const char* _address, const char* _port);
MessageStream* vktrace_MessageStream_create(BOOL _isHost, const char* _address, unsigned int _port);
void vktrace_MessageStream_destroy(MessageStream** ppStream);
BOOL vktrace_MessageStream_BufferedSend(MessageStream* pStream, const void* _bytes, size_t _size, BOOL _optional);
BOOL vktrace_MessageStream_Send(MessageStream* pStream, const void* _bytes, size_t _len);

BOOL vktrace_MessageStream_Recv(MessageStream* pStream, void* _out, size_t _len);
BOOL vktrace_MessageStream_BlockingRecv(MessageStream* pStream, void* _outBuffer, size_t _len);
#ifdef __cplusplus
}
#endif
extern MessageStream* gMessageStream;

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
typedef struct SimpleBuffer
{
    void* mBuffer;
    size_t mEnd;
    size_t mSize;
} SimpleBuffer;

SimpleBuffer* vktrace_SimpleBuffer_create(size_t _bufferSize);
void vktrace_SimpleBuffer_destroy(SimpleBuffer** ppBuffer);
BOOL vktrace_SimpleBuffer_AddBytes(SimpleBuffer* pBuffer, const void* _bytes, size_t _size);
void vktrace_SimpleBuffer_EmptyBuffer(SimpleBuffer* pBuffer);
BOOL vktrace_SimpleBuffer_WouldOverflow(SimpleBuffer* pBuffer, size_t _requestedSize);
const void* vktrace_SimpleBuffer_GetBytes(SimpleBuffer* pBuffer, size_t* _outByteCount);
