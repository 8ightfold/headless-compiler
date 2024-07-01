//===- Sys/Win/Args.hpp ---------------------------------------------===//
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

#include <Sys/Args.hpp>
#include <Bootstrap/Win64KernelDefs.hpp>

using namespace hc;
using namespace hc::sys;
namespace B = hc::bootstrap;
namespace S = hc::sys;

extern "C" {
  // TODO: Prolly change this lol
  extern char*** __p___argv(void);
  extern char*** __imp___initenv;
} // extern "C"

namespace {
  B::Win64UnicodeString __get_program_path() {
    B::Win64PEB* PEB = B::Win64TEB::LoadPEBFromGS();
    auto* mods = PEB->getLDRModulesInMemOrder();
    return mods->prev()->fullName();
  }

  // https://github.com/wine-mirror/wine/blob/master/dlls/ntdll/path.c#L886
  B::Win64UnicodeString __get_working_path() {
    B::Win64PEB* PEB = B::Win64TEB::LoadPEBFromGS();
    return PEB->process_params->getCurrDir();
  }

  template <typename T>
  PtrRange<T*> __find_end(T** PP) {
    T** E = PP;
    while (*++E);
    return {PP, E};
  }
} // namespace `anonymous`

Args::ArgType<char*> Args::Argv() {
  return __find_end(*__p___argv());
}

Args::ArgType<char*> Args::Envp() {
  return __find_end(*__imp___initenv);
}

Args::ArgType<wchar_t> Args::ProgramDir() {
  static thread_local B::Win64UnicodeString S = __get_program_path();
  return S.intoImmRange();
}

Args::ArgType<wchar_t> Args::WorkingDir() {
  // Unlike the program path, this may change, so we cant cache it :(
  const B::Win64UnicodeString S = __get_working_path();
  return S.intoImmRange();
}
