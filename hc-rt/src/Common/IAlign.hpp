//===- Common/IAlign.hpp --------------------------------------------===//
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

#pragma once

#include <Common/Fundamental.hpp>
#include <Common/Memory.hpp>

namespace hcrt {
  template <usize Align>
  inline uptr offset_from_last_align(const void* ptr) {
    static_assert((Align & (Align - 1)) == 0,
      "Align must be a power of 2");
    return Align - (uptr(ptr) & (Align - 1U));
  }

  template <typename T, typename U>
  inline void adjust(uptrdiff offset, 
   T* __restrict& t, U* __restrict& u, usize& len) {
    t += offset;
    u += offset;
    len -= offset;
  }

  template <usize Align, typename T>
  inline void align_to_next_boundary(T* __restrict& t, usize& len) {
    const T* og = t;
    adjust(offset_from_last_align<Align>(t), t, og, len);
    t = hc::common::__assume_aligned<Align>(t);
  }
} // namespace hcrt