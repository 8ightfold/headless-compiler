//===- Sys/IO/Win/Filesystem.hpp ------------------------------------===//
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
  inline win::MutantHandle win_create_mutant(
    win::NtStatus& S,
    NtAccessMask mask = win::MutantAllAccess,
    bool owner = true,
    const wchar_t* name = nullptr
  ) {
    win::MutantHandle hout;
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
    win::MutantHandle handle,
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
    win::MutantHandle handle,
    i32* prev_count = nullptr
  ) {
    return isyscall<NtSyscall::ReleaseMutant>(
      $unwrap_handle(handle), prev_count
    );
  }
} // namespace hc::sys
