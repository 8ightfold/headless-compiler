//===- Phase1/Initialization.hpp ------------------------------------===//
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
#include <Locks.hpp>

extern "C" {
  using VVFunc = void(*)(void);
  using IVFunc = int(*)(void);
  using VIFunc = void(*)(int);

  extern i32  __xcrt_atexit(VVFunc);
  extern void __xcrt_invoke_atexit(void);

  extern u64  __xcrt_locks_setup(void);
  extern void __xcrt_sysio_setup(void);
  extern void __xcrt_emutils_setup(void);

  extern void __xcrt_locks_shutdown(void);
  extern void __xcrt_sysio_shutdown(void);
  extern void __xcrt_emutils_shutdown(void);
}
