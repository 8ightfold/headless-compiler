//===- String/XStrcmp.cpp -------------------------------------------===//
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
  int strcmp(const char* __lhs, const char* __rhs) {
    const auto __cmp = [](char L, char R) -> i32 { return L - R; };
    return xcrt::xstrcmp(__lhs, __rhs, __cmp);
  }

  int wcscmp(const wchar_t* __lhs, const wchar_t* __rhs) {
    const auto __cmp = [](wchar_t L, wchar_t R) -> i32 { return L - R; };
    return xcrt::xstrcmp(__lhs, __rhs, __cmp);
  }
} // extern "C"
