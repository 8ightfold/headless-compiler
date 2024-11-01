//===- String/XStrstr.cpp -------------------------------------------===//
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

#include "Utils.hpp"

using namespace hc;

extern "C" {
  char* strstr(char* __str, const char* __substr) {
    const usize __len = xcrt::stringlen(__str);
    return xcrt::find_first_str(__str, __substr, __len);
  }

  wchar_t* wcsstr(wchar_t* __str, const wchar_t* __substr) {
    const usize __len = xcrt::wstringlen(__str);
    return xcrt::wfind_first_str(__str, __substr, __len);
  }
} // extern "C"
