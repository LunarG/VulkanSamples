//Copyright (c) 2007-2008, Marton Anka
//
//Permission is hereby granted, free of charge, to any person obtaining a 
//copy of this software and associated documentation files (the "Software"), 
//to deal in the Software without restriction, including without limitation 
//the rights to use, copy, modify, merge, publish, distribute, sublicense, 
//and/or sell copies of the Software, and to permit persons to whom the 
//Software is furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included 
//in all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
//OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
//THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
//FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
//IN THE SOFTWARE.
#pragma once
 
#if defined(__linux__)
#if defined(__i386__)
#define _M_IX86
#elif defined(__x86_64__)
#define _M_IX86_X64
#endif
#elif defined(WIN32)
#ifdef _M_IX86
#define _M_IX86_X64
#elif defined _M_X64
#define _M_IX86_X64
#endif
#endif

#include "wintypes.h"
#ifdef __cplusplus
extern "C" {
#endif

void Mhook_Initialize();

BOOL Mhook_SetHook(PVOID *ppSystemFunction, PVOID pHookFunction);
BOOL Mhook_Unhook(PVOID *ppHookedFunction);

// If you'll be hooking (or unhooking) a large number of functions, you can
// make it faster by wrapping them all with one call to Begin/End MultiOperation
// bStillSuspendThreads should be true if you need each call to SetHook to try
// and suspend other threads, and FALSE if you know the other threads will already
// be suspended.
VOID Mhook_BeginMultiOperation(BOOL bStillSuspendThreads);
VOID Mhook_EndMultiOperation();

#ifdef __cplusplus
}
#endif
