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

#if  defined __linux__
#include <stdint.h>
#include <stddef.h>
typedef void * LPVOID;
typedef void * PVOID;
typedef void VOID;
typedef char CHAR;
typedef char TCHAR;
typedef long LONG;
typedef unsigned long ULONG;
typedef int BOOL;
typedef size_t SIZE_T;
typedef unsigned long DWORD;
typedef unsigned char BYTE;
typedef unsigned char *PBYTE;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;
typedef unsigned short WORD;
typedef DWORD * DWORD_PTR;
typedef DWORD *PDWORD;
typedef DWORD_PTR *PDWORD_PTR;
typedef int32_t INT32;
typedef int64_t LONG64;
typedef uint64_t ULONG64;
typedef const char * PCSTR;
typedef const wchar_t * PCWSTR;
#ifndef MAX_PATH
#include <limits.h>
#ifndef PATH_MAX
#define MAX_PATH 4096
#else
#define MAX_PATH PATH_MAX
#endif
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)            (((a) < (b)) ? (b) : (a))
#endif
#elif WIN32
#include <windows.h>
#endif

