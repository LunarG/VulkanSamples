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
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Peter Lohrmann <peterl@valvesoftware.com>
 **************************************************************************/
#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define VKTRACE_NEW(type) (type*)vktrace_malloc(sizeof(type))
#define VKTRACE_NEW_ARRAY(type, count) (type*)vktrace_malloc(sizeof(type)*count)
#define VKTRACE_DELETE(ptr) vktrace_free(ptr);
#define VKTRACE_REALLOC(ptr, size) vktrace_realloc(ptr, size);

static void* vktrace_malloc(size_t size)
{
    void* pMemory;
    if (size == 0)
        return NULL;

    pMemory = malloc(size);

    return pMemory;
}

static void vktrace_free(void* ptr)
{
    free(ptr);
    ptr = NULL;
}

static void * vktrace_realloc(void *ptr,size_t size)
{
    void *pMemory;
    if (size == 0)
        return NULL;

    pMemory = realloc(ptr, size);
    return pMemory;
}

static char* vktrace_allocate_and_copy(const char* _src)
{
    if (_src == NULL)
    {
        return NULL;
    }
    else
    {
        size_t bufferSize = 1 + strlen(_src);

        char* retVal = VKTRACE_NEW_ARRAY(char, bufferSize);
#ifdef WIN32
        strcpy_s(retVal, bufferSize, _src);
#else // linux
        strncpy(retVal, _src, bufferSize);
#endif

        return retVal;
    }
}

static char* vktrace_allocate_and_copy_n(const char* _src, int _count)
{
    size_t bufferSize = 1 + _count;

    char* retVal = VKTRACE_NEW_ARRAY(char, bufferSize);

#ifdef WIN32
    strncpy_s(retVal, bufferSize, _src, _count);
#else // linux
    strncpy(retVal, _src, _count);
    retVal[_count] = '\0';
#endif

    return retVal;
}

static char* vktrace_copy_and_append(const char* pBaseString, const char* pSeparator, const char* pAppendString)
{
    size_t baseSize = (pBaseString != NULL) ? strlen(pBaseString) : 0;
    size_t separatorSize = ((pAppendString != NULL) && strlen(pAppendString) && (pSeparator != NULL)) ?
                           strlen(pSeparator) : 0;
    size_t appendSize = (pAppendString != NULL) ? strlen(pAppendString) : 0;
    size_t bufferSize = baseSize + separatorSize + appendSize + 1;
    char* retVal = VKTRACE_NEW_ARRAY(char, bufferSize);
    if (retVal != NULL)
    {
#ifdef WIN32
        strncpy_s(retVal, bufferSize, pBaseString, baseSize);
        strncpy_s(&retVal[baseSize], bufferSize-baseSize, pSeparator, separatorSize);
        strncpy_s(&retVal[baseSize+separatorSize], bufferSize-baseSize-separatorSize, pAppendString, appendSize);
#else // linux
        strncpy(retVal, pBaseString, baseSize);
        strncpy(&retVal[baseSize], pSeparator, separatorSize);
        strncpy(&retVal[baseSize+separatorSize], pAppendString, appendSize);
#endif
    }
    retVal[bufferSize-1] = '\0';
    return retVal;
}

static char* vktrace_copy_and_append_args(const char* pBaseString, const char* pSeparator, const char* pAppendFormat, va_list args)
{
    size_t baseSize = (pBaseString != NULL) ? strlen(pBaseString) : 0;
    size_t separatorSize = (pSeparator != NULL) ? strlen(pSeparator) : 0;
    size_t appendSize = 0;
    size_t bufferSize = 0;
    char* retVal = NULL;

#if defined(WIN32)
    appendSize = _vscprintf(pAppendFormat, args);
#elif defined(PLATFORM_LINUX)
    va_list argcopy;
    va_copy(argcopy, args);
    appendSize = vsnprintf(NULL, 0, pAppendFormat, argcopy);
    va_end(argcopy);
#endif

    bufferSize = baseSize + separatorSize + appendSize + 1;
    retVal = VKTRACE_NEW_ARRAY(char, bufferSize);
    if (retVal != NULL)
    {
#ifdef WIN32
        strncpy_s(retVal, bufferSize, pBaseString, baseSize);
        strncpy_s(&retVal[baseSize], bufferSize-baseSize, pSeparator, separatorSize);
        _vsnprintf_s(&retVal[baseSize+separatorSize], bufferSize-baseSize-separatorSize, appendSize, pAppendFormat, args);
#else // linux
        strncpy(retVal, pBaseString, baseSize);
        strncpy(&retVal[baseSize], pSeparator, separatorSize);
        vsnprintf(&retVal[baseSize+separatorSize], appendSize, pAppendFormat, args);
#endif
    }
    return retVal;
}

