//===- Sys/Win/Wait.hpp ---------------------------------------------===//
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

#include <Sys/Core/Nt/Structs.hpp>

namespace hc::sys {
inline namespace __nt {
  inline win::NtStatus wait_single(
    win::WaitHandle handle,
    win::LargeInt* timeout,
    bool alertable
  ) {
    return isyscall<NtSyscall::WaitForSingleObject>(
      $unwrap_handle(handle), timeout, 
      win::Boolean(alertable)
    );
  }

  inline win::NtStatus wait_single(
    win::WaitHandle handle
  ) {
    return isyscall<NtSyscall::WaitForSingleObject>(
      $unwrap_handle(handle), 
      (win::LargeInt*)nullptr, 
      win::Boolean(false)
    );
  }
} // inline namespace __nt
} // namespace hc::sys
