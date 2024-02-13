//===- BuiltinOverrides.hpp -----------------------------------------===//
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

#include <Common/Casting.hpp>
#include <Common/InlineMemcpy.hpp>
#include <Common/InlineMemset.hpp>
#include <Common/Strings.hpp>

using namespace hc;

extern "C" {
  void* memcpy(void* __restrict __dst, const void* __restrict __src, usize __len) {
    rt::__memcpy_dispatch(
      ptr_cast<u8>(__dst), 
      ptr_cast<const u8>(__src), 
      __len
    );
    return __dst;
  }

  void* memset(void* __restrict __dst, int __ch, usize __len) {
    rt::__memset_dispatch(
      ptr_cast<u8>(__dst), 
      static_cast<u8>(__ch), 
      __len
    );
    return __dst;
  }

  [[gnu::flatten]] usize wcslen(const wchar_t* __str) {
    $tail_return common::__wstrlen(__str);
  }

  [[gnu::flatten]] int wcsncmp(
   const wchar_t* __lhs, const wchar_t* __rhs, usize __len) {
    $tail_return common::__wstrncmp(__lhs, __rhs, __len);
  }
} // extern "C"