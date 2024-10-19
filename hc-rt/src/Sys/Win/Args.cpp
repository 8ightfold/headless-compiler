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
#include <Common/ManualDrop.hpp>
#include <Meta/Once.hpp>
#include <Parcel/StaticVec.hpp>

using namespace hc;
using namespace hc::sys;

using PathStorage = pcl::StaticVec<char, RT_MAX_PATH + 1>;

#ifndef __XCRT__
extern "C" {
// TODO: Prolly change this lol
extern char*** __p___argv(void);
extern char*** __imp___initenv;
} // extern "C"
#endif // !__XCRT__

namespace {

__imut ManualDrop<PathStorage> program_dir {};
__imut ManualDrop<PathStorage> working_dir {};

boot::UnicodeString __get_program_path() {
  auto* mods = boot::HcCurrentPEB()->getLDRModulesInMemOrder();
  return mods->prev()->fullName();
}

// https://github.com/wine-mirror/wine/blob/master/dlls/ntdll/path.c#L886
boot::UnicodeString __get_working_path() {
  return boot::HcCurrentPEB()->process_params->getCurrDir();
}

template <typename T> PtrRange<T*> __find_end(T** PP) {
  T** E = PP;
  while (*++E);
  return {PP, E};
}

[[gnu::noinline]]
bool __init_filename(PathStorage& P, ImmPtrRange<wchar_t> str) {
  const usize base_size = str.size();
  if __expect_false(base_size + 1 >= P.Capacity())
    return false;
  
  __hc_assertOrIdent(
    P.resizeUninit(base_size + 1));
  for (usize Ix = 0; Ix < base_size; ++Ix)
    P[Ix] = static_cast<char>(str[Ix]);
  P[base_size] = '\0';

  return true;
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

Args::ArgType<char> Args::ProgramDir() {
  return program_dir->intoImmRange();
}

Args::ArgType<char> Args::WorkingDir() {
  return working_dir->intoImmRange();
}

Args::ArgType<wchar_t> Args::ProgramDirW() {
  static boot::UnicodeString S = __get_program_path();
  return S.intoImmRange();
}

Args::ArgType<wchar_t> Args::WorkingDirW() {
  // Unlike the program path, this may change, so we cant cache it :(
  const boot::UnicodeString S = __get_working_path();
  return S.intoImmRange();
}

//////////////////////////////////////////////////////////////////////////

namespace hc::sys {

void __init_paths(void) {
  if (program_dir->isEmpty())
    __init_filename(program_dir.unwrap(), Args::ProgramDirW());
  if (working_dir->isEmpty())
    __init_filename(working_dir.unwrap(), Args::WorkingDirW());
}

#ifndef __XCRT__
  $Once { __init_paths(); };
#endif // __XCRT__?

} // namespace hc::sys
