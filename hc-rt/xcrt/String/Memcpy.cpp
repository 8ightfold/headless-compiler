//===- String/Memcpy.cpp --------------------------------------------===//
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

#include <Common/InlineMemcpy.hpp>

using namespace hc;

extern "C" {
  void* memcpy(
   void* __restrict __dst, 
   const void* __restrict __src, usize __len) {
    common::inline_memcpy(__dst, __src, __len);
    return __dst;
  }

  wchar_t* wmemcpy(
   wchar_t* __restrict __dst, 
   const wchar_t* __restrict __src, usize __len) {
    static constexpr usize __wcl = sizeof(wchar_t); 
    common::inline_memcpy(__dst, __src, __len * __wcl);
    return __dst;
  }
} // extern "C"
