//===- Sys/Args.hpp -------------------------------------------------===//
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
//  Get stuff like argv, envp, the program path, and working directory.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/PtrRange.hpp>

namespace hc::sys {

struct Args {
  template <typename T>
  using ArgType = com::ImmPtrRange<T>;
public:
  static ArgType<char*>   Argv();
  static ArgType<char*>   Envp();
  static ArgType<char>    ProgramDir();
  static ArgType<char>    WorkingDir();
#if HC_PLATFORM_WIN64
  static ArgType<wchar_t> ProgramDirW();
  static ArgType<wchar_t> WorkingDirW();
#endif
};

} // namespace hc::sys
