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
#include <BinaryFormat/Consumer.hpp>

// For more info:
// "Finding Kernel32 Base and Function Addresses in Shellcode"
// https://en.wikipedia.org/wiki/Win32_Thread_Information_Block
// https://www.geoffchappell.com/studies/windows/km/
// https://github.com/wine-mirror/wine/blob/master/include/winternl.h

#define _MutLDRNode mutable ::hc::bootstrap::Win64ListEntryNode

static_assert(HC_PLATFORM_WIN64, "Windows only.");

namespace hc::bootstrap {
  using Win64Addr       = void*;
  using Win64AddrRange  = common::AddrRange;
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

  struct Win64ListEntryNode {
    Win64ListEntryNode*   __prev;
    Win64ListEntryNode*   __next;
    HC_MARK_DELETED(Win64ListEntryNode);
  public:
    static Win64ListEntryNode* GetBaseNode();
  };

  struct Win64LDRDataTableEntry {
    _MutLDRNode         __links_in_load_order;
    _MutLDRNode         __links_in_mem_order;
    _MutLDRNode         __links_in_init_order;
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
  public:
    Win64AddrRange getImageRange() const {
      common::_VoidPtrProxy begin { this->dll_base };
      auto end = begin + this->size_of_image;
      return { begin, end };
    }

    template <typename T = void>
    T* getRVA(u32 offset) const {
      __hc_invariant(offset < this->size_of_image);
      auto loc = common::_VoidPtrProxy(dll_base) + offset;
      return static_cast<T*>(loc);
    }

    Win64AddrRange getRangeFromRVA(u32 offset) const {
      return this->getImageRange().dropFront(offset);
    }

    binfmt::Consumer getConsumerFromRVA(u32 offset) const {
      return binfmt::Consumer::New(getRangeFromRVA(offset));
    }

    __always_inline Win64UnicodeString name() const {
      return this->base_dll_name;
    }
  };

  struct Win64ModuleType {
    static constexpr usize loadOrder  = 0U;
    static constexpr usize memOrder   = 1U;
    static constexpr usize initOrder  = 2U;
  };

  template <usize TableOffset>
  struct TWin64ListEntry : Win64ListEntryNode {
    using BaseType = Win64ListEntryNode;
    using SelfType = TWin64ListEntry<TableOffset>;
    using TblType  = Win64LDRDataTableEntry;
  public:
    [[gnu::const]] static const SelfType* GetListSentinel() __noexcept {
      // We assume the base node never changes.
      // If it does, something has gone horribly wrong...
      static const auto base_node = Win64ListEntryNode::GetBaseNode();
      return reinterpret_cast<const SelfType*>(base_node + TableOffset);
    }

    __always_inline static Win64LDRDataTableEntry* GetExecutableEntry() __noexcept {
      // initOrder doesn't have the executable as the base, so we always use memOrder.
      const auto mem_node = TWin64ListEntry<Win64ModuleType::memOrder>::GetListSentinel();
      return mem_node->prev()->asLDRDataTableEntry();
    }

    //=== General ===//

    Win64LDRDataTableEntry* findModule(const wchar_t* str, bool ignore_extension = false) const {
      (void)ignore_extension;
      if __expect_false(!str) 
        return nullptr;
      SelfType* curr = this->asMutable();
      if __expect_false(curr->isSentinel())
        curr = curr->next();
      // This is evil... buuut we never modify the buffer so it's ok ;)
      const auto ustr = Win64UnicodeString::New(const_cast<wchar_t*>(str));
      while(!curr->isSentinel()) {
        const auto tbl = curr->asLDRDataTableEntry();
        const Win64UnicodeString& dll_ustr = tbl->base_dll_name;
        if(dll_ustr.isEqual(ustr)) return tbl;
        curr = curr->next();
      }
      return nullptr;
    }

    Win64LDRDataTableEntry* findModule(const char* str, bool ignore_extension = false) const {
      if __expect_false(!str) 
        return nullptr;
      // return findModule(wconvert(str)?, ignore_extension);
      const usize len = C::__strlen(str);
      auto wstr = $dynalloc(len + 1, wchar_t).zeroMemory();
      for(usize I = 0; I < len; ++I)
        wstr[I] = static_cast<wchar_t>(str[I]);
      return findModule(wstr, ignore_extension);
    }

    //=== Observers ===//

    bool isSentinel() const __noexcept {
      return this == GetListSentinel();
    }

    [[gnu::always_inline]] SelfType* next() const {
      auto* base_next = asMutable()->BaseType::__next;
      return reinterpret_cast<SelfType*>(base_next);
    }

    [[gnu::always_inline]] SelfType* prev() const {
      auto* base_prev = asMutable()->BaseType::__prev;
      return reinterpret_cast<SelfType*>(base_prev);
    }

    Win64UnicodeString name() const {
      return this->asLDRDataTableEntry()->name();
    }
  
    //=== Conversions ===//

    [[gnu::always_inline, gnu::const]]
    TblType* asLDRDataTableEntry() const {
      // Stupid fucking windows bullshit
      SelfType* table_base = this->asMutable() - TableOffset;
      auto* pLDR_dte = reinterpret_cast<TblType*>(table_base);
      // This is fine because we know it IS a data table entry.
      // TODO: Check if I'm chattin shit
      return $launder(pLDR_dte);
    }

    [[gnu::always_inline, gnu::const]]
    SelfType* asMutable() const {
      return const_cast<SelfType*>(this);
    }
  };

  extern template struct TWin64ListEntry<Win64ModuleType::loadOrder>;
  extern template struct TWin64ListEntry<Win64ModuleType::memOrder>;
  extern template struct TWin64ListEntry<Win64ModuleType::initOrder>;

  using Win64LoadOrderList = TWin64ListEntry<Win64ModuleType::loadOrder>;
  using Win64MemOrderList  = TWin64ListEntry<Win64ModuleType::memOrder>;
  using Win64InitOrderList = TWin64ListEntry<Win64ModuleType::initOrder>;

  template <typename T>
  concept __is_win64_list_entry = 
    __is_base_of(Win64ListEntryNode, T);

  //=== PEB Data ===//

  struct Win64PEBLDRData {
    u32                length;
    Win64Bool          is_initialized;
    Win64Handle        ss_handle;
    _MutLDRNode        __mlist_in_load_order;
    _MutLDRNode        __mlist_in_mem_order;
    _MutLDRNode        __mlist_in_init_order;
    Win64Addr          entry_in_progress;
    Win64Bool          is_shutdown_in_progress;
    Win64Handle        shutdown_thread_id;
    HC_MARK_DELETED(Win64PEBLDRData);
  public:
    Win64ListEntryNode* getBaseEntryNode() const {
      const auto* base = &this->__mlist_in_load_order;
      return const_cast<Win64ListEntryNode*>(base);
    }

    template <usize Offset>
    [[gnu::always_inline, gnu::flatten]]
    TWin64ListEntry<Offset>* getEntryNodeAt() const {
      static_assert(Offset < 3, "Invalid offset.");
      using CastType = TWin64ListEntry<Offset>;
      auto* base = this->getBaseEntryNode();
      return reinterpret_cast<CastType*>(base + Offset);
    }
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
    Win64InitOrderList* getLDRModulesInInitOrder() const;
    Win64MemOrderList*  getLDRModulesInMemOrder() const;
    Win64LoadOrderList* getLDRModulesInLoadOrder() const;
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
    Win64Addr   __fast_syscall_addr;
    u32         __current_locale;
    u32         fp_status_reg;
    u32         __os_reserved[54];
    u32         exception_code;
    // ...
    HC_MARK_DELETED(Win64TEB);
  public:
    static Win64TEB* LoadTEBFromGS();
    Win64AddrRange getStackRange();
    uptr getProcessId() const;
    uptr getThreadId() const;
    Win64PEB* getPEB() const;
  };
} // namespace hc::bootstrap