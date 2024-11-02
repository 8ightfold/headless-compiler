//===- Sys/Win/Nt/Handles.hpp ---------------------------------------===//
//
// Copyright (C) 2024 Eightfold
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
//     limitations under the License.
//
//===----------------------------------------------------------------===//
//
//  This file defines type-safe handles for HANDLE.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Handle.hpp>
#include <Common/Casting.hpp>

// For more info:
// https://en.wikipedia.org/wiki/Object_Manager
// https://github.com/hfiref0x/WinObjEx64/blob/master/Source/WinObjEx64/objects.h#L27

#ifndef _HC_HANDLE_DECLARE
# error Oops! You broke compatibility!
#endif

#define _HC_NTHANDLE_GROUP(name) InGroup<name>

#define $DeclHANDLEHead(hname, groups...) \
struct hname : public \
  _HC_HANDLE_DECLARE(hname, void*, Boolean, Equality, \
  $PP_mapCL(_HC_NTHANDLE_GROUP, ##groups))

#define $DeclHANDLE(name, cls, groups...) \
$DeclHANDLEHead(name##Handle, ##groups) { \
  static constexpr WinObjectClass clazz = cls; \
}; static_assert(sizeof(name##Handle) == sizeof(void*))

#define $DefHANDLEEx(name, cls, groups...) \
  $DeclHANDLE(name, cls, ##groups)

/// Define and check validity of handle.
#define $DefHANDLE(name, groups...) \
  $DeclHANDLE(name, OB_##name, HANDLE, ##groups)

/// Extracts `__data` or returns `INVALID_HANDLE`.
#define $unwrap_handle(H) ({ \
  if __expect_false(!H) \
    return 0xC0000008; \
  H.__data; \
})

namespace hc::sys::win {

enum WinObjectClass : boot::NtULong {
  OB_Device                       = 0,
  OB_Driver                       = 1,
  OB_Section                      = 2,
  OB_Port                         = 3,
  OB_SymbolicLink                 = 4,
  OB_Key                          = 5,
  OB_Event                        = 6,
  OB_Job                          = 7,
  OB_Mutant                       = 8,
  OB_KeyedEvent                   = 9,
  OB_Type                         = 10,
  OB_Directory                    = 11,
  OB_Winstation                   = 12,
  OB_Callback                     = 13,
  OB_Semaphore                    = 14,
  OB_WaitablePort                 = 15,
  OB_Timer                        = 16,
  OB_Session                      = 17,
  OB_Controller                   = 18,
  OB_Profile                      = 19,
  OB_EventPair                    = 20,
  OB_Desktop                      = 21,
  OB_File                         = 22,
  OB_WMIGuid                      = 23,
  OB_DebugObject                  = 24,
  OB_IoCompletion                 = 25,
  OB_Process                      = 26,
  OB_Adapter                      = 27,
  OB_Token                        = 28,
  OB_ETWRegistration              = 29,
  OB_Thread                       = 30,
  OB_TmTx                         = 31,
  OB_TmTm                         = 32,
  OB_TmRm                         = 33,
  OB_TmEn                         = 34,
  OB_PcwObject                    = 35,
  OB_FltConnPort                  = 36,
  OB_FltComnPort                  = 37,
  OB_PowerRequest                 = 38,
  OB_ETWConsumer                  = 39,
  OB_TpWorkerFactory              = 40,
  OB_Composition                  = 41,
  OB_IRTimer                      = 42,
  OB_DxgkSharedResource           = 43,
  OB_DxgkSharedSwapChain          = 44,
  OB_DxgkSharedSyncObject         = 45,
  OB_DxgkCurrentDxgProcessObject  = 46,
  OB_DxgkCurrentDxgThreadObject   = 47,
  OB_DxgkDisplayManager           = 48,
  OB_DxgkDisplayMuxSwitch         = 49,
  OB_DxgkSharedBundle             = 50,
  OB_DxgkSharedProtectedSession   = 51,
  OB_DxgkComposition              = 52,
  OB_DxgkSharedKeyedMutex         = 53,
  OB_MemoryPartition              = 54,
  OB_RegistryTransaction          = 55,
  OB_DmaAdapter                   = 56,
  OB_DmaDomain                    = 57,
  OB_CoverageSampler              = 58, // NI
  OB_ActivationObject             = 59, // NI
  OB_ActivityReference            = 60, // NI
  OB_CoreMessaging                = 61, // NI
  OB_RawInputManager              = 62, // NI
  OB_WaitCompletionPacket         = 63, // NI
  OB_IoCompletionReserve          = 64, 
  OB_UserApcReserve               = 65, // NI
  OB_IoRing                       = 66, // NI
  OB_Terminal                     = 67, // NI
  OB_TerminalEventQueue           = 68, // NI
  OB_EnergyTracker                = 69, // NI

  OB_AccessTok                    = OB_Token,
  OB_Console                      = OB_File,
  OB_ConsoleBuf                   = OB_File,
  OB_FileMap                      = OB_File,
  OB_Mutex                        = OB_Mutant,
  OB_Symlink                      = OB_SymbolicLink,
  
  // TODO: Mailslot, Pipe
  OB_Unknown                      = 70,
  OB_Mailslot                     = OB_Unknown,
  OB_Pipe                         = OB_Unknown,
  OB_Max
};

//======================================================================//
// Handle Objects
//======================================================================//

$HandleGroup(HANDLE);
$HandleGroup(CONSOLE_HANDLE);
$HandleGroup(FILE_HANDLE);
$HandleGroup(IO_HANDLE);
$HandleGroup(IPC_HANDLE);
$HandleGroup(SYNC_HANDLE);
$HandleGroup(TOKEN_HANDLE);
$HandleGroup(WAIT_HANDLE);

$DefHANDLE(AccessTok,   TOKEN_HANDLE);
$DefHANDLE(Console,     CONSOLE_HANDLE, IO_HANDLE);
$DefHANDLE(ConsoleBuf,  IO_HANDLE);
$DefHANDLE(Device,      FILE_HANDLE, IPC_HANDLE);
$DefHANDLE(Directory,   FILE_HANDLE);
$DefHANDLE(Event);
$DefHANDLE(File,        FILE_HANDLE, IO_HANDLE);
$DefHANDLE(FileMap,     FILE_HANDLE);
$DefHANDLE(Job);
$DefHANDLE(Key);
$DefHANDLE(Mailslot);
$DefHANDLE(Mutex,       SYNC_HANDLE);
$DefHANDLE(Port);
$DefHANDLE(Pipe,        IPC_HANDLE, IO_HANDLE);
$DefHANDLE(Process,     IPC_HANDLE);
$DefHANDLE(Section);
$DefHANDLE(Semaphore,   SYNC_HANDLE);
$DefHANDLE(Symlink,     FILE_HANDLE);
$DefHANDLE(Thread,      IPC_HANDLE);
$DefHANDLE(Timer,       SYNC_HANDLE);
$DefHANDLE(Token,       TOKEN_HANDLE);

using AccessTok = AccessTokHandle;
using SymbolicLink = SymlinkHandle;

template <typename H>
concept __is_HANDLE = handle_in_group<H, HANDLE>;

template <typename H, typename...Groups>
concept __is_HANDLE_of = __is_HANDLE<H> && handle_in_group<H, Groups...>;

__always_inline constexpr bool
 __is_valid_HANDLE(const void* data) noexcept {
  // There are exceptions to this check (eg. Console, Process),
  // but for the most part it holds true.
  static constexpr uptr invalid = uptr(-1LL);
  return data && (data != ptr_cast<>(invalid));
}

struct [[gsl::Pointer]] GenericHandle {
  GenericHandle() = default;

  template <__is_HANDLE H> 
  __always_inline GenericHandle(H h) : __data(h.__data) { }

  template <__is_HANDLE H>
  explicit operator H() const { return H::New(__data); }

  void* get() const { return this->__data; }
  explicit operator bool() const {
    return __is_valid_HANDLE(__data);
  }

public:
  void* __data = nullptr;
};

template <typename...Groups>
struct [[gsl::Pointer]] SelectiveHandle {
  SelectiveHandle() = default;
  SelectiveHandle(nullptr_t) : SelectiveHandle() { }
  explicit SelectiveHandle(GenericHandle h) : __data(h.__data) { }

  template <__is_HANDLE_of<Groups...> H>
  SelectiveHandle(H h) : __data(h.__data) { }

  template <__is_HANDLE_of<Groups...> H>
  operator H() const { return H::New(__data); }

  void* get() const { return this->__data; }
  explicit operator bool() const {
    return __is_valid_HANDLE(__data);
  }

public:
  void* __data = nullptr;
};

template <typename...Groups> struct HandleRef {
  template <__is_HANDLE_of<Groups...> H>
  HandleRef(H& handle) :
   __idata(&handle.__data), __clazz(H::clazz) {
  }

  template <__is_HANDLE_of<Groups...> H>
  HandleRef& operator=(H handle) {
    if __likely_true(H::clazz == this->__clazz) {
      __hc_invariant(this->__idata);
      (*this->__idata) = handle.__data;
    }
    return *this;
  }

  template <__is_HANDLE_of<Groups...> H>
  operator H() const {
    if __likely_false(H::clazz != this->__clazz)
      return H::New(nullptr);
    __hc_invariant(this->__idata);
    return H::New(*__idata);
  }

  void*& get() const { return *this->__idata; }
  explicit operator bool() const {
    return __is_valid_HANDLE(*__idata);
  }

public:
  void** __idata; // Intrusive __data.
  WinObjectClass __clazz = OB_Unknown;
};

using FileObjHandle = SelectiveHandle<CONSOLE_HANDLE, FILE_HANDLE>;
using IOHandle      = SelectiveHandle<IO_HANDLE>;
using IPCHandle     = SelectiveHandle<IPC_HANDLE>;
using SyncHandle    = SelectiveHandle<SYNC_HANDLE>;
using WaitHandle    = SelectiveHandle<SYNC_HANDLE, IPC_HANDLE>;

using FileObjRef    = HandleRef<CONSOLE_HANDLE, FILE_HANDLE>;
using IORef         = HandleRef<IO_HANDLE>;
using IPCRef        = HandleRef<IPC_HANDLE>;
using SyncRef       = HandleRef<SYNC_HANDLE>;
using WaitRef       = HandleRef<SYNC_HANDLE, IPC_HANDLE>;

//======================================================================//
// Predefined Handles
//======================================================================//

#define $DefProxy(name, value, restrictions...) \
  __global HandleProxy<value, ##restrictions> name {}

template <auto Value, typename...GroupRestrictions>
struct HandleProxy {
  using SelfType = HandleProxy;
  using Selective = SelectiveHandle<GroupRestrictions...>;
  static constexpr uptr rawValue = static_cast<uptr>(Value);
public:
  static void* GetRaw() { return ptr_cast<>(rawValue); }

  template <__is_HANDLE_of<GroupRestrictions...> H>
  static H Get() { return H::New(SelfType::GetRaw()); }

  template <__is_HANDLE_of<GroupRestrictions...> H>
  operator H() const { return SelfType::Get<H>(); }

  inline Selective operator()() const  {
    Selective out {};
    out.__data = SelfType::GetRaw();
    return out;
  }
};

template <__is_HANDLE H, auto Value, typename...Groups>
requires __is_HANDLE_of<H, Groups...>
constexpr bool operator==(
 const H& lhs, const HandleProxy<Value, Groups...>& rhs) noexcept {
  return (lhs == static_cast<H>(rhs));
}

$DefProxy(InvalidHandle,    -1);
$DefProxy(CreateNewConsole, -2, CONSOLE_HANDLE, FILE_HANDLE);
$DefProxy(CreateNoWindow,   -3, CONSOLE_HANDLE, FILE_HANDLE);

$DefProxy(ProcessToken,         -4, TOKEN_HANDLE);
$DefProxy(ThreadToken,          -5, TOKEN_HANDLE);
$DefProxy(ThreadEffectiveToken, -6, TOKEN_HANDLE);

} // namespace hc::sys::win

#undef _HC_NTHANDLE_GROUP
#undef $DeclHANDLEHead
#undef $DeclHANDLE
#undef $DefHANDLEEx
#undef $DefHANDLE


#undef $DefProxy
