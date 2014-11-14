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

#include "glv_interconnect.h"
#include "glv_common.h"

#include "glv_filelike.h"

const size_t kSendBufferSize = 1024 * 1024;

MessageStream* gMessageStream = NULL;

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// private functions
BOOL glv_MessageStream_SetupSocket(MessageStream* pStream);
BOOL glv_MessageStream_SetupHostSocket(MessageStream* pStream);
BOOL glv_MessageStream_SetupClientSocket(MessageStream* pStream);
BOOL glv_MessageStream_Handshake(MessageStream* pStream);
BOOL glv_MessageStream_ReallySend(MessageStream* pStream, const void* _bytes, size_t _size, BOOL _optional);
void glv_MessageStream_FlushSendBuffer(MessageStream* pStream, BOOL _optional);

// public functions
MessageStream* glv_MessageStream_create_port_string(BOOL _isHost, const char* _address, const char* _port)
{
    MessageStream* pStream;
    // make sure the strings are shorter than the destination buffer we have to store them!
    assert(strlen(_address) + 1 <= 64);
    assert(strlen(_port) + 1 <= 8);

    pStream = GLV_NEW(MessageStream);
    memcpy(pStream->mAddress, _address, strlen(_address) + 1);
    memcpy(pStream->mPort, _port, strlen(_port) + 1);

    pStream->mErrorNum = 0;
    memset(pStream->mSmallBuffer, 0, 64);
    pStream->mHost = _isHost;
    pStream->mHostAddressInfo = NULL;
    pStream->mNextPacketId = 0;
    pStream->mSocket = INVALID_SOCKET;
    pStream->mSendBuffer = NULL;

    if (glv_MessageStream_SetupSocket(pStream) == FALSE)
    {
        pStream->mErrorNum = GLV_WSAGetLastError();
    }

    return pStream;
}

MessageStream* glv_MessageStream_create(BOOL _isHost, const char* _address, unsigned int _port)
{
    char portBuf[32];
    memset(portBuf, 0, 32 * sizeof(char));
    sprintf(portBuf, "%u", _port);
    return glv_MessageStream_create_port_string(_isHost, _address, portBuf);
}

void glv_MessageStream_destroy(MessageStream** ppStream)
{
    if ((*ppStream)->mSendBuffer != NULL) { 
        // Try to get our data out.
        glv_MessageStream_FlushSendBuffer(*ppStream, TRUE);
        glv_SimpleBuffer_destroy(&(*ppStream)->mSendBuffer);
    }

    if ((*ppStream)->mHostAddressInfo != NULL)
    {
        freeaddrinfo((*ppStream)->mHostAddressInfo);
        (*ppStream)->mHostAddressInfo = NULL;
    }

#if defined(WIN32)
    WSACleanup();
#endif
    GLV_DELETE(*ppStream);
    (*ppStream) = NULL;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// private function implementations
BOOL glv_MessageStream_SetupSocket(MessageStream* pStream)
{
    BOOL result = TRUE;
#if defined(WIN32)
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
        result = FALSE;
    }
    else
#endif
    {
        if (pStream->mHost) {
            result = glv_MessageStream_SetupHostSocket(pStream);
        } else {
            result = glv_MessageStream_SetupClientSocket(pStream);
        }
    }
    return result;
}

BOOL glv_MessageStream_SetupHostSocket(MessageStream* pStream)
{
    int hr = 0;
#ifdef PLATFORM_LINUX
    int yes = 1;
#endif
    struct addrinfo hostAddrInfo = { 0 };
    SOCKET listenSocket;

    hostAddrInfo.ai_family = AF_INET;
    hostAddrInfo.ai_socktype = SOCK_STREAM;
    hostAddrInfo.ai_protocol = IPPROTO_TCP;
    hostAddrInfo.ai_flags = AI_PASSIVE;

    hr = getaddrinfo(NULL, pStream->mPort, &hostAddrInfo, &pStream->mHostAddressInfo);
    if (hr != 0) {
        glv_LogError("Host: Failed getaddrinfo\n");
        return FALSE;
    }

    listenSocket = socket(pStream->mHostAddressInfo->ai_family, pStream->mHostAddressInfo->ai_socktype, pStream->mHostAddressInfo->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        // TODO: Figure out errors
        glv_LogError("Host: Failed creating a listen socket\n");
        freeaddrinfo(pStream->mHostAddressInfo);
        pStream->mHostAddressInfo = NULL;
        return FALSE;
    }

#ifdef PLATFORM_LINUX
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif
    hr = bind(listenSocket, pStream->mHostAddressInfo->ai_addr, (int)pStream->mHostAddressInfo->ai_addrlen);
    if (hr == SOCKET_ERROR) {
        glv_LogError("Host: Failed binding socket err=%d\n", GLV_WSAGetLastError());
        freeaddrinfo(pStream->mHostAddressInfo);
        pStream->mHostAddressInfo = NULL;
        closesocket(listenSocket);
        return FALSE;
    }

    // Done with this.
    freeaddrinfo(pStream->mHostAddressInfo);
    pStream->mHostAddressInfo = NULL;

    hr = listen(listenSocket, 1);
    if (hr == SOCKET_ERROR) {
        glv_LogError("Host: Failed listening on socket err=%d\n");
        closesocket(listenSocket);
        return FALSE;
    }

    // Fo reals.
    pStream->mSocket = accept(listenSocket, NULL, NULL);
    closesocket(listenSocket);

    if (pStream->mSocket == INVALID_SOCKET) {
        glv_LogError("Host: Failed accepting socket connection\n");
        return FALSE;
    }

    if (glv_MessageStream_Handshake(pStream))
    {
        // TODO: The SendBuffer can cause big delays in sending messages back to the client.
        // We haven't verified if this improves performance in real applications,
        // so disable it for now.
        //pStream->mSendBuffer = glv_SimpleBuffer_create(kSendBufferSize);
        pStream->mSendBuffer = NULL;
    }
    else
    {
        glv_LogError("glv_MessageStream_SetupHostSocket failed handshake\n");
    }
    return TRUE;
}

// ------------------------------------------------------------------------------------------------
BOOL glv_MessageStream_SetupClientSocket(MessageStream* pStream)
{
    int hr = 0;
    unsigned int attempt = 0;
    BOOL bConnected = FALSE;
    struct addrinfo hostAddrInfo = { 0 },
        *currentAttempt = NULL;

    hostAddrInfo.ai_family = AF_UNSPEC;
    hostAddrInfo.ai_socktype = SOCK_STREAM;
    hostAddrInfo.ai_protocol = IPPROTO_TCP;

    hr = getaddrinfo(pStream->mAddress, pStream->mPort, &hostAddrInfo, &pStream->mHostAddressInfo);
    if (hr != 0) {
        glv_LogError("Client: Failed getaddrinfo\n");
        return FALSE;
    }

    // make several attempts to connect before bailing out
    for (attempt = 0; attempt < 10 && !bConnected; attempt++)
    {
        for (currentAttempt = pStream->mHostAddressInfo; currentAttempt != NULL; currentAttempt = currentAttempt->ai_next)
        {
            pStream->mSocket = socket(currentAttempt->ai_family, currentAttempt->ai_socktype, currentAttempt->ai_protocol);

            hr = connect(pStream->mSocket, currentAttempt->ai_addr, (int)currentAttempt->ai_addrlen);
            if (hr == SOCKET_ERROR)
            {
                glv_LogVerbose("Client: Failed connect. Possibly non-fatal.\n");
                closesocket(pStream->mSocket);
                pStream->mSocket = INVALID_SOCKET;
                continue;
            }

            bConnected = TRUE;
            break;
        }

        if (!bConnected)
        {
            Sleep(1);
            glv_LogVerbose("Client: Connect attempt %u on port %s failed, trying again.\n", attempt, pStream->mPort);
        }
        else
        {
            glv_LogVerbose("Client: Connected to port %s successfully\n", pStream->mPort);
        }
    }

    freeaddrinfo(pStream->mHostAddressInfo);
    pStream->mHostAddressInfo = NULL;

    if (pStream->mSocket == INVALID_SOCKET) {
        glv_LogError("Client: Couldn't find any connections.\n");
        return FALSE;
    }

    if (!glv_MessageStream_Handshake(pStream))
    {
        glv_LogError("Client: Failed handshake with host.\n");
        return FALSE;
    }
    return TRUE;
}

// ------------------------------------------------------------------------------------------------
BOOL glv_MessageStream_Handshake(MessageStream* pStream)
{
    BOOL result = TRUE;
    FileLike* fileLike = glv_FileLike_create_msg(pStream);
    Checkpoint* syn = glv_Checkpoint_create("It's a trap!");
    Checkpoint* ack = glv_Checkpoint_create(" - Admiral Ackbar");

    if (pStream->mHost) {
        glv_Checkpoint_write(syn, fileLike);
        result = glv_Checkpoint_read(ack, fileLike);
    } else {
        if (glv_Checkpoint_read(syn, fileLike))
        {
            glv_Checkpoint_write(ack, fileLike);
        }
        else
        {
            result = FALSE;
        }
    }

    // Turn on non-blocking modes for sockets now.
    if (result)
    {
#if defined(WIN32)
        u_long asyncMode = 1;
        ioctlsocket(pStream->mSocket, FIONBIO, &asyncMode);
#else
        fcntl(pStream->mSocket, F_SETFL, O_NONBLOCK);
#endif
    }

    GLV_DELETE(syn);
    GLV_DELETE(ack);
    GLV_DELETE(fileLike);

    return result;
}

// ------------------------------------------------------------------------------------------------
void glv_MessageStream_FlushSendBuffer(MessageStream* pStream, BOOL _optional)
{
    size_t bufferedByteSize = 0;
    const void* bufferBytes = glv_SimpleBuffer_GetBytes(pStream->mSendBuffer, &bufferedByteSize);
    if (bufferedByteSize > 0) {
        // TODO use return value from ReallySend
        glv_MessageStream_ReallySend(pStream, bufferBytes, bufferedByteSize, _optional);
        glv_SimpleBuffer_EmptyBuffer(pStream->mSendBuffer);
    }
}

// ------------------------------------------------------------------------------------------------
BOOL glv_MessageStream_BufferedSend(MessageStream* pStream, const void* _bytes, size_t _size, BOOL _optional)
{
    BOOL result = TRUE;
    if (pStream->mSendBuffer == NULL) {
        result = glv_MessageStream_ReallySend(pStream, _bytes, _size, _optional);
    }
    else
    {
        if (!glv_SimpleBuffer_WouldOverflow(pStream->mSendBuffer, _size)) {
            result = glv_SimpleBuffer_AddBytes(pStream->mSendBuffer, _bytes, _size);
        } else {
            // Time to flush the cache.
            glv_MessageStream_FlushSendBuffer(pStream, FALSE);

            // Check to see if the packet is larger than the send buffer 
            if (glv_SimpleBuffer_WouldOverflow(pStream->mSendBuffer, _size)) { 
                result = glv_MessageStream_ReallySend(pStream, _bytes, _size, _optional); 
            } else { 
                result = glv_SimpleBuffer_AddBytes(pStream->mSendBuffer, _bytes, _size);
            }
        }
    }
    return result;
}

// ------------------------------------------------------------------------------------------------
BOOL glv_MessageStream_Send(MessageStream* pStream, const void* _bytes, size_t _len)
{
    return glv_MessageStream_BufferedSend(pStream, _bytes, _len, FALSE);
}

// ------------------------------------------------------------------------------------------------
BOOL glv_MessageStream_ReallySend(MessageStream* pStream, const void* _bytes, size_t _size, BOOL _optional)
{
    size_t bytesSent = 0;
    assert(_size > 0);

    do {
        int sentThisTime = send(pStream->mSocket, (const char*)_bytes + bytesSent, (int)_size - (int)bytesSent, 0);
        if (sentThisTime == SOCKET_ERROR) {
            int socketError = GLV_WSAGetLastError();
            if (socketError == WSAEWOULDBLOCK) {
                // Try again. Don't sleep, because that nukes performance from orbit.
                continue;
            }

            if (!_optional) {
                return FALSE;
            } 
        }
        if (sentThisTime == 0) {
            if (!_optional) {
                return FALSE;
            }
            break;
        }

        bytesSent += sentThisTime;

    } while (bytesSent < _size);
    return TRUE;
}

// ------------------------------------------------------------------------------------------------
BOOL glv_MessageStream_Recv(MessageStream* pStream, void* _out, size_t _len)
{
    unsigned int totalDataRead = 0;
    do {
        int dataRead = recv(pStream->mSocket, ((char*)_out) + totalDataRead, (int)_len - totalDataRead, 0);
        if (dataRead == SOCKET_ERROR) {
            pStream->mErrorNum = GLV_WSAGetLastError();
            if (pStream->mErrorNum == WSAEWOULDBLOCK || pStream->mErrorNum == EAGAIN) {
                if (totalDataRead == 0) {
                    return FALSE;
                } else {
                    // I don't do partial reads--once I start receiving I wait for everything.
                    glv_LogDebug("Sleep on partial socket recv (%u bytes / %u), error num %d\n", totalDataRead, _len, pStream->mErrorNum);
                    Sleep(1);
                }
                // I've split these into two blocks because one of them is expected and the other isn't.
            } else if (pStream->mErrorNum == WSAECONNRESET) {
                // The remote client disconnected, probably not an issue.
                return FALSE;
            } else {
                // Some other wonky network error--place a breakpoint here.
                glv_LogError("Unexpected error (%d) while receiving message stream\n", pStream->mErrorNum);
                return FALSE;
            }
        } else {
            totalDataRead += dataRead;
        }
    } while (totalDataRead < _len);

    return TRUE;
}

// ------------------------------------------------------------------------------------------------
BOOL glv_MessageStream_BlockingRecv(MessageStream* pStream, void* _outBuffer, size_t _len)
{
    while (!glv_MessageStream_Recv(pStream, _outBuffer, _len)) {
        Sleep(1);
    }
    return TRUE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SimpleBuffer* glv_SimpleBuffer_create(size_t _bufferSize)
{
    SimpleBuffer* pBuffer = GLV_NEW(SimpleBuffer);
    pBuffer->mBuffer = (unsigned char*)glv_malloc(_bufferSize);
    if (pBuffer->mBuffer == NULL)
    {
        GLV_DELETE(pBuffer);
        return NULL;
    }

    pBuffer->mEnd = 0;
    pBuffer->mSize = _bufferSize;

    return pBuffer;
}

void glv_SimpleBuffer_destroy(SimpleBuffer** ppBuffer)
{
    glv_free((*ppBuffer)->mBuffer);
    GLV_DELETE(*ppBuffer);
}

BOOL glv_SimpleBuffer_AddBytes(SimpleBuffer* pBuffer, const void* _bytes, size_t _size)
{
    if (glv_SimpleBuffer_WouldOverflow(pBuffer, _size))
    { 
        return FALSE;
    }

    memcpy((unsigned char*)pBuffer->mBuffer + pBuffer->mEnd, _bytes, _size);
    pBuffer->mEnd += _size;

    return TRUE;
}

void glv_SimpleBuffer_EmptyBuffer(SimpleBuffer* pBuffer)
{
    pBuffer->mEnd = 0;
}

BOOL glv_SimpleBuffer_WouldOverflow(SimpleBuffer* pBuffer, size_t _requestedSize)
{
    return pBuffer->mEnd + _requestedSize > pBuffer->mSize;
}

const void* glv_SimpleBuffer_GetBytes(SimpleBuffer* pBuffer, size_t* _outByteCount)
{
    (*_outByteCount) = pBuffer->mEnd; 
    return pBuffer->mBuffer; 
}

//// ------------------------------------------------------------------------------------------------
//void RemoteCommand::Read(FileLike* _fileLike)
//{
//    unsigned int myCommand = 0;
//    _fileLike->Read(&myCommand);
//    mRemoteCommandType = (EnumRemoteCommand)myCommand;
//}
//
//// ------------------------------------------------------------------------------------------------
//void RemoteCommand::Write(FileLike* _fileLike) const
//{
//    _fileLike->Write((unsigned int)mRemoteCommandType);
//}
