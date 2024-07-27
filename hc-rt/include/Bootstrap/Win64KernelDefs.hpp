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
#include <Bootstrap/UnicodeString.hpp>

// For more info:
// "Finding Kernel32 Base and Function Addresses in Shellcode"
// https://en.wikipedia.org/wiki/Win32_Thread_Information_Block
// https://www.geoffchappell.com/studies/windows/km/
// https://github.com/wine-mirror/wine/blob/master/include/winternl.h

#define _MutLDRNode mutable ::hc::bootstrap::Win64ListEntryNode

static_assert(HC_PLATFORM_WIN64, "Windows only.");

namespace hc {
namespace sys {
  struct IIOFile;
  struct RawOSSemaphore;
} // namespace sys

using RawIOFile = sys::IIOFile;
using IOFile    = RawIOFile*;

namespace bootstrap {
  using Win64Addr       = void*;
  using Win64AddrRange  = common::AddrRange;
  using Win64Handle     = hc::__void*;
  using Win64Lock       = sys::RawOSSemaphore;
  using Win64Bool       = ubyte;

  struct Win64ModuleType {
    static constexpr usize loadOrder  = 0U;
    static constexpr usize memOrder   = 1U;
    static constexpr usize initOrder  = 2U;
  };

  template <usize TableOffset>
  struct TWin64ListEntry;

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
    __always_inline Win64UnicodeString fullName() const {
      return this->full_dll_name;
    }

    //==================================================================//
    // Conversions
    //==================================================================//

    template <usize TableOffset = Win64ModuleType::memOrder>
    [[gnu::always_inline, gnu::const]]
    TWin64ListEntry<TableOffset>* asListEntry() const {
      using TblType = TWin64ListEntry<TableOffset>;
      // Stupid fucking windows bullshit
      auto* const pnode = reinterpret_cast<TblType*>(this->asMutable());
      auto* plist = pnode + TableOffset;
      return $launder(plist);
    }

    [[gnu::always_inline, gnu::const]]
    Win64LDRDataTableEntry* asMutable() const {
      return const_cast<Win64LDRDataTableEntry*>(this);
    }
  };

  template <usize TableOffset>
  struct TWin64ListEntry : Win64ListEntryNode {
    using BaseType = Win64ListEntryNode;
    using SelfType = TWin64ListEntry<TableOffset>;
    using TblType  = Win64LDRDataTableEntry;

    struct Iterator {
      using difference_type = uptrdiff;
      using value_type = TWin64ListEntry*;
    public:
      bool operator==(const Iterator&) const = default;
      value_type operator*()  const { return __iter_val; }
      value_type operator->() const { return __iter_val; }

      Iterator& operator++() {
        this->__iter_val = __iter_val->prev();
        return *this;
      }
      Iterator operator++(int) {
        Iterator tmp = *this;
        ++*this;
        return tmp;
      }
    
    public:
      value_type __iter_val = nullptr;
    };

    struct ListProxy {
      Iterator begin() const {
        return {end()->prev()};
      }
      Iterator end() const {
        return {GetListSentinel()->asMutable()};
      }
    };

  public:
    [[gnu::const]] static const SelfType* GetListSentinel() __noexcept {
      // We assume the base node never changes.
      // If it does, something has gone horribly wrong...
      static const auto* base_node = Win64ListEntryNode::GetBaseNode();
      return reinterpret_cast<const SelfType*>(base_node + TableOffset);
    }

    __always_inline static Win64LDRDataTableEntry* GetExecutableEntry() __noexcept {
      // initOrder doesn't have the executable as the base, so we always use memOrder.
      const auto mem_node = TWin64ListEntry<Win64ModuleType::memOrder>::GetListSentinel();
      return mem_node->prev()->asLDRDataTableEntry();
    }

    static ListProxy GetIterable() __noexcept {
      return ListProxy{};
    }

    //==================================================================//
    // General
    //==================================================================//

    Win64LDRDataTableEntry* findModule(const wchar_t* S, bool ignore_extension = false) const {
      (void)ignore_extension;
      if __expect_false(!S) 
        return nullptr;
      SelfType* curr = this->asMutable();
      if __expect_false(curr->isSentinel())
        curr = curr->next();
      // This is evil... buuut we never modify the buffer so it's ok ;)
      const auto ustr = Win64UnicodeString::New(const_cast<wchar_t*>(S));
      while (!curr->isSentinel()) {
        const auto tbl = curr->asLDRDataTableEntry();
        const Win64UnicodeString& dll_ustr = tbl->base_dll_name;
        if(dll_ustr.isEqual(ustr)) return tbl;
        curr = curr->next();
      }
      return nullptr;
    }

    Win64LDRDataTableEntry* findModule(
     const char* S, bool ignore_extension = false) const {
      if __expect_false(!S) 
        return nullptr;
      return this->findModule(
        $to_wstr(S).data(), 
        ignore_extension
      );
    }

    //==================================================================//
    // Observers
    //==================================================================//

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
    Win64UnicodeString fullName() const {
      return this->asLDRDataTableEntry()->fullName();
    }
  
    //==================================================================//
    // Conversions
    //==================================================================//

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

  //====================================================================//
  // PEB Data
  //====================================================================//

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

  struct Win64CurrDir {
    Win64UnicodeString dos_path;
    Win64Handle handle; // ?
  };

  struct Win64ProcParams {
    u32          allocation_size;
    u32          size;
    u32          flags;
    u32          debug_flags;
    Win64Handle  console_handle;
    u32          console_flags;
    IOFile       std_in;
    IOFile       std_out;
    IOFile       std_err;
    Win64CurrDir curr_dir;
    // ...
    HC_MARK_DELETED(Win64ProcParams);
  public:
    bool hasConsole() const { return !!console_handle; }
    Win64UnicodeString getCurrDir() const {
      return curr_dir.dos_path;
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
    Win64ProcParams*  process_params;
    Win64Addr         subsystem_data;
    Win64Handle       process_heap;
    Win64Lock*        fast_PEB_lock;
    Win64Addr         fast_PEB_lock_fn;   // PPEBLOCKROUTINE
    Win64Addr         fast_PEB_unlock_fn; // PPEBLOCKROUTINE
    u32               env_update_count;
    Win64Addr         ke_callback_tbl;
    // ...
    HC_MARK_DELETED(Win64PEB);
  public:
    Win64InitOrderList* getLDRModulesInInitOrder() const;
    Win64MemOrderList*  getLDRModulesInMemOrder() const;
    Win64LoadOrderList* getLDRModulesInLoadOrder() const;
  };

  //====================================================================//
  // TEB Data
  //====================================================================//
  
  struct ClientID {
    Win64Handle unique_process;
    Win64Handle unique_thread;
  };

  struct ListEntry {
    ListEntry* prev;
    ListEntry* next;
  };

  struct alignas(u32) ProcessorNumber {
    u16   group;
    ubyte number;
    ubyte __reserved;
  };

  ////////////////////////////////////////////////////////////////////////

  struct Win64TIB {
    Win64Addr         SEH_frame;
    Win64Addr         stack_begin;
    Win64Addr         stack_end;
    Win64Addr         subsystem_TIB;
    Win64Addr         fiber_info;
    Win64Addr         arbitrary_data;
    Win64Addr         TEB_addr;
    HC_MARK_DELETED(Win64TIB);
  };

  struct Win64TEB {
    Win64TIB          tib;
    Win64Addr         env_addr;
    ClientID          client_id;
    Win64Addr         rpc_handle;
    Win64Addr         TLS_array_addr;
    Win64Addr         PEB_addr;
    u32               last_error;
    u32               critical_section_count;
    Win64Addr         CSR_thread_addr;
    Win64Addr         win32_thread_info;
    u32               __user32_reserved[26];
    u32               __user_reserved[5];
    Win64Addr         __WOW32_reserved;
    u32               __current_locale;
    u32               fp_status_reg;
    Win64Addr         __dbg_reserved[16];
    Win64Addr         __system1_reserved[30];
    char              placeholder_compat_mode;
    ubyte             placeholder_hydration;
    char              __placeholder_reserved[10];
    u32               proxied_process_id;
    void*             activation_stack_tmp[5];
    ubyte             working_behalf_ticket[8];
    i32               exception_code;
    Win64Addr         activation_stack_ptr;
    u64               instrumentation_cb_sp;
    u64               instrumentation_cb_prev_pc;
    u64               instrumentation_cb_prev_sp;
    u32               tx_fs_ctx;
    ubyte             instrumentation_cb_disabled;
    ubyte             unaligned_loadstore_exceptions;
    void*             GDI_teb_batch_tmp[157];
    ClientID          real_client_id;
    Win64Addr         GDI_cached_process_handle;
    u32               GDI_client_PID;
    u32               GDI_client_TID;
    Win64Addr         GDI_TLS_info;
    u64               win32_client_info[62];
    void*             __gl_dispatch_table[233];
    uptr              __gl_reserved[30];
    void*             __gl_data[5];
    u32               last_status;
    Win64UnicodeString static_unicode_string;
    wchar_t            static_unicode_buf[261];
    void*             dealloc_stack;
    void*             tls_slots[64];
    ListEntry         tls_links;
    void*             VDM;
    void*             __nt_RPC_reserved;
    void*             __dbg_SS_reserved[2];
    u32               hard_error_mode;
    void*             instrumentation[11];
    u32               activity_id[4];
    void*             subprocess_tag;
    void*             perflib_data;
    void*             ETWtrace_data;
    void*             winsock_data;
    u32               GDI_batch_count;
    ProcessorNumber   curr_ideal_processor;
    u32               guaranteed_stack_bytes;
    void*             __perf_reserved;
    void*             __ole_reserved;
    u32               waiting_on_loader_lock;
    void*             saved_prio_state;
    u64               __codecov_reserved;
    void*             threadpool_data;
    void**            TLS_expansion_slots;
    Win64Addr         __chpev2_cpu_area_info;
    void*             __unused;
    u32               MUI_generation;
    u32               is_impersonating;
    void*             NLS_cache;
    void*             shim_data;
    u32               heap_data;
    void*             curr_transaction_handle;
    Win64Addr         active_frame;
    void*             FLS_data;
    void*             __language_data[3];
    u32               MUI_impersonation;
    volatile u16      cross_TEB_flags;
    union {
      u16             same_TEB_flags;
      struct {
        u16           safe_thunk_call         : 1;
        u16           in_debug_print          : 1;
        u16           has_fiber_data          : 1;
        u16           skip_thread_attach      : 1;
        u16           in_ship_assert_mode     : 1;
        u16           ran_process_init        : 1;
        u16           cloned_thread           : 1;
        u16           suppress_debug_msg      : 1;
        u16           disable_user_stackwalk  : 1;
        u16           rtl_exception_attached  : 1;
        u16           initial_thread          : 1;
        u16           session_aware           : 1;
        u16           load_owner              : 1;
        u16           loader_worker           : 1;
        u16           skip_loader_init        : 1;
        u16           skip_file_brokering     : 1;
      };
    };
    void*             TXN_scope_enter_cb;
    void*             TXN_scope_exit_cb;
    // ...
    HC_MARK_DELETED(Win64TEB);
  public:
    static Win64TIB* LoadTIBFromGS();
    static Win64TEB* LoadTEBFromGS();
    static Win64PEB* LoadPEBFromGS();
    static Win64ProcParams* GetProcessParams();
    Win64AddrRange getStackRange() const;
    uptr getProcessId() const;
    uptr getThreadId() const;
    Win64PEB* getPEB() const;
  };
} // namespace bootstrap
} // namespace hc
