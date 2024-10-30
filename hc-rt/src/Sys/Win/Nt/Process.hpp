//===- Sys/Win/Nt/Process.hpp ---------------------------------------===//
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
//  Info used for Nt[Query|Set]InformationProcess.
//
//===----------------------------------------------------------------===//

#pragma once
#pragma clang system_header

#include "Structs.hpp"
#include "Filesystem.hpp"
#include "Utils.hpp"

// For more info:
// https://processhacker.sourceforge.io/doc/ntpsapi_8h_source.html
// https://ntdoc.m417z.com/processinfoclass

namespace hc::sys::win {

enum class ProcInfo : ULong {
# define ProcDecl(name, ...) name,
# define $ProcQ_(name, ...)  name,
# define $Proc_S(name, ...)  name,
# define $ProcQS(name, ...)  name,
# define $ProcUnused(name, ...) name,
// TODO: Remove when complete
# define _NT_PROCINFO_EXTRAS 1
# include "Process.mac"
  MaxValue
};

//======================================================================//
// Classes
//======================================================================//

namespace proc {

enum IoPriorityHint : ULong {
  IOP_VeryLow = 0, // Defragging, content indexing and other background I/Os.
  IOP_Low, // Prefetching for applications.
  IOP_Normal, // Normal I/Os.
  IOP_High, // Used by filesystems for checkpoint I/O.
  IOP_Critical, // Used by memory manager. Not available for applications.
};

enum MemExecOption : ULong {
  ME_Enable                         = 0x01,
  ME_Disable                        = 0x02,
  ME_DisableThunkEmulation          = 0x04,
  ME_Permanent                      = 0x08,
  ME_ExecuteDispatchEnable          = 0x10,
  ME_ImageDispatchEnable            = 0x20,
  ME_ExceptionChainValidateDisable  = 0x40
};

//////////////////////////////////////////////////////////////////////////
// Objects

struct BasicInfo {
  NtStatus      exit_status;
  PEB*          base_address;
  KAffinity     affinity_mask;
  KPriority     base_priority;
  ProcessHandle process_id;
  ProcessHandle parent_process_id;
};

struct BasicInfoEx {
  usize size = sizeof(BasicInfoEx);
  BasicInfo base;
  union {
    ULong flags;
    struct {
      ULong is_protected : 1;
      ULong is_wow64 : 1;
      ULong is_process_deleting : 1;
      ULong is_cross_session_create : 1;
      ULong is_frozen : 1;
      ULong is_background : 1; // WIN://BGKD
      ULong is_strongly_named : 1; // WIN://SYSAPPID
      ULong is_secure : 1;
      ULong is_subsystem_process : 1;
      ULong is_trusted_app : 1; // since 24H2
      ULong __bitpadding : 22;
    };
  };
};

// ...

struct AccessToken {
  AccessTokHandle token; // TOKEN_ASSIGN_PRIMARY access
  ThreadHandle thread; // THREAD_QUERY_INFORMATION access
};

union AffinityUpdateMode {
  ULong flags;
  struct {
    ULong enable_autoupdate : 1;
    ULong permanent : 1;
    ULong __bitpadding : 30;
  };
};

struct CycleTimeInfo {
  u64 accumulated_cycles;
  u64 current_cycle_count;
};

struct DevicemapInfo {
  union {
    DirectoryHandle directory_handle;
    struct {
      ULong map; // Bitmask
      u8 type[32]; // DRIVE_* WinBase.h
    } drive;
  };
};

struct DevicemapInfoEx
 : public DevicemapInfo {
  ULong flags;
};

struct ExceptionPort {
  PortHandle port; // TODO: Type check?
  ULong state_flags;
};

struct ForegroundInfo {
  Boolean foreground;
};

struct HandleInfo {
  ULong count;
  ULong high_watermark;
};

struct InstrumentationCallbackInfo {
  ULong version;
  ULong __reserved;
  void* callback;
};

struct LDTEntry {
  i16 limit_lo;
  i16 base_lo;
  union {
    struct {
      u8 mid;
      u8 __flagsA;
      u8 __flagsb;
      u8 hi;
    } bytes;
    struct {
      DWord base_mid : 8;
      DWord type : 5;
      DWord dpl : 2;
      DWord present : 1;
      DWord limit_hi : 4;
      DWord sys : 1;
      DWord __reserved : 1;
      DWord default_big : 1;
      DWord granularity : 1;
      DWord base_hi : 8;
    };
  };
};

struct LDTInfo {
  ULong start;
  ULong length;
  union {
    LDTEntry entries[];
    LDTEntry __forsize[1];
  };
}; // TODO: Add wrapper

struct LDTSize {
  ULong length;
};

union MemoryAllocMode {
  ULong flags;
  struct {
    ULong top_down : 1;
    ULong __bitpadding : 31;
  };
};

struct PriorityClass {
  Boolean foreground;
  u8 priority_class;
};

struct SessionInfo {
  ULong session_id;
};

struct StackAllocInfo {
  usize reserve_size;
  usize zero_bits;
  void* stack_base;
};

struct StackAllocInfoEx {
  ULong preferred_node;
  ULong __reserved[3];
  StackAllocInfo base;
};

struct TracingEnable {
  ULong flags;
};

struct TracingEnableEx {
  ULong flags;
  ULong slots;
};

struct TracingEntry {
  GenericHandle handle;
  ClientID client_id;
  ULong type;
  void* stacks[/*PROCESS_HANDLE_TRACING_MAX_STACKS*/16];
};

struct TracingQuery {
  GenericHandle handle;
  ULong total_traces;
  union {
    TracingEntry entries[];
    TracingEntry __forsize[1];
  };
};

struct WSWatchInfo {
  void* vaulting_pc;
  void* vaulting_va;
};

struct WSWatchInfoEx : public WSWatchInfo {
  ULongPtr faulting_thread_id;
  ULongPtr flags;
};

} // namespace proc

//======================================================================//
// Definitions
//======================================================================//

namespace proc {

template <
  class T = void,
  class TEx = T,
  class TEx2 = TEx>
struct InfoAssocBase {
  static constexpr bool isSet = true;
  using Type    = T;
  using TypeEx  = TEx;
  using TypeEx2 = TEx2;
};

template <ProcInfo> struct InfoQuery {
  static constexpr bool isSet = false;
};

template <ProcInfo> struct InfoSet {
  static constexpr bool isSet = false;
};

#define $GenInfo(mode, name, ty...) \
template <> struct mode <ProcInfo::name> \
  : public InfoAssocBase<ty> {};
#define $ProcQ_(name, ty...) $GenInfo(InfoQuery, name, ty)
#define $Proc_S(name, ty...) $GenInfo(InfoSet, name, ty)

#include "Process.mac"

#undef $GenInfo

} // namespace proc

//======================================================================//
// Wrapper
//======================================================================//

template <ProcInfo Info>
concept is_procQuery = proc::InfoQuery<Info>::isSet;

template <ProcInfo Info>
concept is_procSet = proc::InfoSet<Info>::isSet;

template <class Base> struct ProcInfoBase {
  using Info    = QSInfo<typename Base::Type>;
  using InfoEx  = QSInfo<typename Base::TypeEx>;
  using InfoEx2 = QSInfo<typename Base::TypeEx2>;
};

template <ProcInfo Info> struct ProcInfoQuery {
  static_assert(is_procQuery<Info>,
    "Query has not been enabled for this command!");
public:
  using Type = proc::InfoQuery<Info>;
  using Base = ProcInfoBase<Type>;
  // Public defs
  using Query = QSQueryType<typename Base::Info>;
  using QueryEx = QSQueryType<typename Base::InfoEx>;
  using QueryEx2 = QSQueryType<typename Base::InfoEx2>;
};

template <ProcInfo Info> struct ProcInfoSet {
  static_assert(is_procSet<Info>,
    "Set has not been enabled for this command!");
public:
  using Type = proc::InfoSet<Info>;
  using Base = ProcInfoBase<Type>;
  // Public defs
  using Set = QSSetType<typename Base::Info>;
  using SetEx = QSSetType<typename Base::InfoEx>;
  using SetEx2 = QSSetType<typename Base::InfoEx2>;
};

template <ProcInfo Info>
using ProcQuery = typename ProcInfoQuery<Info>::Query;

template <ProcInfo Info>
using ProcQueryEx = typename ProcInfoQuery<Info>::QueryEx;

template <ProcInfo Info>
using ProcSet = typename ProcInfoSet<Info>::Set;

template <ProcInfo Info>
using ProcSetEx = typename ProcInfoSet<Info>::SetEx;

template <class QSType, typename...Args>
concept __is_valid_procinfo =
  (sizeof...(Args) >= QSType::argMin)
&& (sizeof...(Args) <= QSType::argMin)
&& requires(Args&...args) {
  QSType::Size(args...);
  QSType::Arg(args...);
};

} // namespace hc::sys::win
