//===- Sys/Win/Mutant.hpp -------------------------------------------===//
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

#include <Sys/Core/Nt/Mutant.hpp>

namespace hc::sys {
  inline win::MutexHandle win_create_mutant(
    win::NtStatus& S,
    NtAccessMask mask = win::MutantAllAccess,
    const wchar_t* name = nullptr,
    bool owner = false
  ) {
    win::MutexHandle hout;
    auto ustr = make_unicode_string(name);
    win::ObjectAttributes attr { .object_name = &ustr };
    S = isyscall<NtSyscall::CreateMutant>(
      &hout, mask, 
      name ? &attr : nullptr, 
      win::Boolean(owner)
    );
    return hout;
  }

  inline win::NtStatus win_query_mutant(
    win::MutexHandle handle,
    win::MutantInfoClass type,
    win::BasicMutantInfo& info,
    win::ULong* result_len = nullptr
  ) {
    return isyscall<NtSyscall::QueryMutant>(
      $unwrap_handle(handle), type,
      &info, win::ULong(sizeof(info)),
      result_len
    );
  }

  inline win::NtStatus win_release_mutant(
    win::MutexHandle handle,
    i32* prev_count = nullptr
  ) {
    return isyscall<NtSyscall::ReleaseMutant>(
      $unwrap_handle(handle), prev_count
    );
  }

  __always_inline win::NtStatus win_close_mutant(
    win::MutexHandle handle
  ) {
    return isyscall<NtSyscall::Close>(
      $unwrap_handle(handle));
  }
} // namespace hc::sys
