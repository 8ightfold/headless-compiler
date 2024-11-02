//===- Sys/Win/Object.hpp -------------------------------------------===//
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

#include "Filesystem.hpp"
#include "Process.hpp"
#include "Nt/Object.hpp"

namespace hc::sys {
inline namespace __nt {

template <win::__is_HANDLE Handle>
__nt_attrs win::NtStatus duplicate_object_ex(
 Handle source, win::ProcessHandle source_proc,
 Handle& target, win::ProcessHandle target_proc,
 win::AccessMask desired_access,
 win::ObjAttribMask attr,
 win::DupOptsMask opts
  = win::DupOptsMask::None
) {
  return isyscall<NtSyscall::DuplicateObject>(
    $unwrap_handle(source), $unwrap_handle(source_proc),
    &(target.__data), $unwrap_handle(target_proc),
    desired_access, attr, opts
  );
}

template <win::__is_HANDLE Handle>
inline win::NtStatus duplicate_object(
 Handle source, Handle& target,
 win::AccessMask desired_access,
 win::ObjAttribMask attr,
 win::DupOptsMask opts
  = win::DupOptsMask::None
) {
  return duplicate_object_ex(
    source, current_process(),
    target, current_process(),
    desired_access, attr, opts
  );
}

} // inline namespace __nt
} // namespace hc::sys
