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

#include "Nt/Generic.hpp"
#include <Common/Casting.hpp>

namespace hc::sys {
inline namespace __nt {
  /// Gets a handle to the current process.
  inline win::ProcessHandle current_process() {
    static constexpr uptr curr = iptr(-1);
    void* const P = ptr_cast<>(curr);
    return win::ProcessHandle::New(P);
  }

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
} // inline namespace __nt
} // namespace hc::sys
