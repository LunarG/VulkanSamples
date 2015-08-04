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

#include <wintypes.h>
#if defined(WIN32)
#include <tlhelp32.h>
#elif defined(__linux__)
#include <pthread.h>
#include <wchar.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#define PAGE_EXECUTE_READWRITE (PROT_READ | PROT_EXEC | PROT_WRITE)
#else
#error unsupported platform
#endif

#include <stdio.h>
#include "mhook.h"
#include "../disasm-lib/disasm.h"

//=========================================================================
#ifndef cntof
#define cntof(a) (sizeof(a)/sizeof(a[0]))
#endif

//=========================================================================
#ifndef GOOD_HANDLE
#define GOOD_HANDLE(a) ((a!=INVALID_HANDLE_VALUE)&&(a!=NULL))
#endif

//=========================================================================
unsigned int gle()
{
#if defined(WIN32)
    return GetLastError();
#else
    return errno;
#endif
}

//=========================================================================
#ifndef ODPRINTF

#ifdef _DEBUG
#define ODPRINTF(a) odprintf a
#else
#define ODPRINTF(a)
#endif

#ifdef __linux__
inline void odprintf(PCSTR format, ...) {
	va_list	args;
	va_start(args, format);
	vprintf(format, args);
        printf("\n");
	va_end(args);
}

#else
void __cdecl odprintf(char* format, ...) {
	va_list	args;
	int len;
	PSTR buf;
	va_start(args, format);
	len = _vscprintf(format, args);
	if (len > 0) {
		len += (1 + 2);
		buf = (PSTR) malloc(len);
		if (buf) {
			len = vsprintf_s(buf, len, format, args);
			if (len > 0) {
				while (len && isspace(buf[len-1])) len--;
				buf[len++] = '\r';
				buf[len++] = '\n';
				buf[len] = 0;
				OutputDebugString(buf);
			}
			free(buf);
		}
		va_end(args);
	}
}
#endif
#endif //#ifndef ODPRINTF

//=========================================================================
#define MHOOKS_MAX_CODE_BYTES	32
#define MHOOKS_MAX_RIPS			 4

//=========================================================================
// The trampoline structure - stores every bit of info about a hook
struct MHOOKS_TRAMPOLINE;
typedef struct MHOOKS_TRAMPOLINE MHOOKS_TRAMPOLINE;
struct MHOOKS_TRAMPOLINE {
	PBYTE	pSystemFunction;								// the original system function
	DWORD	cbOverwrittenCode;								// number of bytes overwritten by the jump
	PBYTE	pHookFunction;									// the hook function that we provide
	BYTE	codeJumpToHookFunction[MHOOKS_MAX_CODE_BYTES];	// placeholder for code that jumps to the hook function
	BYTE	codeTrampoline[MHOOKS_MAX_CODE_BYTES];			// placeholder for code that holds the first few
															//   bytes from the system function and a jump to the remainder
															//   in the original location
	BYTE	codeUntouched[MHOOKS_MAX_CODE_BYTES];			// placeholder for unmodified original code
															//   (we patch IP-relative addressing)
	MHOOKS_TRAMPOLINE* pPrevTrampoline;						// When in the free list, thess are pointers to the prev and next entry.
	MHOOKS_TRAMPOLINE* pNextTrampoline;						// When not in the free list, this is a pointer to the prev and next trampoline in use.
};

//=========================================================================
// The patch data structures - store info about rip-relative instructions
// during hook placement
struct MHOOKS_RIPINFO;
typedef struct MHOOKS_RIPINFO MHOOKS_RIPINFO;
struct MHOOKS_RIPINFO
{
	DWORD	dwOffset;
	S64		nDisplacement;
};

struct MHOOKS_PATCHDATA;
typedef struct MHOOKS_PATCHDATA MHOOKS_PATCHDATA;
struct MHOOKS_PATCHDATA
{
	S64				nLimitUp;
	S64				nLimitDown;
	DWORD			nRipCnt;
	MHOOKS_RIPINFO	rips[MHOOKS_MAX_RIPS];
};

//=========================================================================
// Global vars
static BOOL g_bVarsInitialized = FALSE;
#if defined(__linux__)
static pthread_mutex_t g_cs;
static void* g_hThreadHandles = NULL;
#elif defined(WIN32)
static CRITICAL_SECTION g_cs;
static HANDLE* g_hThreadHandles = NULL;
#endif
static MHOOKS_TRAMPOLINE* g_pHooks = NULL;
static MHOOKS_TRAMPOLINE* g_pFreeList = NULL;
static DWORD g_nHooksInUse = 0;
static DWORD g_nThreadHandles = 0;
static BOOL g_bSuspendThreads = TRUE;
static BOOL g_bMustGrabCritSec = TRUE;

#define MHOOK_JMPSIZE 5
#define MHOOK_MINALLOCSIZE 4096

#ifdef WIN32
//=========================================================================
// Toolhelp defintions so the functions can be dynamically bound to
typedef HANDLE (WINAPI * _CreateToolhelp32Snapshot)(
	DWORD dwFlags,	   
	DWORD th32ProcessID  
	);

typedef BOOL (WINAPI * _Thread32First)(
									   HANDLE hSnapshot,	 
									   LPTHREADENTRY32 lpte
									   );

typedef BOOL (WINAPI * _Thread32Next)(
									  HANDLE hSnapshot,	 
									  LPTHREADENTRY32 lpte
									  );

//=========================================================================
// Bring in the toolhelp functions from kernel32
_CreateToolhelp32Snapshot fnCreateToolhelp32Snapshot = NULL;//(_CreateToolhelp32Snapshot) GetProcAddress(GetModuleHandle("kernel32"), "CreateToolhelp32Snapshot");
_Thread32First fnThread32First = NULL;//(_Thread32First) GetProcAddress(GetModuleHandle("kernel32"), "Thread32First");
_Thread32Next fnThread32Next = NULL;//(_Thread32Next) GetProcAddress(GetModuleHandle("kernel32"), "Thread32Next");

void Mhook_Initialize()
{
	fnCreateToolhelp32Snapshot = (_CreateToolhelp32Snapshot) GetProcAddress(GetModuleHandle("kernel32"), "CreateToolhelp32Snapshot");
	fnThread32First = (_Thread32First) GetProcAddress(GetModuleHandle("kernel32"), "Thread32First");
	fnThread32Next = (_Thread32Next) GetProcAddress(GetModuleHandle("kernel32"), "Thread32Next");
}
#endif
//=========================================================================
// Internal function:
//
// Remove the trampoline from the specified list, updating the head pointer
// if necessary.
//=========================================================================
static VOID ListRemove(MHOOKS_TRAMPOLINE** pListHead, MHOOKS_TRAMPOLINE* pNode) {
	if (pNode->pPrevTrampoline) {
		pNode->pPrevTrampoline->pNextTrampoline = pNode->pNextTrampoline;
	}

	if (pNode->pNextTrampoline) {
		pNode->pNextTrampoline->pPrevTrampoline = pNode->pPrevTrampoline;
	}

	if ((*pListHead) == pNode) {
		(*pListHead) = pNode->pNextTrampoline;
		assert((*pListHead)->pPrevTrampoline == NULL);
	}

	pNode->pPrevTrampoline = NULL;
	pNode->pNextTrampoline = NULL;
}

//=========================================================================
// Internal function:
//
// Prepend the trampoline from the specified list and update the head pointer.
//=========================================================================
static VOID ListPrepend(MHOOKS_TRAMPOLINE** pListHead, MHOOKS_TRAMPOLINE* pNode) {
	pNode->pPrevTrampoline = NULL;
	pNode->pNextTrampoline = (*pListHead);
	if ((*pListHead)) {
		(*pListHead)->pPrevTrampoline = pNode;
	}
	(*pListHead) = pNode;
}

//=========================================================================
static VOID EnterCritSec() {
	if (!g_bVarsInitialized) {
#if defined(WIN32)
		InitializeCriticalSection(&g_cs);
#elif defined(__linux__)
		pthread_mutex_init(&g_cs, NULL);
#endif

		g_bVarsInitialized = TRUE;
	}
#if defined(WIN32)
	EnterCriticalSection(&g_cs);
#elif defined(__linux__)
	pthread_mutex_lock(&g_cs );
#endif
}

//=========================================================================
static VOID LeaveCritSec()
{
#if defined(WIN32)
	LeaveCriticalSection(&g_cs);
#elif defined(__linux__)
	pthread_mutex_unlock(&g_cs );
#endif
}

//=========================================================================
static size_t GetAllocGranularity()
{
	// Always allocate in bulk, in case the system actually has a smaller allocation granularity than MINALLOCSIZE.
#if defined(WIN32)
	SYSTEM_INFO sSysInfo =  {0};
	GetSystemInfo(&sSysInfo);

	return max(sSysInfo.dwAllocationGranularity, MHOOK_MINALLOCSIZE);
#elif defined(__linux__)
	// There is no situation where the allocation granularity is higher than the page size of the system
	// on Linux, so you should be able to safely use PAGE_SIZE
	return max(getpagesize(), MHOOK_MINALLOCSIZE);
#endif
}

//=========================================================================
//  return of NULL means fatal  error, return of (void *) -1 means couldn't alloc
//  the size at the requested area)
static void * MapVirtualMem(const size_t allocSize, void * pbAlloc)
{
#if defined(WIN32)
	// determine current state
	MEMORY_BASIC_INFORMATION mbi;
	ODPRINTF(("mhooks: BlockAlloc: Looking at address %p", pbAlloc));
	if (!VirtualQuery(pbAlloc, &mbi, sizeof(mbi)))
		return NULL;
	// free & large enough?
	if (mbi.State == MEM_FREE && mbi.RegionSize >= (unsigned)allocSize) {
		// and then try to allocate it
		return VirtualAlloc(pbAlloc, allocSize, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	} else {
		return (void *) -1;
	}
#elif defined(__linux__)
	return mmap(pbAlloc, allocSize, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
}

//=========================================================================
// Internal function:
// 
// Skip over jumps that lead to the real function. Gets around import
// jump tables, etc.
//=========================================================================
static PBYTE SkipJumps(PBYTE pbCode) {
#ifdef _M_IX86_X64
	if (pbCode[0] == 0xff && pbCode[1] == 0x25) {
#ifdef _M_IX86
		// on x86 we have an absolute pointer...
		PBYTE pbTarget = *(PBYTE *)&pbCode[2];
		// ... that shows us an absolute pointer.
		return SkipJumps(*(PBYTE *)pbTarget);
#elif defined _M_IX86_X64
		// on x64 we have a 32-bit offset...
		INT32 lOffset = *(INT32 *)&pbCode[2];
		// ... that shows us an absolute pointer
		return SkipJumps(*(PBYTE*)(pbCode + 6 + lOffset));
#endif
	} else if (pbCode[0] == 0xe9) {
		// here the behavior is identical, we have...
		// ...a 32-bit offset to the destination.
		return SkipJumps(pbCode + 5 + *(INT32 *)&pbCode[1]);
	} else if (pbCode[0] == 0xeb) {
		// and finally an 8-bit offset to the destination
		return SkipJumps(pbCode + 2 + *(CHAR *)&pbCode[1]);
	}
#else
#error unsupported platform
#endif
	return pbCode;
}

//=========================================================================
// Internal function:
//
// Writes code at pbCode that jumps to pbJumpTo. Will attempt to do this
// in as few bytes as possible. Important on x64 where the long jump
// (0xff 0x25 ....) can take up 14 bytes.
//=========================================================================
static PBYTE EmitJump(PBYTE pbCode, PBYTE pbJumpTo) {
#ifdef _M_IX86_X64
	PBYTE pbJumpFrom = pbCode + 5;
	SIZE_T cbDiff = pbJumpFrom > pbJumpTo ? pbJumpFrom - pbJumpTo : pbJumpTo - pbJumpFrom;
	ODPRINTF(("mhooks: EmitJump: Jumping from %p to %p, diff is %p", pbJumpFrom, pbJumpTo, cbDiff));
	if (cbDiff <= 0x7fff0000) {
		pbCode[0] = 0xe9;
		pbCode += 1;
		*((PDWORD)pbCode) = (DWORD)(DWORD_PTR)(pbJumpTo - pbJumpFrom);
		pbCode += sizeof(DWORD);
	} else {
		pbCode[0] = 0xff;
		pbCode[1] = 0x25;
		pbCode += 2;
#ifdef _M_IX86
		// on x86 we write an absolute address (just behind the instruction)
		*((PDWORD)pbCode) = (DWORD)(DWORD_PTR)(pbCode + sizeof(DWORD));
#elif defined _M_X64
		// on x64 we write the relative address of the same location
		*((PDWORD)pbCode) = (DWORD)0;
#endif
		pbCode += sizeof(DWORD);
		*((PDWORD_PTR)pbCode) = (DWORD_PTR)(pbJumpTo);
		pbCode += sizeof(DWORD_PTR);
	}
#else 
#error unsupported platform
#endif
	return pbCode;
}


//=========================================================================
// Internal function:
//
// Round down to the next multiple of rndDown
//=========================================================================
static size_t RoundDown(size_t addr, size_t rndDown)
{
	return (addr / rndDown) * rndDown;
}

//=========================================================================
// Internal function:
//
// Will attempt allocate a block of memory within the specified range, as 
// near as possible to the specified function.
//=========================================================================
static MHOOKS_TRAMPOLINE* BlockAlloc(PBYTE pSystemFunction, PBYTE pbLower, PBYTE pbUpper) {
	ptrdiff_t cAllocSize, bytesToOffset;
	PBYTE pModuleGuess;
	PBYTE pbAlloc;
	MHOOKS_TRAMPOLINE* pRetVal = NULL;
	int loopCount = 0;

	cAllocSize = GetAllocGranularity();

	pModuleGuess = (PBYTE) RoundDown((size_t)pSystemFunction, cAllocSize);

	for (pbAlloc = pModuleGuess; pbLower < pbAlloc && pbAlloc < pbUpper; ++loopCount) {
	pRetVal = (MHOOKS_TRAMPOLINE*) MapVirtualMem(cAllocSize, pbAlloc);
	if(!pRetVal)
		break;
	if (pRetVal != (void *) -1) {
		size_t s;
		size_t trampolineCount = cAllocSize / sizeof(MHOOKS_TRAMPOLINE);
		ODPRINTF(("mhooks: BlockAlloc: Allocated block at %p as %d trampolines", pRetVal, trampolineCount));

		pRetVal[0].pPrevTrampoline = NULL;
		pRetVal[0].pNextTrampoline = &pRetVal[1];

		// prepare them by having them point down the line at the next entry
		for (s = 1; s < trampolineCount; ++s) {
			pRetVal[s].pPrevTrampoline = &pRetVal[s - 1];
			pRetVal[s].pNextTrampoline = &pRetVal[s + 1];
		}

		// last entry points to the current head of the free list
		pRetVal[trampolineCount - 1].pNextTrampoline = g_pFreeList;
		break;
	    }
				
	    // This is a spiral, should be -1, 1, -2, 2, -3, 3, etc. (* cAllocSize)
	    bytesToOffset = (cAllocSize * (loopCount + 1) * ((loopCount % 2 == 0) ? -1 : 1));
	    pbAlloc = pbAlloc + bytesToOffset;
	}
	
	return pRetVal;
}

//=========================================================================
// Internal function:
//
// Will try to allocate a big block of memory inside the required range. 
//=========================================================================
static MHOOKS_TRAMPOLINE* FindTrampolineInRange(PBYTE pLower, PBYTE pUpper) {

	// This is a standard free list, except we're doubly linked to deal with soem return shenanigans.
	MHOOKS_TRAMPOLINE* curEntry = g_pFreeList;
	if (!g_pFreeList) {
		return NULL;
	}

	while (curEntry) {
		if ((MHOOKS_TRAMPOLINE*) pLower < curEntry && curEntry < (MHOOKS_TRAMPOLINE*) pUpper) {
			ListRemove(&g_pFreeList, curEntry);

			return curEntry;
		}

		curEntry = curEntry->pNextTrampoline;
	}

	return NULL;
}

//=========================================================================
// Internal function:
//
// Will try to allocate the trampoline structure within 2 gigabytes of
// the target function. 
//=========================================================================
static MHOOKS_TRAMPOLINE* TrampolineAlloc(PBYTE pSystemFunction, S64 nLimitUp, S64 nLimitDown) {

	MHOOKS_TRAMPOLINE* pTrampoline = NULL;
	PBYTE pUpper;
	// determine lower and upper bounds for the allocation locations.
	// in the basic scenario this is +/- 2GB but IP-relative instructions
	// found in the original code may require a smaller window.
	PBYTE pLower = pSystemFunction + nLimitUp;
	pLower = pLower < (PBYTE)(DWORD_PTR)0x0000000080000000 ? 
						(PBYTE)(0x1) : (PBYTE)(pLower - (PBYTE)0x7fff0000);
	pUpper = pSystemFunction + nLimitDown;
	pUpper = pUpper < (PBYTE)(DWORD_PTR)0xffffffff80000000 ? 
		(PBYTE)(pUpper + 0x7ff80000) : (PBYTE)(DWORD_PTR)0xfffffffffff80000;
	ODPRINTF(("mhooks: TrampolineAlloc: Allocating for %p between %p and %p", pSystemFunction, pLower, pUpper));

	// try to find a trampoline in the specified range
	pTrampoline = FindTrampolineInRange(pLower, pUpper);
	if (!pTrampoline) {
		// if it we can't find it, then we need to allocate a new block and 
		// try again. Just fail if that doesn't work 
		g_pFreeList = BlockAlloc(pSystemFunction, pLower, pUpper);
		pTrampoline = FindTrampolineInRange(pLower, pUpper);
	}

	// found and allocated a trampoline?
	if (pTrampoline) {
		ListPrepend(&g_pHooks, pTrampoline);
	}

	return pTrampoline;
}

//=========================================================================
// Internal function:
//
// Return the internal trampoline structure that belongs to a hooked function.
//=========================================================================
static MHOOKS_TRAMPOLINE* TrampolineGet(PBYTE pHookedFunction) {
	MHOOKS_TRAMPOLINE* pCurrent = g_pHooks;

	while (pCurrent) {
		if (pCurrent->pHookFunction == pHookedFunction) {
			return pCurrent;
		}

		pCurrent = pCurrent->pNextTrampoline;
	}

	return NULL;
}

static void * GetCurProcess()
{
#ifdef WIN32
	return (void *) GetCurrentProcess();
#else
	return(NULL);
#endif
}

static void FlushCache(void * process, void * address, size_t size)
{
#if defined(WIN32)
	FlushInstructionCache(process, address, size);
#elif defined(__linux__)
	// TODO do we really need to flush the Instruction cache on x86 on Linux?
#else
#error unsupported platform  
#endif
}

static BOOL MemProtect(void * process, void * address, size_t size, DWORD newProtect, DWORD *oldProtect)
{
#if defined(WIN32)
	return (VirtualProtectEx(process, address, size, newProtect, oldProtect));
#elif defined(__linux__)
	*oldProtect = PROT_READ | PROT_EXEC;
	if (mprotect(address, size, newProtect))
		return TRUE;
	else
		return FALSE;
#else
#error unsupported platform  
#endif
}
//=========================================================================
// Internal function:
//
// Free a trampoline structure.
//=========================================================================
static VOID TrampolineFree(MHOOKS_TRAMPOLINE* pTrampoline, BOOL bNeverUsed) {
	ListRemove(&g_pHooks, pTrampoline);

	// If a thread could feasinbly have some of our trampoline code 
	// on its stack and we yank the region from underneath it then it will
	// surely crash upon returning. So instead of freeing the 
	// memory we just let it leak. Ugly, but safe.
	if (bNeverUsed) {
		ListPrepend(&g_pFreeList, pTrampoline);
	}

	g_nHooksInUse--;
}

//=========================================================================
// Internal function:
//
// Suspend a given thread and try to make sure that its instruction
// pointer is not in the given range.
//=========================================================================
static HANDLE SuspendOneThread(DWORD dwThreadId, PBYTE pbCode, DWORD cbBytes) {
	// open the thread
	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, dwThreadId);
	if (GOOD_HANDLE(hThread)) {
		// attempt suspension
		DWORD dwSuspendCount = SuspendThread(hThread);
		if (dwSuspendCount != -1) {
			// see where the IP is
			CONTEXT ctx;
			int nTries = 0;
			ctx.ContextFlags = CONTEXT_CONTROL;

			while (GetThreadContext(hThread, &ctx)) {
#ifdef _M_IX86
				PBYTE pIp = (PBYTE)(DWORD_PTR)ctx.Eip;
#elif defined _M_X64
				PBYTE pIp = (PBYTE)(DWORD_PTR)ctx.Rip;
#endif
				if (pIp >= pbCode && pIp < (pbCode + cbBytes)) {
					if (nTries < 3) {
						// oops - we should try to get the instruction pointer out of here. 
						ODPRINTF(("mhooks: SuspendOneThread: suspended thread %d - IP is at %p - IS COLLIDING WITH CODE", dwThreadId, pIp));
						ResumeThread(hThread);
						Sleep(100);
						SuspendThread(hThread);
						nTries++;
					} else {
						// we gave it all we could. (this will probably never 
						// happen - unless the thread has already been suspended 
						// to begin with)
						ODPRINTF(("mhooks: SuspendOneThread: suspended thread %d - IP is at %p - IS COLLIDING WITH CODE - CAN'T FIX", dwThreadId, pIp));
						ResumeThread(hThread);
						CloseHandle(hThread);
						hThread = NULL;
						break;
					}
				} else {
					// success, the IP is not conflicting
					ODPRINTF(("mhooks: SuspendOneThread: Successfully suspended thread %d - IP is at %p", dwThreadId, pIp));
					break;
				}
			}
		} else {
			// couldn't suspend
			CloseHandle(hThread);
			hThread = NULL;
		}
	}
	return hThread;
}

//=========================================================================
// Internal function:
//
// Resumes all previously suspended threads in the current process.
//=========================================================================
static VOID ResumeOtherThreads() {
	DWORD i;
	// make sure things go as fast as possible
	INT nOriginalPriority = GetThreadPriority(GetCurrentThread());
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	// go through our list
	for (i=0; i<g_nThreadHandles; i++) {
		// and resume & close thread handles
		ResumeThread(g_hThreadHandles[i]);
		CloseHandle(g_hThreadHandles[i]);
	}
	// clean up
	free(g_hThreadHandles);
	g_hThreadHandles = NULL;
	g_nThreadHandles = 0;
	SetThreadPriority(GetCurrentThread(), nOriginalPriority);
}

//=========================================================================
// Internal function:
//
// Suspend all threads in this process while trying to make sure that their 
// instruction pointer is not in the given range.
//=========================================================================
static BOOL SuspendOtherThreads(PBYTE pbCode, DWORD cbBytes) {
	BOOL bRet = FALSE;
	HANDLE hSnap;
	// make sure we're the most important thread in the process
	INT nOriginalPriority = GetThreadPriority(GetCurrentThread());
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	// get a view of the threads in the system
	hSnap = fnCreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId());
	if (GOOD_HANDLE(hSnap)) {
		THREADENTRY32 te;
		DWORD nThreadsInProcess;
		te.dwSize = sizeof(te);
		// count threads in this process (except for ourselves)
		nThreadsInProcess = 0;
		if (fnThread32First(hSnap, &te)) {
			do {
				if (te.th32OwnerProcessID == GetCurrentProcessId()) {
					if (te.th32ThreadID != GetCurrentThreadId()) {
						nThreadsInProcess++;
					}
				}
				te.dwSize = sizeof(te);
			} while(fnThread32Next(hSnap, &te));
		}
		ODPRINTF(("mhooks: SuspendOtherThreads: counted %d other threads", nThreadsInProcess));
		if (nThreadsInProcess) {
			// alloc buffer for the handles we really suspended
			g_hThreadHandles = (HANDLE*)malloc(nThreadsInProcess*sizeof(HANDLE));
			if (g_hThreadHandles) {
				DWORD nCurrentThread = 0;
				BOOL bFailed = FALSE;

				ZeroMemory(g_hThreadHandles, nThreadsInProcess*sizeof(HANDLE));
				te.dwSize = sizeof(te);
				// go through every thread
				if (fnThread32First(hSnap, &te)) {
					do {
						if (te.th32OwnerProcessID == GetCurrentProcessId()) {
							if (te.th32ThreadID != GetCurrentThreadId()) {
								// attempt to suspend it
								g_hThreadHandles[nCurrentThread] = SuspendOneThread(te.th32ThreadID, pbCode, cbBytes);
								if (GOOD_HANDLE(g_hThreadHandles[nCurrentThread])) {
									ODPRINTF(("mhooks: SuspendOtherThreads: successfully suspended %d", te.th32ThreadID));
									nCurrentThread++;
								} else {
									ODPRINTF(("mhooks: SuspendOtherThreads: error while suspending thread %d: %d", te.th32ThreadID, gle()));
									// TODO: this might not be the wisest choice
									// but we can choose to ignore failures on
									// thread suspension. It's pretty unlikely that
									// we'll fail - and even if we do, the chances
									// of a thread's IP being in the wrong place
									// is pretty small.
									// bFailed = TRUE;
								}
							}
						}
						te.dwSize = sizeof(te);
					} while(fnThread32Next(hSnap, &te) && !bFailed);
				}
				g_nThreadHandles = nCurrentThread;
				bRet = !bFailed;
			}
		}
		CloseHandle(hSnap);
		//TODO: we might want to have another pass to make sure all threads
		// in the current process (including those that might have been
		// created since we took the original snapshot) have been 
		// suspended.
	} else {
		ODPRINTF(("mhooks: SuspendOtherThreads: can't CreateToolhelp32Snapshot: %d", gle()));
	}
	SetThreadPriority(GetCurrentThread(), nOriginalPriority);
	if (!bRet) {
		ODPRINTF(("mhooks: SuspendOtherThreads: Had a problem (or not running multithreaded), resuming all threads."));
		ResumeOtherThreads();
	}
	return bRet;
}

//=========================================================================
// if IP-relative addressing has been detected, fix up the code so the
// offset points to the original location
static void FixupIPRelativeAddressing(PBYTE pbNew, PBYTE pbOriginal, MHOOKS_PATCHDATA* pdata)
{
#if defined _M_X64
	S64 diff = pbNew - pbOriginal;
		DWORD i;
	for (i = 0; i < pdata->nRipCnt; i++) {
		DWORD dwNewDisplacement = (DWORD)(pdata->rips[i].nDisplacement - diff);
		ODPRINTF(("mhooks: fixing up RIP instruction operand for code at 0x%p: "
			"old displacement: 0x%8.8x, new displacement: 0x%8.8x", 
			pbNew + pdata->rips[i].dwOffset, 
			(DWORD)pdata->rips[i].nDisplacement, 
			dwNewDisplacement));
		*(PDWORD)(pbNew + pdata->rips[i].dwOffset) = dwNewDisplacement;
	}
#endif
}

//=========================================================================
// Examine the machine code at the target function's entry point, and
// skip bytes in a way that we'll always end on an instruction boundary.
// We also detect branches and subroutine calls (as well as returns)
// at which point disassembly must stop.
// Finally, detect and collect information on IP-relative instructions
// that we can patch.
static DWORD DisassembleAndSkip(PVOID pFunction, DWORD dwMinLen, MHOOKS_PATCHDATA* pdata) {
	DWORD dwRet = 0;

#ifdef _M_IX86
	ARCHITECTURE_TYPE arch = ARCH_X86;
#elif defined _M_IX86_X64
	ARCHITECTURE_TYPE arch = ARCH_X64;
#else
	#error unsupported platform
#endif
	DISASSEMBLER dis;
	pdata->nLimitDown = 0;
	pdata->nLimitUp = 0;
	pdata->nRipCnt = 0;
	if (InitDisassembler(&dis, arch)) {
		INSTRUCTION* pins = NULL;
		U8* pLoc = (U8*)pFunction;
		DWORD dwFlags = DISASM_DECODE | DISASM_DISASSEMBLE | DISASM_ALIGNOUTPUT;

		ODPRINTF(("mhooks: DisassembleAndSkip: Disassembling %p", pLoc));
		while ( (dwRet < dwMinLen) && (pins = GetInstruction(&dis, (ULONG_PTR)pLoc, pLoc, dwFlags)) ) {
#if defined _M_IX86_X64
			BOOL bProcessRip;
#endif
			ODPRINTF(("mhooks: DisassembleAndSkip: %p: %s", pLoc, pins->String));
			if (pins->Type == ITYPE_RET		) break;
			if (pins->Type == ITYPE_BRANCH	) break;
			if (pins->Type == ITYPE_BRANCHCC) break;
			if (pins->Type == ITYPE_CALL	) break;
			if (pins->Type == ITYPE_CALLCC	) break;

			#if defined _M_IX86_X64
				bProcessRip = FALSE;
				// mov or lea to register from rip+imm32
				if ((pins->Type == ITYPE_MOV || pins->Type == ITYPE_LEA) && (pins->X86.Relative) && 
					(pins->X86.OperandSize == 8) && (pins->OperandCount == 2) &&
					(pins->Operands[1].Flags & OP_IPREL) && (pins->Operands[1].Register == AMD64_REG_RIP))
				{
					// rip-addressing "mov reg, [rip+imm32]"
					ODPRINTF(("mhooks: DisassembleAndSkip: found OP_IPREL on operand %d with displacement 0x%x (in memory: 0x%x)", 1, pins->X86.Displacement, *(PDWORD)(pLoc+3)));
					bProcessRip = TRUE;
				}
				// mov or lea to rip+imm32 from register
				else if ((pins->Type == ITYPE_MOV || pins->Type == ITYPE_LEA) && (pins->X86.Relative) && 
					(pins->X86.OperandSize == 8) && (pins->OperandCount == 2) &&
					(pins->Operands[0].Flags & OP_IPREL) && (pins->Operands[0].Register == AMD64_REG_RIP))
				{
					// rip-addressing "mov [rip+imm32], reg"
					ODPRINTF(("mhooks: DisassembleAndSkip: found OP_IPREL on operand %d with displacement 0x%x (in memory: 0x%x)", 0, pins->X86.Displacement, *(PDWORD)(pLoc+3)));
					bProcessRip = TRUE;
				}
				else if ( (pins->OperandCount >= 1) && (pins->Operands[0].Flags & OP_IPREL) )
				{
					DWORD i;
					// unsupported rip-addressing
					ODPRINTF(("mhooks: DisassembleAndSkip: found unsupported OP_IPREL on operand %d", 0));
					// dump instruction bytes to the debug output
					for (i=0; i<pins->Length; i++) {
						ODPRINTF(("mhooks: DisassembleAndSkip: instr byte %2.2d: 0x%2.2x", i, pLoc[i]));
					}
					break;
				}
				else if ( (pins->OperandCount >= 2) && (pins->Operands[1].Flags & OP_IPREL) )
				{
					DWORD i;
					// unsupported rip-addressing
					ODPRINTF(("mhooks: DisassembleAndSkip: found unsupported OP_IPREL on operand %d", 1));
					// dump instruction bytes to the debug output
					for (i=0; i<pins->Length; i++) {
						ODPRINTF(("mhooks: DisassembleAndSkip: instr byte %2.2d: 0x%2.2x", i, pLoc[i]));
					}
					break;
				}
				else if ( (pins->OperandCount >= 3) && (pins->Operands[2].Flags & OP_IPREL) )
				{
					DWORD i;
					// unsupported rip-addressing
					ODPRINTF(("mhooks: DisassembleAndSkip: found unsupported OP_IPREL on operand %d", 2));
					// dump instruction bytes to the debug output
					for (i=0; i<pins->Length; i++) {
						ODPRINTF(("mhooks: DisassembleAndSkip: instr byte %2.2d: 0x%2.2x", i, pLoc[i]));
					}
					break;
				}
				// follow through with RIP-processing if needed
				if (bProcessRip) {
					// calculate displacement relative to function start
					S64 nAdjustedDisplacement = pins->X86.Displacement + (pLoc - (U8*)pFunction);
					// store displacement values furthest from zero (both positive and negative)
					if (nAdjustedDisplacement < pdata->nLimitDown)
						pdata->nLimitDown = nAdjustedDisplacement;
					if (nAdjustedDisplacement > pdata->nLimitUp)
						pdata->nLimitUp = nAdjustedDisplacement;
					// store patch info
					if (pdata->nRipCnt < MHOOKS_MAX_RIPS) {
						pdata->rips[pdata->nRipCnt].dwOffset = dwRet + 3;
						pdata->rips[pdata->nRipCnt].nDisplacement = pins->X86.Displacement;
						pdata->nRipCnt++;
					} else {
						// no room for patch info, stop disassembly
						break;
					}
				}
			#endif

			dwRet += pins->Length;
			pLoc  += pins->Length;
		}

		CloseDisassembler(&dis);
	}

	return dwRet;
}

//=========================================================================
BOOL Mhook_SetHook(PVOID *ppSystemFunction, PVOID pHookFunction) {
	DWORD dwInstructionLength;
	MHOOKS_TRAMPOLINE* pTrampoline = NULL;
	PVOID pSystemFunction = *ppSystemFunction;
	MHOOKS_PATCHDATA patchdata;
	patchdata.nLimitDown= 0;
	patchdata.nLimitUp = 0;
	patchdata.nRipCnt = 0;

	// ensure thread-safety
	if (g_bMustGrabCritSec) {
		EnterCritSec();
	}
	ODPRINTF(("mhooks: Mhook_SetHook: Started on the job: %p / %p", pSystemFunction, pHookFunction));
	// find the real functions (jump over jump tables, if any)
	pSystemFunction = SkipJumps((PBYTE)pSystemFunction);
	pHookFunction   = SkipJumps((PBYTE)pHookFunction);
	ODPRINTF(("mhooks: Mhook_SetHook: Started on the job: %p / %p", pSystemFunction, pHookFunction));
	// figure out the length of the overwrite zone

	dwInstructionLength = DisassembleAndSkip(pSystemFunction, MHOOK_JMPSIZE, &patchdata);
	if (dwInstructionLength >= MHOOK_JMPSIZE) {
		ODPRINTF(("mhooks: Mhook_SetHook: disassembly signals %d bytes", dwInstructionLength));
		// suspend every other thread in this process, and make sure their IP 
		// is not in the code we're about to overwrite.
		// Note: The caller might've told us not to worry about this, if they're sure no other threads
		// could be running--for example when hooking functions in a suspended process.
		if (g_bSuspendThreads) {
			SuspendOtherThreads((PBYTE)pSystemFunction, dwInstructionLength);
		}
		// allocate a trampoline structure (TODO: it is pretty wasteful to get
		// VirtualAlloc to grab chunks of memory smaller than 100 bytes)
		pTrampoline = TrampolineAlloc((PBYTE)pSystemFunction, patchdata.nLimitUp, patchdata.nLimitDown);
		if (pTrampoline) {
			// open ourselves so we can VirtualProtectEx
			void * hProc = GetCurProcess();
			DWORD dwOldProtectSystemFunction = 0;
			DWORD dwOldProtectTrampolineFunction = 0;
			ODPRINTF(("mhooks: Mhook_SetHook: allocated structure at %p", pTrampoline));
			// set the system function to PAGE_EXECUTE_READWRITE
			if (MemProtect(hProc, pSystemFunction, dwInstructionLength, PAGE_EXECUTE_READWRITE, &dwOldProtectSystemFunction)) {
				ODPRINTF(("mhooks: Mhook_SetHook: readwrite set on system function"));
				// mark our trampoline buffer to PAGE_EXECUTE_READWRITE
				if (MemProtect(hProc, pTrampoline, sizeof(MHOOKS_TRAMPOLINE), PAGE_EXECUTE_READWRITE, &dwOldProtectTrampolineFunction)) {
					PBYTE pbCode;
					DWORD i;
					DWORD_PTR dwDistance;
					ODPRINTF(("mhooks: Mhook_SetHook: readwrite set on trampoline structure"));

					// create our trampoline function
					pbCode = pTrampoline->codeTrampoline;
					// save original code..
					for (i = 0; i<dwInstructionLength; i++) {
						pTrampoline->codeUntouched[i] = pbCode[i] = ((PBYTE)pSystemFunction)[i];
					}
					pbCode += dwInstructionLength;
					// plus a jump to the continuation in the original location
					pbCode = EmitJump(pbCode, ((PBYTE)pSystemFunction) + dwInstructionLength);
					ODPRINTF(("mhooks: Mhook_SetHook: updated the trampoline"));

					// fix up any IP-relative addressing in the code
					FixupIPRelativeAddressing(pTrampoline->codeTrampoline, (PBYTE)pSystemFunction, &patchdata);

					dwDistance = (PBYTE)pHookFunction < (PBYTE)pSystemFunction ? 
						(PBYTE)pSystemFunction - (PBYTE)pHookFunction : (PBYTE)pHookFunction - (PBYTE)pSystemFunction;
					if (dwDistance > 0x7fff0000) {
						// create a stub that jumps to the replacement function.
						// we need this because jumping from the API to the hook directly 
						// will be a long jump, which is 14 bytes on x64, and we want to 
						// avoid that - the API may or may not have room for such stuff. 
						// (remember, we only have 5 bytes guaranteed in the API.)
						// on the other hand we do have room, and the trampoline will always be
						// within +/- 2GB of the API, so we do the long jump in there. 
						// the API will jump to the "reverse trampoline" which
						// will jump to the user's hook code.
						pbCode = pTrampoline->codeJumpToHookFunction;
						pbCode = EmitJump(pbCode, (PBYTE)pHookFunction);
						ODPRINTF(("mhooks: Mhook_SetHook: created reverse trampoline"));
						FlushCache(hProc, pTrampoline->codeJumpToHookFunction, 
							pbCode - pTrampoline->codeJumpToHookFunction);

						// update the API itself
						pbCode = (PBYTE)pSystemFunction;
						pbCode = EmitJump(pbCode, pTrampoline->codeJumpToHookFunction);
					} else {
						// the jump will be at most 5 bytes so we can do it directly
						// update the API itself
						pbCode = (PBYTE)pSystemFunction;
						pbCode = EmitJump(pbCode, (PBYTE)pHookFunction);
					}

					// update data members
					pTrampoline->cbOverwrittenCode = dwInstructionLength;
					pTrampoline->pSystemFunction = (PBYTE)pSystemFunction;
					pTrampoline->pHookFunction = (PBYTE)pHookFunction;

					// flush instruction cache and restore original protection
					FlushCache(hProc, pTrampoline->codeTrampoline, dwInstructionLength);
					MemProtect(hProc, pTrampoline, sizeof(MHOOKS_TRAMPOLINE), dwOldProtectTrampolineFunction, &dwOldProtectTrampolineFunction);
				} else {
					ODPRINTF(("mhooks: Mhook_SetHook: failed VirtualProtectEx 2: %d", gle()));
				}
				// flush instruction cache and restore original protection
				FlushCache(hProc, pSystemFunction, dwInstructionLength);
				MemProtect(hProc, pSystemFunction, dwInstructionLength, dwOldProtectSystemFunction, &dwOldProtectSystemFunction);
			} else {
				ODPRINTF(("mhooks: Mhook_SetHook: failed VirtualProtectEx 1: %d", gle()));
			}
			if (pTrampoline->pSystemFunction) {
				// this is what the application will use as the entry point
				// to the "original" unhooked function.
				*ppSystemFunction = pTrampoline->codeTrampoline;
				ODPRINTF(("mhooks: Mhook_SetHook: Hooked the function!"));
			} else {
				// if we failed discard the trampoline (forcing VirtualFree)
				TrampolineFree(pTrampoline, TRUE);
				pTrampoline = NULL;
			}
		}
		// resume everybody else
        if (g_bSuspendThreads) {
		    ResumeOtherThreads();
        }
	} else {
		ODPRINTF(("mhooks: disassembly signals %u bytes (unacceptable)", dwInstructionLength));
	}
	if (g_bMustGrabCritSec) {
		LeaveCritSec();
	}
	return (pTrampoline != NULL);
}

//=========================================================================
BOOL Mhook_Unhook(PVOID *ppHookedFunction) {
	BOOL bRet = FALSE;
	MHOOKS_TRAMPOLINE* pTrampoline;
	ODPRINTF(("mhooks: Mhook_Unhook: %p", *ppHookedFunction));
	if (g_bMustGrabCritSec) {
		EnterCritSec();
	}
	// get the trampoline structure that corresponds to our function
	pTrampoline = TrampolineGet((PBYTE)*ppHookedFunction);
	if (pTrampoline) {
		void * hProc;
		DWORD dwOldProtectSystemFunction;
		// make sure nobody's executing code where we're about to overwrite a few bytes
		if (g_bSuspendThreads) {
			SuspendOtherThreads(pTrampoline->pSystemFunction, pTrampoline->cbOverwrittenCode);
		}
		ODPRINTF(("mhooks: Mhook_Unhook: found struct at %p", pTrampoline));
		// open ourselves so we can VirtualProtectEx
		hProc = GetCurProcess();
		dwOldProtectSystemFunction = 0;
		// make memory writable
		if (MemProtect(hProc, pTrampoline->pSystemFunction, pTrampoline->cbOverwrittenCode, PAGE_EXECUTE_READWRITE, &dwOldProtectSystemFunction)) {
			DWORD i;
			PBYTE pbCode = (PBYTE)pTrampoline->pSystemFunction;
			ODPRINTF(("mhooks: Mhook_Unhook: readwrite set on system function"));
			for (i = 0; i<pTrampoline->cbOverwrittenCode; i++) {
				pbCode[i] = pTrampoline->codeUntouched[i];
			}
			// flush instruction cache and make memory unwritable
			FlushCache(hProc, pTrampoline->pSystemFunction, pTrampoline->cbOverwrittenCode);
			MemProtect(hProc, pTrampoline->pSystemFunction, pTrampoline->cbOverwrittenCode, dwOldProtectSystemFunction, &dwOldProtectSystemFunction);
			// return the original function pointer
			*ppHookedFunction = pTrampoline->pSystemFunction;
			bRet = TRUE;
			ODPRINTF(("mhooks: Mhook_Unhook: sysfunc: %p", *ppHookedFunction));
			// free the trampoline while not really discarding it from memory
			TrampolineFree(pTrampoline, FALSE);
			ODPRINTF(("mhooks: Mhook_Unhook: unhook successful"));
		} else {
			ODPRINTF(("mhooks: Mhook_Unhook: failed VirtualProtectEx 1: %d", gle()));
		}
		if (g_bSuspendThreads) {
		    // make the other guys runnable
		    ResumeOtherThreads();
		}
	}
	if (g_bMustGrabCritSec) {
		LeaveCritSec();
	}
	return bRet;
}

//=========================================================================
VOID Mhook_BeginMultiOperation(BOOL bStillSuspendThreads)
{
	EnterCritSec();
	g_bSuspendThreads = bStillSuspendThreads;
	g_bMustGrabCritSec = FALSE;
}

VOID Mhook_EndMultiOperation()
{
	g_bMustGrabCritSec = TRUE;
	g_bSuspendThreads = TRUE;
	LeaveCritSec();
}
