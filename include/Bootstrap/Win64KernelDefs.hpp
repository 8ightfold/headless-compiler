//===- Bootstrap/Win64KernelDefs.hpp --------------------------------===//
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
//  This file defines structures used to get the kernel32 base offset.
//  Doesn't define every member, and doesn't allow anything to be copied.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Features.hpp>
#include <Common/Fundamental.hpp>
#include <Common/DynAlloc.hpp>

// For more info:
// "Finding Kernel32 Base and Function Addresses in Shellcode"
// https://en.wikipedia.org/wiki/Win32_Thread_Information_Block
// https://www.geoffchappell.com/studies/windows/km/
// https://github.com/wine-mirror/wine/blob/master/include/winternl.h

// TODO: refactor list entry stuff

static_assert(HC_PLATFORM_WIN64, "Windows only.");

namespace hc::bootstrap {
  using Win64Addr       = void*;
  using Win64AddrRange  = common::PointerRange<void>;
  using Win64Handle     = hc::__void*;
  using Win64Bool       = bool;

  struct Win64UnicodeString {
    u16 size, size_max;
    wchar_t*  buffer;
  public:
    static Win64UnicodeString New(wchar_t* str);
    static Win64UnicodeString New(wchar_t* str, usize max);
    bool isEqual(const Win64UnicodeString& rhs) const;
  };

  struct Win64LDRDataTableEntry;

  struct Win64ListEntry {
    Win64ListEntry*   prev;
    Win64ListEntry*   next;
    HC_MARK_DELETED(Win64ListEntry);
  public:
    Win64LDRDataTableEntry* asLDRDataTableEntry() const;
    Win64LDRDataTableEntry* findDLL(const wchar_t* str) const;
    Win64LDRDataTableEntry* findDLL(const char* str) const;
    Win64ListEntry* getFirst() const;
    Win64ListEntry* getLast() const;
    bool isFirst() const;
  private:
    [[gnu::always_inline, gnu::const]]
    Win64ListEntry* asMutable() const {
      return const_cast<Win64ListEntry*>(this);
    }
  };

  struct Win64LDRDataTableEntry {
    Win64ListEntry      __links_in_load_order;
    Win64ListEntry      __links_in_mem_order;
    Win64ListEntry      __links_in_init_order;
    Win64Addr           dll_base;
    Win64Addr           entry_point;
    u32                 size_of_image;
    Win64UnicodeString  full_dll_name;
    Win64UnicodeString  base_dll_name;
    u32                 flags;
    i16                 load_count;
    i16                 TLS_index;
    // ...
    HC_MARK_DELETED(Win64LDRDataTableEntry);
  };

  //=== PEB Data ===//

  struct Win64PEBLDRData {
    u32               length;
    Win64Bool         is_initialized;
    Win64Handle       ss_handle;
    Win64ListEntry    __mlist_in_load_order;
    Win64ListEntry    __mlist_in_mem_order;
    Win64ListEntry    __mlist_in_init_order;
    Win64Addr         entry_in_progress;
    Win64Bool         is_shutdown_in_progress;
    Win64Handle       shutdown_thread_id;
    HC_MARK_DELETED(Win64PEBLDRData);
  };

  struct Win64PEB {
    Win64Bool         is_inherited_addr_space;
    Win64Bool         image_file_exec_options;
    Win64Bool         being_debugged;
    Win64Bool         using_large_pages : 1;
    Win64Bool         is_protected      : 1;
    Win64Bool         using_dyn_reloc   : 1;
    Win64Bool         __SPU32F          : 1;
    Win64Bool         is_packaged       : 1;
    Win64Bool         is_app_container  : 1;
    Win64Bool         is_protected_lite : 1;
    Win64Bool         is_lpath_aware    : 1;
    Win64Handle       mutant;
    Win64Handle       image_base_addr;
    Win64PEBLDRData*  LDR_data;
    // ...
    HC_MARK_DELETED(Win64PEB);
  public:
    Win64ListEntry* getLDRModulesInMemOrder();
  private:
    // Slightly broken (segfaults!!!)
    Win64ListEntry* getLDRModulesInInitOrder();
    Win64ListEntry* getLDRModulesInLoadOrder();
  };

  //=== TEB Data ===//

  struct Win64TIB {
    Win64Addr SEH_frame;
    Win64Addr stack_begin;
    Win64Addr stack_end;
    Win64Addr subsystem_TIB;
    Win64Addr fiber_info;
    Win64Addr arbitrary_data;
    Win64Addr TEB_addr;
    HC_MARK_DELETED(Win64TIB);
  };

  struct Win64TEB {
    Win64TIB    tib;
    Win64Addr   env_addr;
    Win64Handle process_id;
    Win64Handle thread_id;
    Win64Addr   rpc_handle;
    Win64Addr   TLS_array_addr;
    Win64Addr   PEB_addr;
    u32         last_error;
    u32         critical_section_count;
    Win64Addr   CSR_thread_addr;
    Win64Addr   win32_thread_info;
    u32         __user32_reserved[31];
    Win64Addr   fast_syscall_addr;
    u32         current_locale;
    u32         fp_status_reg;
    u32         __os_reserved[54];
    u32         exception_code;
    // ...
    HC_MARK_DELETED(Win64TEB);
  public:
    static Win64TEB* LoadTEBFromGS();
    Win64AddrRange getStackRange() const;
    uptr getProcessId() const;
    uptr getThreadId() const;
    Win64PEB* getPEB() const;
  };
} // namespace hc::bootstrap
