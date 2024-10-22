//===- Sys/Win/Except.hpp -------------------------------------------===//
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

#include "Nt/Except.hpp"

namespace hc::sys {
inline namespace __nt {
  /// Raises a premade SEH exception.
  /// @param record A valid `ExceptionRecord`
  /// @param ctx Pointer to a platform-specific `ContextSave`.
  /// @param first_chance
  ///   If exception handlers should be considered.
  ///   If `false`, calling process will be killed.
  __nt_attrs win::NtStatus raise_exception(
   win::ExceptionRecord& record,
   __nonnull win::ContextSave* ctx,
   bool first_chance = false
  ) {
    return isyscall<NtSyscall::RaiseException>(
      &record, ctx, win::Boolean(first_chance)
    );
  }

  /// Creates and raises a first-chance SEH exception.
  /// @param code The status code to raise.
  /// @param address An address (obviously).
  /// @param args
  ///   Arguments to be passed.
  ///   Each must be `<= sizeof(void*)`.
  inline void raise_exception(
   i32 code,
   __nonnull auto* address,
   auto...args
  ) {
    using ExType = win::ExceptionRecord;
    auto record = ExType::New(code, address, args...); 
    ExType::Raise(record);
  }
} // namespace __nt
} // namespace hc::sys
