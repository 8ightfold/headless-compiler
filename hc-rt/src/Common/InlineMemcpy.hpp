//===- Common/InlineMemcpy.hpp --------------------------------------===//
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
//  Underlying implementation of the memcpy equivelent.
//  Based on Google's automemcpy implementation.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Features.hpp>
#include "IAlign.hpp"
#include "Prefetching.hpp"

// TODO: Use prefetching/SIMD?

#define _HC_MEMCPY_FN(name) \
 static inline void name(u8* __restrict dst, \
  const u8* __restrict src, [[maybe_unused]] usize len = 0)

namespace hc::rt {
  template <usize BlockSize>
  _HC_MEMCPY_FN(__copy_block) {
    __builtin_memcpy_inline(dst, src, BlockSize);
  }

  template <usize BlockSize>
  _HC_MEMCPY_FN(__copy_last_block) {
    const usize off = len - BlockSize;
    $tail_return __copy_block<BlockSize>(dst + off, src + off, len);
  }

  template <usize BlockSize>
  _HC_MEMCPY_FN(__copy_overlap_block) {
    __copy_block<BlockSize>(dst, src);
    $tail_return __copy_last_block<BlockSize>(dst, src, len);
  }

  template <usize BlockSize, usize Align = BlockSize>
  _HC_MEMCPY_FN(__copy_aligned_blocks) {
    using namespace hc::common;
    // Check if Align = N^2.
    static_assert((Align & (Align - 1)) == 0);
    __copy_block<BlockSize>(dst, src);
    // Offset from Last Aligned.
    const usize offla = uptr(dst) & (Align - 1U);
    const usize limit = len + offla - BlockSize;
    for (usize off = BlockSize; off < limit; off += BlockSize)
      __copy_block<BlockSize>(
        __assume_aligned<BlockSize>(dst - offla + off), 
        src - offla + off);
    // Copy last block.
    $tail_return __copy_last_block<BlockSize>(dst, src, len);
  }

  _HC_MEMCPY_FN(__memcpy_small) {
    if (len == 1)
      $tail_return __copy_block<1>(dst, src, len);
    if (len == 2)
      $tail_return __copy_block<2>(dst, src, len);
    if (len == 3)
      $tail_return __copy_block<3>(dst, src, len);
    // else:
    $tail_return __copy_block<4>(dst, src, len);
  }

  [[gnu::always_inline]] _HC_MEMCPY_FN(__memcpy_dispatch) {
    if (len == 0)
      return;
    if (len < 5)
      $tail_return __memcpy_small(dst, src, len);
    if (len < 8)
      $tail_return __copy_overlap_block<4>(dst, src, len);
    if (len < 16)
      $tail_return __copy_overlap_block<8>(dst, src, len);
    if (len < 32)
      $tail_return __copy_overlap_block<16>(dst, src, len);
    if (len < 64)
      $tail_return __copy_overlap_block<32>(dst, src, len);
    if (len < 128)
      $tail_return __copy_overlap_block<64>(dst, src, len);
    // else:
    $tail_return __copy_aligned_blocks<32>(dst, src, len);
  }
} // namespace hc::rt

namespace hc::common {
  static inline void inline_memcpy(
   void* __restrict dst, const void* __restrict src, usize len) {
    __hc_invariant((dst && src) || !len);
    if __expect_false(len == 0)
      return;
    return rt::__memcpy_dispatch(
      (u8*)dst, (const u8*)src, len);
  }
} // namespace hc::common

#undef _HC_MEMCPY_FN
