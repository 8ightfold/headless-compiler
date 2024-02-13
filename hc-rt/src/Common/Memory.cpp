//===- Common/Memory.cpp --------------------------------------------===//
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
//  Underlying implementation of the memcpy/memset equivelents.
//  Based on Googles' automemcpy implementation.
//
//===----------------------------------------------------------------===//

#include "InlineMemcpy.hpp"
#include "InlineMemset.hpp"
#include <Common/Casting.hpp>

using namespace hc;
using namespace hc::common;
namespace C = hc::common;

void* C::Mem::VCopy(void* __restrict dst, const void* __restrict src, usize len) {
  if __expect_false(!dst || !src) return nullptr;
  rt::__memcpy_dispatch(
    ptr_cast<u8>(dst), 
    ptr_cast<const u8>(src), 
    len
  );
  return dst;
}

void* C::Mem::VSet(void* __restrict dst, int ch, usize len) {
  if __expect_false(!dst) return nullptr;
  rt::__memset_dispatch(
    ptr_cast<u8>(dst), 
    static_cast<u8>(ch), 
    len
  );
  return dst;
}
