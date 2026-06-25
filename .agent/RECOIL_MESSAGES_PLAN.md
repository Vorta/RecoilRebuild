# messages.dll Source-Faithful Reimplementation Plan

## Purpose

This file is the companion-binary work breakdown for reconstructing messages.dll as source-faithful native C/C++.
Function and data addresses remain stable evidence keys; source owners and data owners are the implementation units.

## End Goal

Use the currently loaded `messages.bndb` and `support/messages.dll` as the source of truth for behavior, ABI, layout, imports, resources, and original assembly.
Seeded entries are inventory only; marker promotion requires current Binary Ninja/source/build evidence.

## Plan Usage Rules

- Use `python tools/recoil.py plan <subcommand> --binary messages ...` to navigate or update this plan.
- Use `python tools/recoil.py status --binary messages 0xNNNNNN` for focused entry status.
- Authored function entries start with not-done markers until reconstructed with evidence.
- Data entries record current BN `.data` shape but keep source-owner and reimplementation markers not done.
- Provider-boundary seed hints are not accepted provider evidence; promote them only after provider classification review.

## Group Catalog

Groups are dependency ordered: provider/runtime hints, authored or unresolved `.text` functions, then `.data` variables.

## G001. messages.dll compiler, CRT, and DLL startup providers

- Group ID: messages.provider
- Kind: provider-seed
- Primary source: messages.bndb
- Validation: provider boundary classification and VC5 DLL startup/import evidence
- Depends on: none

- 0x10002610:
  - [✅] Reconstructed (Name: __unlock)
  - [✅] Provider-boundary (Kind: compiler/CRT/runtime; Name: __unlock; Origin: VC5 CRT lock release wrapper; BN library symbol, indexes CRT lock table and calls imported LeaveCriticalSection; File: external; Target: pending; Group: messages.provider)

## G002. messages.dll authored and unresolved .text functions

- Group ID: messages.text
- Kind: source
- Primary source: messages.bndb
- Validation: BN function reconstruction plus source/build/byte evidence
- Depends on: none

- 0x10001000:
  - [✅] Reconstructed (Name: _DllMain@12)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: DllMain; Origin: LIBCMT:dllmain.obj default _DllMain@12 provider stub; File: external; Target: pending; Group: messages.text)

- 0x10001010:
  - [✅] Reconstructed (Name: ZLocGetID)
  - [✅] Source dependencies satisfied
  - [✅] Source owner (Kind: subsystem; Parent: messages.lookup_subsystem; State: implemented)
  - [✅] Data reimplemented
  - [✅] Reimplemented [S]
    - Name: ZLocGetID;
    - File: src/Messages/messages.c;
    - Target: zloc_get_id_messages_lookup;
    - Group: messages.text;
    - Model: source-faithful;
    - Blocker: none

- 0x10001070:
  - [✅] Reconstructed (Name: __CRT_INIT@12)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: __CRT_INIT; Origin: LIBCMT:dllcrt0.obj __CRT_INIT@12 CRT initialization/termination helper; File: external; Target: pending; Group: messages.text)

- 0x10001190:
  - [✅] Reconstructed (Name: __DllMainCRTStartup@12)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: __DllMainCRTStartup; Origin: LIBCMT:dllcrt0.obj __DllMainCRTStartup@12 DLL CRT startup entrypoint; File: external; Target: pending; Group: messages.text)

- 0x10001240:
  - [✅] Reconstructed (Name: __amsg_exit)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: __amsg_exit; Origin: LIBCMT runtime-message fatal-exit helper; calls _FF_MSGBANNER, _NMSG_WRITE, and __aexit_rtn with standard R60xx runtime-message data; File: external; Target: pending; Group: messages.text)

- 0x10001280:
  - [✅] Reconstructed (Name: _cinit)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: _cinit; Origin: VC5 CRT startup/initializer routine; calls optional _FPinit and _initterm over __xi/__xc ranges from __CRT_INIT.; File: external; Target: pending; Group: messages.text)

- 0x100012b0:
  - [✅] Reconstructed (Name: _exit)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: _exit; Origin: VC5 CRT exit wrapper stored in __aexit_rtn; calls doexit(status, quick=1, retcaller=0).; File: external; Target: pending; Group: messages.text)

- 0x100012d0:
  - [✅] Reconstructed (Name: _cexit)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: _cexit; Origin: VC5 CRT C termination wrapper; __CRT_INIT detach path calls doexit(0, quick=0, retcaller=1).; File: external; Target: pending; Group: messages.text)

- 0x100012e0:
  - [✅] Reconstructed (Name: doexit)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: doexit; Origin: VC5 CRT process-exit core; uses exit lock, _onexit walk, __xp/__xt tables, TerminateProcess/ExitProcess, and exit flags.; File: external; Target: pending; Group: messages.text)

- 0x100013a0:
  - [✅] Reconstructed (Name: lockexit)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: lockexit; Origin: VC5 CRT exit-lock helper; calls lock routine with lock id 0x0d.; File: external; Target: pending; Group: messages.text)

- 0x100013b0:
  - [✅] Reconstructed (Name: unlockexit)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: unlockexit; Origin: VC5 CRT exit-lock release helper; calls __unlock(0x0d).; File: external; Target: pending; Group: messages.text)

- 0x100013c0:
  - [✅] Reconstructed (Name: _initterm)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: _initterm; Origin: VC5 CRT initializer-table walker; iterates PVFV entries in [first,last) and calls non-null functions.; File: external; Target: pending; Group: messages.text)

- 0x100013e0:
  - [✅] Reconstructed (Name: __mtinit)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: __mtinit; Origin: VC5 CRT multithread/TLS initializer; allocates TLS index and _tiddata, stores current thread id and handle sentinel.; File: external; Target: pending; Group: messages.text)

- 0x10001440:
  - [✅] Reconstructed (Name: __mtterm)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: __mtterm; Origin: VC5 CRT multithread/TLS termination helper; BN shows __mtdeletelocks, TlsFree(__tlsindex), and reset to 0xffffffff; File: external; Target: pending; Group: messages.text)

- 0x10001470:
  - [✅] Reconstructed (Name: _initptd)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _initptd; Origin: VC5 CRT TLS per-thread-data initializer; writes _tiddata.xcptacttab to _XcptActTab and holdrand seed; File: external; Target: pending; Group: messages.text)

- 0x10001490:
  - [✅] Reconstructed (Name: _getptd)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _getptd; Origin: VC5 CRT per-thread data/TLS runtime support; uses __tlsindex, _calloc_crt, _initptd, LastError preservation, and _RT_THREAD fatal path; File: external; Target: pending; Group: messages.text)

- 0x10001510:
  - [✅] Reconstructed (Name: _freeptd)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _freeptd; Origin: VC5 CRT per-thread data cleanup; frees _tiddata heap fields, non-default exception action table, and clears TLS slot; File: external; Target: pending; Group: messages.text)

- 0x100015c0:
  - [✅] Reconstructed (Name: _ioinit)
  - [✅] Provider-boundary (Kind: CRT; Name: _ioinit; Origin: VC5 static CRT lowio initialization; File: external; Target: pending; Group: messages.text)

- 0x100017d0:
  - [✅] Reconstructed (Name: _ioterm)
  - [✅] Provider-boundary (Kind: CRT; Name: _ioterm; Origin: VC5 CRT / LIBCMT lowio termination routine paired with _ioinit; File: external; Target: pending; Group: messages.text)

- 0x10001830:
  - [✅] Reconstructed (Name: _setenvp)
  - [✅] Provider-boundary (Kind: CRT; Name: _setenvp; Origin: VC5 static CRT environment initialization; builds __initenv from _aenvptr during __CRT_INIT; File: external; Target: pending; Group: messages.text)

- 0x10001920:
  - [✅] Reconstructed (Name: __setargv)
  - [✅] Provider-boundary (Kind: CRT; Name: __setargv; Origin: VC5 static CRT argv initialization during __CRT_INIT; sets __argc/__argv from _acmdln using parse_cmdline; File: external; Target: pending; Group: messages.text)

- 0x100019c0:
  - [✅] Reconstructed (Name: parse_cmdline)
  - [✅] Provider-boundary (Kind: CRT; Name: parse_cmdline; Origin: VC5 static CRT command-line parser helper for __setargv with _mbctype DBCS lead-byte handling; File: external; Target: pending; Group: messages.text)

- 0x10001bd0:
  - [✅] Reconstructed (Name: _setmbcp)
  - [✅] Provider-boundary (Kind: CRT; Name: _setmbcp; Origin: Microsoft VC5-era CRT multibyte codepage setup; locks CRT multibyte state, maps _MB_CP sentinels, updates _mbctype/_mbcasemap/__mbcodepage, and calls GetCPInfo; File: external; Target: pending; Group: messages.text)

- 0x10001e00:
  - [✅] Reconstructed (Name: crt_getSystemCP)
  - [✅] Provider-boundary (Kind: CRT; Name: crt_getSystemCP; Origin: Microsoft VC5-era CRT helper mapping _MB_CP_OEM/_MB_CP_ANSI/_MB_CP_LOCALE through GetOEMCP/GetACP/__lc_codepage; File: external; Target: pending; Group: messages.text)

- 0x10001e50:
  - [✅] Reconstructed (Name: crt_getLCIDFromCodePage)
  - [✅] Provider-boundary (Kind: CRT; Name: crt_getLCIDFromCodePage; Origin: Microsoft VC5-era CRT helper mapping known MBCS code pages to LCIDs; File: external; Target: pending; Group: messages.text)

- 0x10001eb0:
  - [✅] Reconstructed (Name: crt_setSBCS)
  - [✅] Provider-boundary (Kind: CRT; Name: crt_setSBCS; Origin: Microsoft VC5-era CRT helper resetting multibyte state to SBCS; File: external; Target: pending; Group: messages.text)

- 0x10001ef0:
  - [✅] Reconstructed (Name: crt_setSBUpLow)
  - [✅] Provider-boundary (Kind: CRT; Name: crt_setSBUpLow; Origin: Microsoft VC5-era CRT helper rebuilding single-byte upper/lower case maps for multibyte codepage state; File: external; Target: pending; Group: messages.text)

- 0x100020d0:
  - [✅] Reconstructed (Name: __initmbctable)
  - [✅] Provider-boundary (Kind: CRT; Name: __initmbctable; Origin: Microsoft VC5-era CRT initialization helper that initializes the multibyte table with _MB_CP_ANSI; File: external; Target: pending; Group: messages.text)

- 0x100020e0:
  - [✅] Reconstructed (Name: __crtGetEnvironmentStringsA)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: __crtGetEnvironmentStringsA; Origin: VC5 CRT environment-string helper for __CRT_INIT/_aenvptr; converts/copies Kernel32 environment strings with cached W/A mode; File: external; Target: pending; Group: messages.text)

- 0x10002240:
  - [✅] Reconstructed (Name: __heap_init)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: __heap_init; Origin: VC5 CRT heap initialization helper for __CRT_INIT; creates _crtheap with HeapCreate and initializes CRT small-block heap state; File: external; Target: pending; Group: messages.text)

- 0x10002280:
  - [✅] Reconstructed (Name: __heap_term)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: __heap_term; Origin: VC5 CRT heap termination helper for __CRT_INIT; releases small-block heap regions and destroys _crtheap; File: external; Target: pending; Group: messages.text)

- 0x100022c0:
  - [✅] Reconstructed (Name: _FF_MSGBANNER)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: _FF_MSGBANNER; Origin: LIBCMT runtime-message banner helper; calls _NMSG_WRITE(0xfc), optional CRT banner callback, and _NMSG_WRITE(0xff); File: external; Target: pending; Group: messages.text)

- 0x10002300:
  - [✅] Reconstructed (Name: _NMSG_WRITE)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: _NMSG_WRITE; Origin: LIBCMT runtime-message output helper; scans VC CRT runtime error table and emits console/message-box runtime-error text; File: external; Target: pending; Group: messages.text)

- 0x100024e0:
  - [✅] Reconstructed (Name: __mtinitlocks)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: __mtinitlocks; Origin: VC5 CRT multithread lock initialization helper; initializes static __locktable critical-section slots; File: external; Target: pending; Group: messages.text)

- 0x10002510:
  - [✅] Reconstructed (Name: __mtdeletelocks)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: __mtdeletelocks; Origin: VC5 CRT lock-table teardown helper; File: external; Target: pending; Group: messages.text)

- 0x10002590:
  - [✅] Reconstructed (Name: __lock)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: __lock; Origin: VC CRT lock helper using __locktable and Win32 CRITICAL_SECTION imports; File: external; Target: pending; Group: messages.text)

- 0x10002630:
  - [✅] Reconstructed (Name: __lock_file)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: __lock_file; Origin: VC5 CRT _lock_file static runtime helper; File: external; Target: pending; Group: messages.text)

- 0x10002670:
  - [✅] Reconstructed (Name: __lock_file2)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: __lock_file2; Origin: statically linked VC5 CRT file-lock helper; BN assembly locks stream_index+0x1c through __lock for stream_index<20 and CRT_FILEX.lock at +0x20 through EnterCriticalSection otherwise; File: external; Target: pending; Group: messages.text)

- 0x100026a0:
  - [✅] Reconstructed (Name: __unlock_file)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: __unlock_file; Origin: VC5 CRT _unlock_file static runtime helper; paired with __lock_file, unlocks __iob streams through __unlock and non-static stream critical sections through LeaveCriticalSection; File: external; Target: pending; Group: messages.text)

- 0x100026e0:
  - [✅] Reconstructed (Name: __unlock_file2)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: __unlock_file2; Origin: statically linked VC5 CRT file-lock helper; BN assembly unlocks stream_index+0x1c through __unlock for stream_index<20 and CRT_FILEX.lock at +0x20 through LeaveCriticalSection otherwise; paired with provider __lock_file2 at 0x10002670; File: external; Target: pending; Group: messages.text)

- 0x10002710:
  - [✅] Reconstructed (Name: _calloc_crt)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _calloc_crt; Origin: VC5 CRT calloc heap helper using CRT small-block heap, lock table, HeapAlloc zeroed fallback, and new-handler retry; File: external; Target: pending; Group: messages.text)

- 0x100027c0:
  - [✅] Reconstructed (Name: _free)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: _free; Origin: VC5 CRT heap free routine; File: external; Target: pending; Group: messages.text)

- 0x10002830:
  - [✅] Reconstructed (Name: _malloc_crt)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: _malloc_crt; Origin: static CRT heap helper: forwards size and g_CRT_newmode to _nh_malloc; File: external; Target: pending; Group: messages.text)

- 0x10002850:
  - [✅] Reconstructed (Name: _nh_malloc)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: _nh_malloc; Origin: static CRT heap helper: normalizes zero allocation, calls heap allocator, retries through _callnewh when nhFlag is nonzero; File: external; Target: pending; Group: messages.text)

- 0x100028a0:
  - [✅] Reconstructed (Name: _heap_alloc)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: _heap_alloc; Origin: VC5 CRT heap allocator / small-block heap runtime; File: external; Target: pending; Group: messages.text)

- 0x10002900:
  - [✅] Reconstructed (Name: __crtLCMapStringA)
  - [✅] Provider-boundary (Kind: CRT; Name: __crtLCMapStringA; Origin: VC5/MFC-era CRT locale wrapper around LCMapStringA/W with W/A probe, conversion, and mode cache; File: external; Target: pending; Group: messages.text)

- 0x10002b30:
  - [✅] Reconstructed (Name: _strncnt)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: _strncnt; Origin: statically linked CRT bounded string-count helper used only by __crtLCMapStringA; File: external; Target: pending; Group: messages.text)

- 0x10002b60:
  - [✅] Reconstructed (Name: __crtGetStringTypeA)
  - [✅] Provider-boundary (Kind: CRT; Name: __crtGetStringTypeA; Origin: VC5/MFC-era CRT locale wrapper around GetStringTypeA/W with W/A probe, conversion, locale fallback, and mode cache; File: external; Target: pending; Group: messages.text)

- 0x10002ca0:
  - [✅] Reconstructed (Name: __sbh_alloc_new_region)
  - [✅] Provider-boundary (Kind: VC5 CRT small-block heap; Name: __sbh_alloc_new_region; Origin: VC CRT small-block heap provider routine; allocates/reuses __sbh_region, reserves 0x400000 bytes, commits first 0x10000, initializes page descriptors, and links region state; File: external; Target: pending; Group: messages.text)

- 0x10002e10:
  - [✅] Reconstructed (Name: __sbh_free_region)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: __sbh_free_region; Origin: Microsoft VC CRT small-block heap runtime helper releasing an __sbh_region via VirtualFree(MEM_RELEASE), maintaining __sbh_pHeaderScan/list links, and freeing non-static region headers through HeapFree(_crtheap).; File: external; Target: pending; Group: messages.text)

- 0x10002e70:
  - [✅] Reconstructed (Name: __sbh_decommit_pages)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: __sbh_decommit_pages; Origin: VC5 CRT small-block heap (__sbh) runtime helper statically linked into messages.dll; decommits fully-free SBH pages with VirtualFree MEM_DECOMMIT, adjusts the decommittable-page counter, and calls __sbh_free_region for fully uncommitted regions.; File: external; Target: pending; Group: messages.text)

- 0x10002f40:
  - [✅] Reconstructed (Name: __sbh_find_block)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: __sbh_find_block; Origin: VC5 CRT small-block heap runtime helper linked into messages.dll; called only by _free, walks __sbh_static_region/__sbh_region, rejects invalid small-block pointers, writes region/page-base outputs, and returns a page allocation-map byte pointer or NULL.; File: external; Target: pending; Group: messages.text)

- 0x10002fa0:
  - [✅] Reconstructed (Name: __sbh_free_block)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: __sbh_free_block; Origin: VC5 CRT small-block heap implementation used by _free; consumes __sbh_find_block outputs, updates __sbh_region page free-byte metadata, increments the shared decommittable-page counter, and calls __sbh_decommit_pages at threshold.; File: external; Target: pending; Group: messages.text)

- 0x10003000:
  - [✅] Reconstructed (Name: __sbh_alloc_block)
  - [✅] Provider-boundary (Kind: VC5 CRT small-block heap; Name: __sbh_alloc_block; Origin: VC5 CRT runtime small-block heap helper; callers _heap_alloc/_calloc_crt pass rounded_size >> 4 under __sbh_threshold; uses __sbh_pHeaderScan, __sbh_static_region, __sbh_alloc_new_region, VirtualAlloc; File: external; Target: pending; Group: messages.text)

- 0x10003240:
  - [✅] Reconstructed (Name: __sbh_alloc_block_from_page)
  - [✅] Provider-boundary (Kind: VC5 CRT small-block heap; Name: __sbh_alloc_block_from_page; Origin: VC5 CRT runtime small-block heap page allocation helper; only called by __sbh_alloc_block at 0x10003049 and 0x10003084; updates one committed SBH page free cursor/free-byte count; File: external; Target: pending; Group: messages.text)

- 0x100033c0:
  - [✅] Reconstructed (Name: __crtMessageBoxA)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: __crtMessageBoxA; Origin: LIBCMT CRTMBOX.C MessageBoxA wrapper; dynamically loads user32.dll, resolves MessageBoxA/GetActiveWindow/GetLastActivePopup, chooses active popup parent, and calls MessageBoxA.; File: external; Target: pending; Group: messages.text)

- 0x10003450:
  - [✅] Reconstructed (Name: strncpy)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: strncpy; Origin: statically linked Microsoft Visual C++ 5 CRT string routine; copies at most count bytes from src to dest, pads remaining bytes with NUL after source NUL, and returns dest.; File: external; Target: pending; Group: messages.text)

- 0x10003550:
  - [✅] Reconstructed (Name: __initstdio)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: __initstdio; Origin: static VC5 CRT stdio initializer from _file.c; registered in __xi_a and invoked by _cinit/_initterm; allocates __piob from _nstream, seeds __iob pointers, and marks invalid standard lowio handles.; File: external; Target: pending; Group: messages.text)

- 0x10003610:
  - [✅] Reconstructed (Name: __ioexit)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: __ioexit; Origin: VC5 CRT internal _ioexit stream-exit pre-terminator: stored in __xp_a at 0x10006018, invoked by doexit through _initterm over __xp_a, calls CRT stream flush helper, tests __exitflag at 0x1000da14, and conditionally tail-jumps to CRT stream close/termination helper 0x10003990.; File: external; Target: pending; Group: messages.text)

- 0x10003630:
  - [✅] Reconstructed (Name: _callnewh)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: _callnewh; Origin: static VC5 CRT heap/new-handler helper used by _calloc_crt and _nh_malloc after allocation failure; reads the CRT new-handler pointer at 0x1000ddf8, calls it with the requested allocation size, and returns 1 only when a handler exists and returns nonzero.; File: external; Target: pending; Group: messages.text)

- 0x10003650:
  - [✅] Reconstructed (Name: _memmove)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: _memmove; Origin: static VC5 CRT memmove provider routine: cdecl void *(dest, src, count), returns dest, handles overlap with forward/backward aligned rep-movsd and byte-tail dispatch; adjacent to accepted CRT provider helpers __ioexit and _callnewh; File: external; Target: pending; Group: messages.text)

- 0x100038eb:
  - [✅] Reconstructed (Name: sub_100038eb)
  - [✅] Provider-boundary (Kind: VC5 CRT overlap artifact; Name: sub_100038eb; Origin: spurious overlapping BN analysis artifact inside VC5 CRT _memmove backward-copy code; byte 3 of instruction at 0x100038e8; no xrefs; valid table flow reaches 0x100038e4/0x10003927 internal _memmove blocks; File: external; Target: pending; Group: messages.text)

- 0x10003990:
  - [✅] Reconstructed (Name: __fcloseall)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: __fcloseall; Origin: static VC5 CRT _fcloseall stream cleanup routine reached from __ioexit; locks CRT stream state, walks stream table from index 3, closes open streams, deletes critical sections/frees dynamic stream records, and returns close count; File: external; Target: pending; Group: messages.text)

- 0x10003a30:
  - [✅] Reconstructed (Name: _fflush_lk)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: _fflush_lk; Origin: VC5 CRT stdio locked fflush helper: takes CRT_iobuf*, calls internal stream flush, commits stream->_file when _IOCOMMIT is set, and returns 0/EOF; File: external; Target: pending; Group: messages.text)

- 0x10003a70:
  - [✅] Reconstructed (Name: _flush)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: _flush; Origin: VC5 CRT internal stdio stream flush helper: flushes pending bytes from CRT_iobuf through CRT write path, resets _cnt/_ptr, records write errors in _flag, and returns 0/EOF; File: external; Target: pending; Group: messages.text)

- 0x10003ae0:
  - [✅] Reconstructed (Name: __flushall)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: __flushall; Origin: VC5 CRT _flushall cdecl wrapper over internal all-stream flush sweep; File: external; Target: pending; Group: messages.text)

- 0x10003af0:
  - [✅] Reconstructed (Name: _flsall)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: _flsall; Origin: VC5 CRT internal flsall all-stream sweep called by __flushall; locks CRT stream-table lock 2, walks __piob[0.._nstream), locks active FILE streams, calls _fflush_lk, unlocks streams, and returns flushed stream count for _flushall mode or EOF/error accumulator for fflush(NULL)-style mode; File: external; Target: pending; Group: messages.text)

- 0x10003bb0:
  - [✅] Reconstructed (Name: fclose)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: fclose; Origin: statically linked VC5 CRT stdio fclose routine; File: external; Target: pending; Group: messages.text)

- 0x10003bf0:
  - [✅] Reconstructed (Name: _fclose_lk)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: _fclose_lk; Origin: statically linked VC5 CRT lock-free fclose helper; File: external; Target: pending; Group: messages.text)

- 0x10003c60:
  - [✅] Reconstructed (Name: _commit)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: _commit; Origin: VC5 CRT file descriptor commit runtime; File: external; Target: pending; Group: messages.text)

- 0x10003d00:
  - [✅] Reconstructed (Name: _write)
  - [✅] Provider-boundary (Kind: VC5 CRT; Name: _write; Origin: VC5 CRT lowio _write wrapper; File: external; Target: pending; Group: messages.text)

- 0x10003d80:
  - [✅] Reconstructed (Name: _write_lk)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _write_lk; Origin: VC5SP3 CRT lowio WRITE.C _write_lk; File: external; Target: pending; Group: messages.text)

- 0x10003f90:
  - [✅] Reconstructed (Name: _close)
  - [✅] Provider-boundary (Kind: VC5 CRT lowio; Name: _close; Origin: VC5SP3 CRT/SRC/CLOSE.C _MT _close; File: external; Target: pending; Group: messages.text)

- 0x10004000:
  - [✅] Reconstructed (Name: _close_lk)
  - [✅] Provider-boundary (Kind: VC5 CRT lowio; Name: _close_lk; Origin: VC5SP3 CRT/SRC/CLOSE.C _MT _close_lk provider; File: external; Target: pending; Group: messages.text)

- 0x10004090:
  - [✅] Reconstructed (Name: _freebuf)
  - [✅] Provider-boundary (Kind: VC5SP3 CRT stdio; Name: _freebuf; Origin: VC5SP3 CRT/SRC/_FREEBUF.C _freebuf; File: external; Target: pending; Group: messages.text)

- 0x100040d0:
  - [✅] Reconstructed (Name: _free_osfhnd)
  - [✅] Provider-boundary (Kind: VC5SP3 CRT lowio; Name: _free_osfhnd; Origin: VC5SP3 CRT/SRC/OSFINFO.C _free_osfhnd; File: external; Target: pending; Group: messages.text)

- 0x10004170:
  - [✅] Reconstructed (Name: _get_osfhandle)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _get_osfhandle; Origin: VC5SP3 CRT/SRC/OSFINFO.C _get_osfhandle; File: external; Target: pending; Group: messages.text)

- 0x100041c0:
  - [✅] Reconstructed (Name: _lock_fhandle)
  - [✅] Provider-boundary (Kind: VC5SP3 CRT lowio; Name: _lock_fhandle; Origin: VC5SP3 CRT/SRC/OSFINFO.C _MT _lock_fhandle; File: external; Target: pending; Group: messages.text)

- 0x10004230:
  - [✅] Reconstructed (Name: _unlock_fhandle)
  - [✅] Provider-boundary (Kind: VC5SP3 CRT lowio; Name: _unlock_fhandle; Origin: VC5SP3 CRT/SRC/OSFINFO.C _MT _unlock_fhandle; File: external; Target: pending; Group: messages.text)

- 0x10004260:
  - [✅] Reconstructed (Name: _dosmaperr)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _dosmaperr; Origin: VC5SP3 CRT/SRC/DOSMAP.C _dosmaperr; File: external; Target: pending; Group: messages.text)

- 0x100042e0:
  - [✅] Reconstructed (Name: _errno)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _errno; Origin: VC5 CRT per-thread errno accessor returning &_getptd()->terrno; File: external; Target: pending; Group: messages.text)

- 0x100042f0:
  - [✅] Reconstructed (Name: __doserrno)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: __doserrno; Origin: VC5 CRT per-thread DOS errno accessor returning &_getptd()->doserrno; File: external; Target: pending; Group: messages.text)

- 0x10004300:
  - [✅] Reconstructed (Name: _lseek_lk)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _lseek_lk; Origin: VC5SP3 CRT/SRC/LSEEK.C _lseek_lk; File: external; Target: pending; Group: messages.text)

## G003. messages.dll .data variable inventory

- Group ID: messages.data
- Kind: data-inventory
- Primary source: messages.bndb
- Validation: BN .data inventory and owner-led data-symbol verification
- Depends on: none

- 0x1000600c:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: void*)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime; Name: __xi_a[1]; Origin: VC5 CRT initializer table element: __xi_a entry consumed by _cinit/_initterm; points to CRT initializer 0x10003550; File: external; Target: pending; Group: messages.data)

- 0x10006010:
  - [✅] Reconstructed (Name: __xi_z)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __xi_z; Origin: VC5 CRT C initializer table end sentinel; _cinit pushes 0x10006008/0x10006010 to _initterm; CRT0DAT.C declares __xi_a/__xi_z.; File: external; Target: pending; Group: messages.data)

- 0x10006014:
  - [✅] Reconstructed (Name: __xp_a)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __xp_a; Origin: VC5 CRT C pre-terminator table start; doexit pushes 0x10006014/0x1000601c to _initterm; CRT0DAT.C declares __xp_a/__xp_z.; File: external; Target: pending; Group: messages.data)

- 0x10006018:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: void*)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __xp_a[1]; Origin: VC5 CRT pre-terminator table element: 0x10006018 is __xp_a+4, contains 0x10003610 (__ioexit), is consumed by doexit through _initterm(&__xp_a, &__xp_z); CRT0DAT.C defines __xp_a/__xp_z as C pre-terminators and _FILE.C/FFLUSH.C contribute stdio termination entries through .CRT$XPX.; File: external; Target: pending; Group: messages.data)

- 0x1000601c:
  - [✅] Reconstructed (Name: __xp_z)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __xp_z; Origin: VC5 CRT C pre-terminator table end sentinel; doexit pushes 0x10006014/0x1000601c to _initterm; CRT0DAT.C declares __xp_a/__xp_z.; File: external; Target: pending; Group: messages.data)

- 0x10006020:
  - [✅] Reconstructed (Name: __xt_a)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __xt_a; Origin: VC5 CRT C terminator table start sentinel; doexit pushes 0x10006020/0x10006024 to _initterm; CRT0DAT.C declares __xt_a/__xt_z.; File: external; Target: pending; Group: messages.data)

- 0x10006024:
  - [✅] Reconstructed (Name: __xt_z)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __xt_z; Origin: VC5 CRT C terminator table end sentinel; doexit pushes 0x10006020/0x10006024 to _initterm; CRT0DAT.C declares __xt_a/__xt_z.; File: external; Target: pending; Group: messages.data)

- 0x10006030:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: char (*)[0x9])
  - [✅] Source owner (Kind: global-data; Parent: messages.lookup_subsystem; State: implemented)
  - [✅] Reimplemented [S]
    - Name: g_MessagesLookupRows;
    - File: src/Messages/messages_lookup.inc;
    - Target: messages_lookup_data;
    - Group: messages.data;
    - Model: source-faithful;
    - Blocker: none

- 0x1000b2ac:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: void*)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __aexit_rtn; Origin: VC5 CRT crt0dat-style PVFI exit callback initialized to _exit; consumed by __amsg_exit runtime-message fatal-exit path; File: external; Target: pending; Group: messages.data)

- 0x1000b3e0:
  - [✅] Reconstructed (Name: g_CRT_runtimeErrorMessages)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: g_CRT_runtimeErrorMessages; Origin: LIBCMT/crt0msg runtime error-message table consumed by _NMSG_WRITE; BN type struct CRT_RuntimeErrorMessage[0x12], .data range 0x1000b3e0..0x1000b46f; File: external; Target: pending; Group: messages.data)

- 0x1000b2b0:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __tlsindex; Origin: VC5 CRT multithread TLS index global initialized to 0xffffffff and used by CRT __mtinit/__mtterm/_getptd/_freeptd; File: external; Target: pending; Group: messages.data)

- 0x1000b2e0:
  - [✅] Reconstructed (Name: g_crtMbctypeFlagBits)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: g_crtMbctypeFlagBits; Origin: VC5 CRT multibyte classification flag table used by _setmbcp to mark _mbctype ranges; BN .data uint8_t[4] at 0x1000b2e0..0x1000b2e4, bytes 01 02 04 08, xref _setmbcp 0x10001d8b; File: external; Target: pending; Group: messages.data)

- 0x1000b2e8:
  - [✅] Reconstructed (Name: g_crtMBCPInfoTable)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: g_crtMBCPInfoTable; Origin: VC5 CRT multibyte codepage info table used by _setmbcp; BN .data CRT_MBCPInfo[5] at 0x1000b2e8..0x1000b3d8 with code pages 932, 936, 949, 950, 1361; File: external; Target: pending; Group: messages.data)

- 0x1000b470:
  - [✅] Reconstructed (Name: __locktable)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __locktable; Origin: VC5 CRT lock table: CRITICAL_SECTION*[0x30] consumed by __mtinitlocks, __mtdeletelocks, __lock, __unlock, and _NMSG_WRITE; range 0x1000b470..0x1000b52f; File: external; Target: pending; Group: messages.data)

- 0x1000b530:
  - [✅] Reconstructed (Name: _XcptActTab)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: _XcptActTab; Origin: VC5 CRT exception-action table: struct _XCPT_ACTION[0xa] consumed by _initptd/_freeptd; range 0x1000b530..0x1000b5a7; File: external; Target: pending; Group: messages.data)

- 0x1000b5b8:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: void*)
  - [✅] Provider-boundary (Kind: VC5 CRT small-block heap data; Name: __sbh_static_region; Origin: VC CRT small-block heap static region/header object used by __heap_term, __sbh_alloc_new_region, and __sbh_alloc_block; File: external; Target: pending; Group: messages.data)

- 0x1000d5d8:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: void*)
  - [✅] Provider-boundary (Kind: VC5 CRT small-block heap data; Name: __sbh_pHeaderScan; Origin: VC CRT small-block heap region scan pointer used by __sbh_alloc_block and initialized to __sbh_static_region; File: external; Target: pending; Group: messages.data)

- 0x1000d5dc:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT small-block heap data; Name: __sbh_threshold; Origin: VC5 CRT small-block heap threshold global used by _heap_alloc/_calloc_crt to route small allocations to __sbh_alloc_block; BN type uint32_t and bytes e0 01 00 00.; File: external; Target: pending; Group: messages.data)

- 0x1000d5e0:
  - [✅] Reconstructed (Kind: data; Name: __iob; Section: .data; Size: 640; Type: struct CRT_iobuf[0x14])
  - [✅] Provider-boundary (Kind: VC5 CRT stdio data; Name: __iob; Origin: VC5 CRT stdio FILE/iobuf table used by CRT stdio initialization and file lock helpers.; File: external; Target: pending; Group: messages.data)

- 0x1000d860:
  - [✅] Reconstructed (Name: g_CRT_dosErrorMap)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime data; Name: g_CRT_dosErrorMap; Origin: VC5SP3 CRT/SRC/DOSMAP.C _dosmaperr static errtable, 45 entries of {dos_error, errno}, spanning 0x1000d860..0x1000d9c8 exclusive; File: external; Target: pending; Group: messages.data)

- 0x1000d9c8:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __proc_attached; Origin: LIBCMT:dllcrt0.obj CRT process-attach count used by __CRT_INIT and __DllMainCRTStartup; _dosmaperr use of 0x1000d9c8 is only the one-past-end sentinel for the DOS errno table; File: external; Target: pending; Group: messages.data)

- 0x1000d9cc:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: _aenvptr; Origin: VC5 CRT environment pointer used by __CRT_INIT/_setenvp; File: external; Target: pending; Group: messages.data)

- 0x1000d9d4:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __error_mode; Origin: VC5 CRT error reporting mode global; File: external; Target: pending; Group: messages.data)

- 0x1000d9d8:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __app_type; Origin: VC5 CRT app type global; File: external; Target: pending; Group: messages.data)

- 0x1000d9e0:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: _osver; Origin: VC5 CRT version global initialized from GetVersion in __CRT_INIT; File: external; Target: pending; Group: messages.data)

- 0x1000d9e4:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: _winver; Origin: VC5 CRT version global initialized from GetVersion in __CRT_INIT; File: external; Target: pending; Group: messages.data)

- 0x1000d9e8:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: _winmajor; Origin: VC5 CRT version global initialized from GetVersion in __CRT_INIT; File: external; Target: pending; Group: messages.data)

- 0x1000d9ec:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: _winminor; Origin: VC5 CRT version global initialized from GetVersion in __CRT_INIT; File: external; Target: pending; Group: messages.data)

- 0x1000d9f0:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __argc; Origin: VC5 CRT argv global initialized by __setargv; File: external; Target: pending; Group: messages.data)

- 0x1000d9f4:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __argv; Origin: VC5 CRT argv global initialized by __setargv; File: external; Target: pending; Group: messages.data)

- 0x1000d9fc:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __initenv; Origin: VC5 CRT initial environment pointer global initialized by _setenvp; File: external; Target: pending; Group: messages.data)

- 0x1000da0c:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: _pgmptr; Origin: VC5 CRT program path pointer global initialized by __setargv; File: external; Target: pending; Group: messages.data)

- 0x1000da14:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 1; Type: char)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __exitflag; Origin: VC5 CRT exit flag used by __ioexit/doexit; File: external; Target: pending; Group: messages.data)

- 0x1000da18:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: _C_Exit_Done; Origin: VC5 CRT exit completion flag used by __CRT_INIT/doexit; File: external; Target: pending; Group: messages.data)

- 0x1000da1c:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: _C_Termination_Done; Origin: VC5 CRT termination completion flag used by doexit; File: external; Target: pending; Group: messages.data)

- 0x1000db28:
  - [✅] Reconstructed (Name: _mbctype)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: _mbctype; Origin: messages.bndb .data uint8_t[0x101] at 0x1000db28; VC5 CRT multibyte classification table zero-initialized and rebuilt by _setmbcp/crt_setSBCS; no authored xrefs; File: external; Target: pending; Group: messages.data)

- 0x1000dc30:
  - [✅] Reconstructed (Name: _mbcasemap)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: _mbcasemap; Origin: messages.bndb .data uint8_t[0x100] at 0x1000dc30; VC5 CRT multibyte case-map table zero-initialized and rebuilt by _setmbcp/crt_setSBCS; no authored xrefs; File: external; Target: pending; Group: messages.data)

- 0x1000dd30:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __mbcodepage; Origin: VC5 CRT multibyte runtime codepage global used by _setmbcp/crt_setSBCS; File: external; Target: pending; Group: messages.data)

- 0x1000dd34:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __mblcid; Origin: VC5 CRT multibyte runtime LCID global used by _setmbcp/crt_setSBCS; File: external; Target: pending; Group: messages.data)

- 0x1000dd38:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __mbulinfo; Origin: VC5 CRT multibyte runtime update-info array uint32_t[3] used by _setmbcp/crt_setSBCS; File: external; Target: pending; Group: messages.data)

- 0x1000dd44:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __mbcodepage_set; Origin: VC5 CRT multibyte runtime codepage-set flag used by _setmbcp/crt_setSBCS; File: external; Target: pending; Group: messages.data)

- 0x1000dd4c:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: g_crtEnvironmentStringsMode; Origin: messages.bndb .data int32_t at 0x1000dd4c; static/cache mode used only by accepted provider helper __crtGetEnvironmentStringsA to choose W/A Kernel32 environment-string path; File: external; Target: pending; Group: messages.data)

- 0x1000dd50:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: g_CRT_bannerCallback; Origin: messages.bndb .data void (*)() at 0x1000dd50; zero-initialized optional runtime-message banner callback slot used only by accepted provider helper _FF_MSGBANNER; File: external; Target: pending; Group: messages.data)

- 0x1000dd58:
  - [✅] Reconstructed (Name: __crt_static_critical_sections)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __crt_static_critical_sections; Origin: messages.bndb .data CRITICAL_SECTION[4] at 0x1000dd58; static CRT lock critical-section storage referenced through __locktable and used by CRT lock/unlock/runtime-message paths; File: external; Target: pending; Group: messages.data)

- 0x1000ddb8:
  - [✅] Reconstructed (Name: __lc_handle)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: __lc_handle; Origin: messages.bndb .data 0x1000ddb8..0x1000ddd0; LCID[6] locale-handle array, with LC_CTYPE element at +0x8 used by __crtGetStringTypeA; File: external; Target: pending; Group: messages.data)

- 0x1000ddd0:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: __lc_codepage; Origin: messages.bndb .data; CRT locale/codepage global read by crt_getSystemCP, __crtLCMapStringA, and __crtGetStringTypeA; File: external; Target: pending; Group: messages.data)

- 0x1000ddd8:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: g_crtLCMapStringA_mode; Origin: messages.bndb .data; CRT LCMapStringA/W mode cache used only by __crtLCMapStringA; File: external; Target: pending; Group: messages.data)

- 0x1000dde0:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: g_crtGetStringTypeA_mode; Origin: messages.bndb .data; CRT GetStringTypeA/W mode cache used only by __crtGetStringTypeA; File: external; Target: pending; Group: messages.data)

- 0x1000dde4:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: CRT SBH decommittable-page counter; Origin: messages.bndb .data; VC5 CRT small-block heap counter used by __sbh_free_block and __sbh_decommit_pages; File: external; Target: pending; Group: messages.data)

- 0x1000dde8:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: __crtMessageBoxA cached MessageBoxA pointer; Origin: messages.bndb .data; CRTMBOX-style lazy USER32 MessageBoxA resolver; File: external; Target: pending; Group: messages.data)

- 0x1000ddec:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: __crtMessageBoxA cached GetActiveWindow pointer; Origin: messages.bndb .data; CRTMBOX-style lazy USER32 GetActiveWindow resolver; File: external; Target: pending; Group: messages.data)

- 0x1000ddf0:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: __crtMessageBoxA cached GetLastActivePopup pointer; Origin: messages.bndb .data; CRTMBOX-style lazy USER32 GetLastActivePopup resolver; File: external; Target: pending; Group: messages.data)

- 0x1000ddf8:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _pnhHeap; Origin: messages.bndb .data; CRT new-handler pointer consumed by _callnewh; File: external; Target: pending; Group: messages.data)

- 0x1000ddfc:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _newmode; Origin: messages.bndb .data; CRT allocation retry/new-handler mode consumed by _malloc_crt and _calloc_crt; File: external; Target: pending; Group: messages.data)

- 0x1000de00:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: __piob; Origin: VC5 CRT stdio initializer (_file.c / LIBCMT); BN __initstdio allocates and seeds FILE* table; File: external; Target: pending; Group: messages.data)

- 0x1000ee20:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _nstream; Origin: VC5 CRT stdio initializer (_file.c / LIBCMT); BN __initstdio clamps/defaults stream count; File: external; Target: pending; Group: messages.data)

- 0x1000ee24:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: _crtheap; Origin: VC5 CRT process heap handle shared by __heap_init, __heap_term, allocation/free helpers, and small-block heap routines; File: external; Target: pending; Group: messages.data)

- 0x1000ee28:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __ismbcodepage; Origin: VC5 CRT multibyte runtime state flag used by _setmbcp/crt_setSBCS; File: external; Target: pending; Group: messages.data)

- 0x1000ee40:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: __pioinfo; Origin: VC5 CRT lowio _ioinit/_ioterm provider data; BN type g_CRT_pioinfo struct CRT_ioinfo*[0x40]; File: external; Target: pending; Group: messages.data)

- 0x1000ef40:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _nhandle; Origin: VC5 CRT lowio _ioinit/_ioterm provider data; File: external; Target: pending; Group: messages.data)

- 0x1000ef44:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _onexitend; Origin: VC5 CRT exit/onexit provider data from crt0dat.c; File: external; Target: pending; Group: messages.data)

- 0x1000ef48:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _onexitbegin; Origin: VC5 CRT exit/onexit provider data from crt0dat.c; File: external; Target: pending; Group: messages.data)

- 0x1000ef4c:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _FPinit; Origin: VC5 CRT C initialization provider data from crt0dat.c; File: external; Target: pending; Group: messages.data)

- 0x1000ef50:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: _acmdln; Origin: VC5 CRT argv/startup provider data; File: external; Target: pending; Group: messages.data)

- 0x1000ef54:
  - [✅] Reconstructed (Kind: data; Name: (unnamed); Section: .data; Size: 4; Type: int32_t)
  - [✅] Provider-boundary (Kind: VC5 CRT static runtime; Name: CRT_raw_dllmain_callback; Origin: VC5 CRT DLL startup raw callback pointer in dllcrt0.obj; File: external; Target: pending; Group: messages.data)
- 0x10006000:
  - [✅] Reconstructed (Name: __xc_a)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __xc_a; Origin: VC5 CRT C++ initializer table start sentinel; _cinit pushes 0x10006000/0x10006004 to _initterm; CRT0DAT.C declares __xc_a/__xc_z.; File: external; Target: pending; Group: messages.data)

- 0x10006004:
  - [✅] Reconstructed (Name: __xc_z)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __xc_z; Origin: VC5 CRT C++ initializer table end sentinel; _cinit pushes 0x10006000/0x10006004 to _initterm; CRT0DAT.C declares __xc_a/__xc_z.; File: external; Target: pending; Group: messages.data)

- 0x10006008:
  - [✅] Reconstructed (Name: __xi_a)
  - [✅] Provider-boundary (Kind: VC5 CRT runtime data; Name: __xi_a; Origin: VC5 CRT C initializer table start; _cinit pushes 0x10006008/0x10006010 to _initterm; CRT0DAT.C declares __xi_a/__xi_z.; File: external; Target: pending; Group: messages.data)

