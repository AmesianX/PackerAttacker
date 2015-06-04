#pragma once
#include "HookingEngine.h"
#include "TrackedMemoryBlock.h"
#include "SyncLock.h"
#include "ntdefs.h"

#include <Windows.h>
#include <map>

#define _HOOK_DEFINE_INTERNAL(reT, reTm, name, args) \
    typedef reT (reTm *_orig ## name) args; \
    _orig ## name orig ## name; \
    __declspec(dllexport) reT reTm on ## name args; \
    __declspec(dllexport) static reT reTm _on ## name args;
#define HOOK_DEFINE_2(reT, reTm, name, arg1, arg2) _HOOK_DEFINE_INTERNAL(reT, reTm, name, (arg1, arg2));
#define HOOK_DEFINE_5(reT, reTm, name, arg1, arg2, arg3, arg4, arg5) _HOOK_DEFINE_INTERNAL(reT, reTm, name, (arg1, arg2, arg3, arg4, arg5));
#define HOOK_DEFINE_6(reT, reTm, name, arg1, arg2, arg3, arg4, arg5, arg6) _HOOK_DEFINE_INTERNAL(reT, reTm, name, (arg1, arg2, arg3, arg4, arg5, arg6));
#define HOOK_DEFINE_8(reT, reTm, name, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) _HOOK_DEFINE_INTERNAL(reT, reTm, name, (arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8));
#define HOOK_DEFINE_10(reT, reTm, name, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) _HOOK_DEFINE_INTERNAL(reT, reTm, name, (arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10));
#define HOOK_DEFINE_12(reT, reTm, name, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) _HOOK_DEFINE_INTERNAL(reT, reTm, name, (arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12));

#define HOOK_GET_ORIG(object, library, name) object->orig ## name = (_orig ## name)GetProcAddress(LoadLibraryA(library), #name); assert(object->orig ## name);
#define HOOK_SET(object, hooks, name) hooks->placeHook(&(PVOID&)object->orig ## name, &_on ## name);


class UnpackingEngine
{
public:
    UnpackingEngine(void);
    ~UnpackingEngine(void);

    static UnpackingEngine* getInstance()
    {
        if (UnpackingEngine::instance == NULL)
            UnpackingEngine::instance = new UnpackingEngine();
        return UnpackingEngine::instance;
    }

    void initialize();
    void uninitialize();

private:
    static UnpackingEngine* instance;
    bool hooksReady, inAllocationHook, simulateDisabledDEP, bypassHooks;
    DWORD processID;
    HookingEngine* hooks;
    SyncLock* lock;

    std::vector<std::pair<DWORD, DWORD>> PESections;
    MemoryBlockTracker<TrackedMemoryBlock> writeablePEBlocks;
    MemoryBlockTracker<TrackedMemoryBlock> executableBlocks;
    MemoryBlockTracker<TrackedMemoryBlock> blacklistedBlocks;
    std::map<DWORD, MemoryBlockTracker<TrackedCopiedMemoryBlock>> remoteMemoryBlocks;
    std::map<DWORD, DWORD> suspendedThreads;

    void ignoreHooks(bool should) { this->bypassHooks = should; }
    bool shouldIgnoreHooks() { return this->bypassHooks; }
    void startTrackingPEMemoryBlocks();
    bool isPEMemory(DWORD address);
    void startTrackingRemoteMemoryBlock(DWORD pid, DWORD baseAddress, DWORD size, unsigned char* data);
    void dumpRemoteMemoryBlocks();
    void dumpMemoryBlock(TrackedMemoryBlock block, DWORD ep);
    void dumpMemoryBlock(char* fileName, DWORD size, const unsigned char* data);
    bool isSelfProcess(HANDLE process);
    DWORD getProcessIdIfRemote(HANDLE process);
    ULONG processMemoryBlockFromHook(const char* source, DWORD address, DWORD size, ULONG newProtection, ULONG oldProtection, bool considerOldProtection);

    /* NtProtectVirtualMemory hook */
    HOOK_DEFINE_5(NTSTATUS, NTAPI, NtProtectVirtualMemory, HANDLE, PVOID*, PULONG, ULONG, PULONG);
    /* NtWriteVirtualMemory hook */
    HOOK_DEFINE_5(NTSTATUS, NTAPI, NtWriteVirtualMemory, HANDLE, PVOID, PVOID, ULONG, PULONG);
    /* NtCreateThread hook */
    HOOK_DEFINE_8(NTSTATUS, NTAPI, NtCreateThread, 
                PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, HANDLE,
                PCLIENT_ID, PCONTEXT, PINITIAL_TEB, BOOLEAN);
    /* NtMapViewOfSection hook */
    HOOK_DEFINE_10(NTSTATUS, NTAPI, NtMapViewOfSection,
                HANDLE, HANDLE, PVOID*, ULONG, ULONG, PLARGE_INTEGER,
                OUT PULONG, SECTION_INHERIT, ULONG,  ULONG);
    /* NtResumeThread hook */
    HOOK_DEFINE_2(NTSTATUS, NTAPI, NtResumeThread, HANDLE, PULONG);
    /* CreateProcessInternal hook */
    HOOK_DEFINE_12(BOOL, WINAPI, CreateProcessInternalW,
                HANDLE, LPCWSTR, LPWSTR, LPSECURITY_ATTRIBUTES,
                LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCWSTR,
                LPSTARTUPINFOW, LPPROCESS_INFORMATION, PHANDLE);
    /* NtDelayExecution hook */
    HOOK_DEFINE_2(NTSTATUS, NTAPI, NtDelayExecution, BOOLEAN, PLARGE_INTEGER);
    /* NtAllocateVirtualMemory hook */
    HOOK_DEFINE_6(NTSTATUS, NTAPI, NtAllocateVirtualMemory, HANDLE, PVOID*, ULONG, PULONG, ULONG, ULONG);

    /* exception handler for hooking execution on tracked pages */
    long onShallowException(PEXCEPTION_POINTERS info);
    static long __stdcall _onShallowException(PEXCEPTION_POINTERS info);

    /* exception handler for detecting crashes */
    long onDeepException(PEXCEPTION_POINTERS info);
    static long __stdcall _onDeepException(PEXCEPTION_POINTERS info);

};

