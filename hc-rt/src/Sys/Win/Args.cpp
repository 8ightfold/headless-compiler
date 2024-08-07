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
#include <Bootstrap/_NtModule.hpp>

using namespace hc;
using namespace hc::sys;
namespace B = hc::bootstrap;
namespace S = hc::sys;

#ifndef __XCRT__
extern "C" {
// TODO: Prolly change this lol
extern char*** __p___argv(void);
extern char*** __imp___initenv;
} // extern "C"
#endif // !__XCRT__

namespace {
  B::UnicodeString __get_program_path() {
    auto* mods = boot::HcCurrentPEB()->getLDRModulesInMemOrder();
    return mods->prev()->fullName();
  }

  // https://github.com/wine-mirror/wine/blob/master/dlls/ntdll/path.c#L886
  B::UnicodeString __get_working_path() {
    return boot::HcCurrentPEB()->process_params->getCurrDir();
  }

  template <typename T>
  PtrRange<T*> __find_end(T** PP) {
    T** E = PP;
    while (*++E);
    return {PP, E};
  }
} // namespace `anonymous`

#ifdef __XCRT__
# define __find_end(...) {}
#endif // __XCRT__

Args::ArgType<char*> Args::Argv() {
  return __find_end(*__p___argv());
}

Args::ArgType<char*> Args::Envp() {
  return __find_end(*__imp___initenv);
}

Args::ArgType<wchar_t> Args::ProgramDir() {
  static thread_local B::UnicodeString S = __get_program_path();
  return S.intoImmRange();
}

Args::ArgType<wchar_t> Args::WorkingDir() {
  // Unlike the program path, this may change, so we cant cache it :(
  const B::UnicodeString S = __get_working_path();
  return S.intoImmRange();
}
