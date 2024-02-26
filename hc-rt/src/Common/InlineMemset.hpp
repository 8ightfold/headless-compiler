//===- Common/InlineMemset.hpp --------------------------------------===//
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
//  Underlying implementation of the memset equivelent.
//  Adapted from LLVM's inline_memset_x86 implementation.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Array.hpp>
#include "IAlign.hpp"
#include "Prefetching.hpp"

#define _HC_MEMSET_FN(name) \
 inline void name(u8* __restrict dst, \
  u8 val, [[maybe_unused]] usize len = 0)

namespace hcrt {
  static constexpr bool __memset_prefetch_ = _HC_SOFTWARE_PREFETCH;

  template <typename BlockType>
  _HC_MEMSET_FN(__set_block) {
    store<BlockType>(dst, 
      splat<BlockType>(val));
  }

  template <typename BlockType, typename...Next>
  _HC_MEMSET_FN(__set_block_seq) {
    __set_block<BlockType>(dst, val);
    if constexpr (sizeof...(Next) > 0)
      $tail_return __set_block_seq<Next...>(
        dst + sizeof(BlockType), val, len);
  }

  template <typename BlockType>
  _HC_MEMSET_FN(__set_last_block) {
    $tail_return __set_block<BlockType>(
      dst + len - sizeof(BlockType), val);
  }

  template <typename BlockType>
  _HC_MEMSET_FN(__set_first_last_block) {
    __set_block<BlockType>(dst, val);
    $tail_return __set_last_block<BlockType>(dst, val, len);
  }

  template <typename BlockType>
  inline void __set_loop_and_last_off(
   u8* __restrict dst, u8 val, usize len, usize off) {
    do {
      __set_block<BlockType>(dst + off, val);
      off += sizeof(BlockType);
    } while(off < len - sizeof(BlockType));
    __set_last_block<BlockType>(dst, val, len);
  }

  template <typename BlockType>
  [[gnu::flatten]]
  _HC_MEMSET_FN(__set_loop_and_last) {
    return __set_loop_and_last_off<BlockType>(dst, val, len, 0);
  }

  [[maybe_unused]] _HC_MEMSET_FN(__prefetching_memset) {
    static constexpr usize prefetchDistance = cacheLinesSize<5>;
    static constexpr usize prefetchDegree = cacheLinesSize<2>;
    static constexpr usize kSize = sizeof(v512);
    // Prefetch a single cache line
    smart_prefetch<PrefetchMode::Write>(dst + cacheLinesSize<1>);
    if (len <= 128)
      $tail_return __set_first_last_block<v512>(dst, val, len);
    smart_prefetch<PrefetchMode::Write>(dst + cacheLinesSize<2>);
    __set_block<v512>(dst, val);
    align_to_next_boundary<32>(dst, len);
    if (len <= 192) {
      $tail_return __set_loop_and_last<v256>(dst, val, len);
    } else {
      __set_block_seq<v512, v256>(dst, val);
      usize off = 96;
      while (off + prefetchDegree + kSize <= len) {
        smart_prefetch<PrefetchMode::Write>(dst + off + prefetchDistance);
        smart_prefetch<PrefetchMode::Write>(dst + off + prefetchDistance + cacheLinesSize<1>);
        for (usize I = 0; I < prefetchDegree; I += kSize, off += kSize)
          __set_block<v256>(dst + off, val);
      }
      __set_loop_and_last_off<v256>(dst, val, len, off);
    }
  }

  [[gnu::always_inline]] _HC_MEMSET_FN(__memset_small) {
    if (len == 1)
      $tail_return __set_block<u8>(dst, val, len);
    if (len == 2)
      $tail_return __set_block<u16>(dst, val, len);
    if (len == 3)
      $tail_return __set_block_seq<u16, u8>(dst, val, len);
    // else:
    $tail_return __set_block<u32>(dst, val, len);
  }

  [[gnu::always_inline]] static _HC_MEMSET_FN(__memset_dispatch) {
    if (len == 0)
      return;
    if (len < 5)
      $tail_return __memset_small(dst, val, len);
    if (len <= 8)
      $tail_return __set_first_last_block<u32>(dst, val, len);
    if (len <= 16)
      $tail_return __set_first_last_block<u64>(dst, val, len);
    if (len <= 32)
      $tail_return __set_first_last_block<v128>(dst, val, len);
    if (len <= 64)
      $tail_return __set_first_last_block<v256>(dst, val, len);
    if constexpr (__memset_prefetch_)
      $tail_return __prefetching_memset(dst, val, len);
    if (len <= 128)
      $tail_return __set_first_last_block<v512>(dst, val, len);
    // else:
    __set_block<v256>(dst, val);
    align_to_next_boundary<32>(dst, len);
    $tail_return __set_loop_and_last<v256>(dst, val, len);
  }
} // namespace hcrt

namespace hc::common {
  static inline void inline_memset(void* dst, u8 val, usize len) {
    __hc_invariant(dst || !len);
    if __expect_false(len == 0)
      return;
    return rt::__memset_dispatch(
      (u8*)dst, val, len);
  }
} // namespace hc::common

#undef _HC_MEMSET_FN