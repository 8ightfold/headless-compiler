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
  [[gnu::flatten]] int wcsncmp(
   const wchar_t* __lhs, const wchar_t* __rhs, usize __len) {
    $tail_return common::__wstrncmp(__lhs, __rhs, __len);
  }
} // extern "C"