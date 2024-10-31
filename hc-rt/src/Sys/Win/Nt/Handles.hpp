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

#define _HC_NTHANDLE_GROUP(name) InGroup<name>

#define $DefHANDLEEx(name, groups...) \
$Handle(name##Handle, void*, Boolean, Equality, \
 $PP_mapCL(_HC_NTHANDLE_GROUP, ##groups)); \
static_assert(sizeof(name##Handle) == sizeof(void*))
/// Define and check validity of handle.
#define $DefHANDLE(name, groups...) \
$Handle(name##Handle, void*, Boolean, Equality, \
 $PP_mapL(_HC_NTHANDLE_GROUP, HANDLE, ##groups)); \
static_assert(sizeof(name##Handle) == sizeof(void*))

/// Extracts `__data` or returns `INVALID_HANDLE`.
#define $unwrap_handle(H) ({ \
  if __expect_false(!H) \
    return 0xC0000008; \
  H.__data; \
})

namespace hc::sys::win {

$HandleGroup(HANDLE);
$HandleGroup(CONSOLE_HANDLE);
$HandleGroup(FILE_HANDLE);
$HandleGroup(IO_HANDLE);
$HandleGroup(IPC_HANDLE);
$HandleGroup(SYNC_HANDLE);
$HandleGroup(TOKEN_HANDLE);
$HandleGroup(WAIT_HANDLE);

$DefHANDLE(AccessTok);
$DefHANDLE(Console,     CONSOLE_HANDLE, IO_HANDLE);
$DefHANDLE(ConsoleBuf,  IO_HANDLE);
$DefHANDLE(Device);
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

template <typename...GroupRestrictions>
struct [[gsl::Pointer]] SelectiveHandle {
  SelectiveHandle() = default;
  SelectiveHandle(nullptr_t) : SelectiveHandle() { }
  explicit SelectiveHandle(GenericHandle h) : __data(h.__data) { }

  template <__is_HANDLE_of<GroupRestrictions...> H>
  SelectiveHandle(H h) : __data(h.__data) { }

  template <__is_HANDLE_of<GroupRestrictions...> H>
  operator H() const { return H::New(__data); }

  void* get() const { return this->__data; }
  explicit operator bool() const {
    return __is_valid_HANDLE(__data);
  }

public:
  void* __data = nullptr;
};

using FileObjHandle = SelectiveHandle<CONSOLE_HANDLE, FILE_HANDLE>;
using IOHandle      = SelectiveHandle<IO_HANDLE>;
using IPCHandle     = SelectiveHandle<IPC_HANDLE>;
using SyncHandle    = SelectiveHandle<SYNC_HANDLE>;
using WaitHandle    = SelectiveHandle<SYNC_HANDLE, IPC_HANDLE>;

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
#undef $DefHANDLE
#undef $DefProxy
