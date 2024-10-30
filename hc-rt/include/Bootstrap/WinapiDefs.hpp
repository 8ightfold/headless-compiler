//===- Bootstrap/WinapiDefs.hpp -------------------------------------===//
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
//  Defines structures used by the public Windows API (in this project).
//  Other parts may be defined in Sys/Win/Nt/*.hpp
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Features.hpp>
#include <Common/Fundamental.hpp>
#include <Bootstrap/LargeInteger.hpp>
#include <Bootstrap/UnicodeString.hpp>

//======================================================================//
// Aliases
//======================================================================//

namespace hc {

namespace common {
template <typename> struct PtrRange;
using AddrRange = PtrRange<void>;
} // namespace common

namespace sys {
struct IIOFile;
struct RawOSSemaphore;
} // namespace sys

using RawIOFile = sys::IIOFile;
using IOFile    = RawIOFile*;

namespace bootstrap {
struct Win64ListEntryNode;
struct Win64LDRDataTableEntry;
struct Win64PEBLDRData;
struct Win64CurrDir;
struct Win64ProcParams;
struct Win64PEB;
struct Win64TIB;
struct Win64TEB;

using Win64Addr           = void*;
using Win64AddrRange      = common::AddrRange;
using Win64Handle         = hc::__void*;
using Win64Lock           = sys::RawOSSemaphore;
using Win64Bool           = ubyte;

using Win64LargeInt       = LargeInt;
using Win64ULargeInt      = ULargeInt;
using Win64UnicodeString  = UnicodeString;
} // namespace bootstrap

} // namespace hc

//======================================================================//
// New Definitions
//======================================================================//

namespace hc::bootstrap {

//////////////////////////////////////////////////////////////////////////
// Kernel

enum KAffinity : uptr {};

struct KSystemTime {
  u32 low;
  i32 high;
  i32 high_alt;
};

enum KPriority : u32 {
  KP_Low        = 0x00000001,
  KP_Normal     = 0x40000000,
  KP_High       = 0x80000000,
  KP_Exclusive  = 0xFFFFFFFF,
};

struct KSPriority {
  KPriority prio_class;
  u32 prio_subclass;
};

struct KUserTimes {
  LargeInt create_time;
  LargeInt exit_time;
  LargeInt kernel_time;
  LargeInt user_time;
};

//////////////////////////////////////////////////////////////////////////
// Userland

struct ClientID {
  Win64Handle unique_process;
  Win64Handle unique_thread;
};

struct IOCounters {
  u64 read_operations;
  u64 write_operations;
  u64 other_operations;
  u64 read_transfers;
  u64 write_transfers;
  u64 other_transfers;
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

struct PooledUsageAndLimits {
  usize peak_paged_pool_usage;
  usize paged_pool_usage;
  usize paged_pool_limit;
  usize peak_non_paged_pool_usage;
  usize non_paged_pool_usage;
  usize non_paged_pool_limit;
  usize peak_pagefile_usage;
  usize pagefile_usage;
  usize pagefile_limit;
};

struct RateQuotaLimits {
  u32 data;
  u32 bitfields;
};

struct QuotaLimits {
  usize     paged_pool_limit;
  usize     non_paged_pool_limit;
  usize     min_working_set_size;
  usize     max_working_set_size;
  usize     pagefile_limit;
  LargeInt  time_limit;
};

struct QuotaLimitsEx : public QuotaLimits {
  usize     working_set_limit;
  usize     __reserved0;
  usize     __reserved1;
  usize     __reserved2;
  u32       flags;
  RateQuotaLimits cpu_rate_limits;
};

struct VMCounters {
  usize peak_virtual_size;
  usize virtual_size;
  u32   page_fault_count;
  usize peak_working_set_size;
  usize working_set_size;
  usize quota_peak_paged_pool_usage;
  usize quota_paged_pool_usage;
  usize quota_peak_non_paged_pool_usage;
  usize quota_non_paged_pool_usage;
  usize pagefile_usage;
  usize peak_pagefile_usage;
};

struct VMCountersEx : public VMCounters {
  usize __private;
};

struct VMCountersEx2 : public VMCountersEx {
  usize private_working_set_size;
  usize shared_commit_usage;
};

static_assert(sizeof(VMCountersEx)
  == sizeof(VMCounters) + sizeof(usize));
static_assert(sizeof(VMCountersEx2)
  == sizeof(VMCountersEx) + sizeof(usize[2]));

} // namespace hc::bootstrap
