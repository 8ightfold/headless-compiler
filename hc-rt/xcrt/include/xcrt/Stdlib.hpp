//===- xcrt/Stdlib.hpp ----------------------------------------------===//
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

#include <Common/Fundamental.hpp>

extern "C" {

using AtexitHandler = void(void);

int atexit(AtexitHandler* handler) noexcept;
int at_quick_exit(AtexitHandler* handler) noexcept;

[[noreturn, gnu::always_inline]]
inline void quick_abort() noexcept { __builtin_trap(); $unreachable; }
[[noreturn]] void abort() noexcept;
[[noreturn]] void _Exit(int status) noexcept;
[[noreturn]] void exit(int status) noexcept;
[[noreturn]] void quick_exit(int status) noexcept;

} // extern "C"

namespace xcrt {

using ::atexit;
using ::at_quick_exit;

using ::abort;
using ::quick_abort;
using ::_Exit;
using ::exit;
using ::quick_exit;

/// Returns the size of the atexit array.
usize atexit_total() noexcept;
/// Returns the number of open atexit slots.
usize atexit_slots() noexcept;

} // namespace xcrt
