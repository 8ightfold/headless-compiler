//===- Sys/Windows/NtHandles.hpp ------------------------------------===//
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

#define _HC_NTHANDLE_GROUP(name) InGroup<name>
#define $DefHANDLE(name, groups...) \
 $Handle(name##Handle, const void*, Boolean, Equality, \
  $PP_mapL(_HC_NTHANDLE_GROUP, HANDLE, ##groups))

namespace hc::sys::win {
  $HandleGroup(HANDLE);
  $HandleGroup(FILE_HANDLE);
  $HandleGroup(IO_HANDLE);
  $HandleGroup(IPC_HANDLE);
  $HandleGroup(SYNC_HANDLE);

  $DefHANDLE(AccessTok);
  $DefHANDLE(Console,     IO_HANDLE);
  $DefHANDLE(ConsoleBuf,  IO_HANDLE);
  $DefHANDLE(Device);
  $DefHANDLE(Directory,   FILE_HANDLE);
  $DefHANDLE(Event);
  $DefHANDLE(File,        FILE_HANDLE, IO_HANDLE);
  $DefHANDLE(FileMap,     FILE_HANDLE);
  $DefHANDLE(Job);
  $DefHANDLE(Mailslot);
  $DefHANDLE(Mutex,       SYNC_HANDLE);
  $DefHANDLE(Pipe,        IPC_HANDLE, IO_HANDLE);
  $DefHANDLE(Process,     IPC_HANDLE);
  $DefHANDLE(Semaphore,   SYNC_HANDLE);
  $DefHANDLE(Thread,      IPC_HANDLE);

  template <typename H>
  concept __is_HANDLE = handle_in_group<H, HANDLE>;

  struct GenericHandle {
    GenericHandle() = default;
    template <__is_HANDLE H> 
    GenericHandle(H h) : __data(h.__data) { }

    template <typename H>
    requires __is_HANDLE<H>
    explicit operator H() const { return H::New(__data); }

    void* get() const { return this->__data; }
  public:
    void* __data = nullptr;
  };

  template <typename GroupRestriction>
  struct SelectiveHandle {
    SelectiveHandle() = default;
    explicit SelectiveHandle(GenericHandle h) : __data(h.__data) { }

    template <typename H>
    requires handle_in_group<H, GroupRestriction>
    SelectiveHandle(H h) : __data(h.__data) { }

    template <typename H>
    requires handle_in_group<H, GroupRestriction>
    explicit operator H() const { return H::New(__data); }

    void* get() const { return this->__data; }
  public:
    void* __data = nullptr;
  };

  using FileObjHandle = SelectiveHandle<FILE_HANDLE>;
  using IOHandle      = SelectiveHandle<IO_HANDLE>;
  using IPCHandle     = SelectiveHandle<IPC_HANDLE>;
  using SyncHandle    = SelectiveHandle<SYNC_HANDLE>;

} // namespace hc::sys::win

#undef _HC_NTHANDLE_GROUP
#undef $DefHANDLE
