//===- Sys/Win/Process.hpp ------------------------------------------===//
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

#pragma once

#include "Nt/Process.hpp"
#include <Common/Casting.hpp>
#include <Common/Pair.hpp>
#include <Common/Result.hpp>

namespace hc::sys {
inline namespace __nt {

/// Gets a handle to the current process.
inline win::ProcessHandle current_process() {
  static constexpr uptr curr = iptr(-1);
  void* const P = ptr_cast<>(curr);
  return win::ProcessHandle::New(P);
}

/// Invokes `NtTerminateProcess`.
__nt_attrs win::NtStatus __terminate_process(
 const void* process_raw, win::Long exit_status) {
  return isyscall<NtSyscall::TerminateProcess>(
    process_raw,
    exit_status
  );
}

/// Terminates a process from a handle.
inline win::NtStatus terminate_process(
 win::ProcessHandle handle,
 win::NtStatus exit_status
) {
  return __terminate_process(
    $unwrap_handle(handle),
    win::Long(exit_status)
  );
}

/// Terminates the current process.
inline win::NtStatus terminate_process(win::NtStatus exit_status) {
  return terminate_process(current_process(), exit_status);
}

/// Terminates the current process' threads.
inline win::NtStatus terminate_process_threads(win::NtStatus exit_status) {
  return __terminate_process(nullptr, win::Long(exit_status));
}

//======================================================================//
// [Query|Set]Information
//======================================================================//

template <typename T, usize>
struct _ProcQueryTypeDeduced {
  using Type = T;
};

template <typename T, usize N>
struct _ProcQueryTypeDeduced<T[], N> {
  static_assert(N > 0,
    "This query requires array bounds!");
  using Type = T[N];
};

template <class QType, usize N>
using __procinfo_type =
  typename _ProcQueryTypeDeduced<
    typename QType::Type, N>::Type;

using QueryPair = com::Pair<win::NtStatus, /*RealBytes*/ win::ULong>;

template <typename T>
using _QueryResult = com::Result<T, QueryPair>;

template <typename QS>
using QueryResult = _QueryResult<meta::Decay<typename QS::Type>>;

template <typename To, typename From>
inline __abi_hidden void __qpi_assign(To& to, From from) {
  to = static_cast<To>(from);
}

template <win::ProcInfo InfoClass, typename...Query>
__nt_attrs auto query_process_info(
  win::ProcessHandle handle, Query&&...query)
 -> com::Pair<win::NtStatus, /*Length*/ win::ULong> {
  using QType = win::ProcQuery<InfoClass>;
  static_assert(win::__is_valid_procinfo<QType, Query...>,
    "Invalid Query type!");
  // Return ASAP if invalid.
  if __expect_false(!handle) {
    constexpr win::NtStatus S = i32(0xC0000008U);
    return {/*INVALID_HANDLE*/ S, 0};
  }
  
  auto&& info = QType::Arg(query...);
  win::ULong size = QType::Size(query...);
  win::ULong out_size = 0;

  win::NtStatus status = isyscall<
   NtSyscall::QueryInformationProcess>(
    handle.get(), InfoClass,
    ptr_cast<>(&info), size, &out_size
  );
  if constexpr (QType::needsCasting)
    __qpi_assign(query..., info);
  return {status, out_size};
}

template <win::ProcInfo InfoClass, usize ArrSize = 0>
inline auto query_process(win::ProcessHandle handle)
 -> QueryResult<win::ProcQuery<InfoClass>> {
  using QType = win::ProcQuery<InfoClass>;
  // Return ASAP if invalid.
  if __expect_false(!handle)
    return $Err(/*INVALID_HANDLE*/ 0xC0000008, 0);
  __procinfo_type<QType, ArrSize> query {};
  auto [status, bytes] = query_process_info<InfoClass>(handle, query);
  if ($NtFail(status))
    return $Err(status, bytes);
  return $Ok(query);
}

template <win::ProcInfo InfoClass, usize ArrSize = 0>
inline auto query_process() {
  return query_process<InfoClass, ArrSize>(current_process());
}

//////////////////////////////////////////////////////////////////////////

template <win::ProcInfo InfoClass, typename...Set>
__nt_attrs win::NtStatus set_process_info(
  win::ProcessHandle handle, Set&&...set) {
  using SType = win::ProcSet<InfoClass>;
  static_assert(win::__is_valid_procinfo<SType, Set...>,
    "Invalid Set type!");
  // Return ASAP if invalid.
  if __expect_false(!handle)
    return 0xC0000008;
  
  auto&& info = SType::Arg(set...);
  win::ULong size = SType::Size(set...);

  return isyscall<NtSyscall::SetInformationProcess>(
    handle.get(), InfoClass,
    ptr_cast<>(&info), size
  );
}

template <win::ProcInfo InfoClass, typename...Set>
inline bool set_process(win::ProcessHandle handle, Set&&...set) {
  const win::NtStatus status
    = set_process_info<InfoClass>(handle, set...);
  return $NtSuccess(status);
}

template <win::ProcInfo InfoClass, typename...Set>
inline bool set_process(Set&&...set) {
  const win::NtStatus status
    = set_process_info<InfoClass>(current_process(), set...);
  return $NtSuccess(status);
}

} // inline namespace __nt
} // namespace hc::sys
