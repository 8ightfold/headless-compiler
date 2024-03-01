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

#pragma once

#include <Common/Features.hpp>
#include "IAlign.hpp"
#include "MemUtils.hpp"

#define _HC_MEMCMP_FN(name) \
 inline i32 name(const u8* lhs, \
  const u8* rhs, [[maybe_unused]] usize len = 0)

namespace hc::rt {
  template <typename BlockType>
  _HC_MEMCMP_FN(__cmp_block_off) {
    if constexpr (__cmp_is_expensive<BlockType>) {
      if (!eq<BlockType>(lhs, rhs, len))
        $tail_return cmp_neq<BlockType>(lhs, rhs, len);
      return 0;
    } else {
      $tail_return cmp_eq<BlockType>(lhs, rhs, len);
    }
  }

  template <typename BlockType>
  _HC_MEMCMP_FN(__cmp_block) {
    $tail_return __cmp_block_off<BlockType>(lhs, rhs, 0);
  }

  template <typename BlockType, typename...Tails>
  _HC_MEMCMP_FN(__cmp_block_seq) {
    if constexpr (__cmp_is_expensive<BlockType>) {
      if (!eq<BlockType>(lhs, rhs, 0))
        $tail_return cmp_neq<BlockType>(lhs, rhs, 0);
      return 0;
    } else {
      if (const auto V = cmp_eq<BlockType>(lhs, rhs, 0))
        return V;
    }

    if constexpr (sizeof...(Tails) != 0) {
      $tail_return __cmp_block_seq<Tails...>(
        lhs + sizeof(BlockType), rhs + sizeof(BlockType));
    } else {
      return 0;
    }
  }

  template <typename BlockType>
  _HC_MEMCMP_FN(__cmp_last_block) {
    $tail_return __cmp_block_off<BlockType>(
      lhs, rhs, len - sizeof(BlockType));
  }

  template <typename BlockType>
  _HC_MEMCMP_FN(__cmp_first_last_block) {
    if constexpr (__cmp_is_expensive<BlockType>) {
      if (!eq<BlockType>(lhs, rhs, 0))
        $tail_return cmp_neq<BlockType>(lhs, rhs, 0);
    } else {
      if (const auto V = cmp_eq<BlockType>(lhs, rhs, 0))
        return V;
    }
    $tail_return __cmp_last_block<BlockType>(lhs, rhs, len);
  }

  template <typename BlockType>
  inline i32 __cmp_loop_and_last_off(
   const u8* lhs, const u8* rhs, usize len, usize off) {
    if constexpr (sizeof(BlockType) > 1) {
      const usize limit = len - sizeof(BlockType);
      for (; off < limit; off += sizeof(BlockType)) {
        if constexpr (__cmp_is_expensive<BlockType>) {
          if (!eq<BlockType>(lhs, rhs, off))
            return cmp_neq<BlockType>(lhs, rhs, off);
        } else {
          if (const auto V = cmp<BlockType>(lhs, rhs, off))
            return V;
        }
      }
      return __cmp_block_off<BlockType>(lhs, rhs, limit);
    } else {
      for (; off < len; ++off)
        if (auto V = cmp<BlockType>(lhs, rhs, off))
          return V;
      return 0;
    }
  }

  template <typename BlockType>
  _HC_MEMCMP_FN(__cmp_loop_and_last) {
    return __cmp_loop_and_last_off<BlockType>(lhs, rhs, len, 0);
  }

  template <typename BlockType, usize Threshold = 384>
  _HC_MEMCMP_FN(__cmp_loop_and_last_align) {
    const auto align = offset_from_last_align<sizeof(BlockType)>(lhs);
    if (__expect_false(len >= Threshold) && (align != sizeof(BlockType))) {
      if (const auto V = __cmp_block<BlockType>(lhs, rhs))
        return V;
      adjust_nr(align, lhs, rhs, len);
    }
    $tail_return __cmp_loop_and_last<BlockType>(lhs, rhs, len);
  }

  //====================================================================//
  // Implementation
  //====================================================================//

  [[maybe_unused]] _HC_MEMCMP_FN(__memcmp_large_gen) {
    $tail_return __cmp_loop_and_last_align<u64>(lhs, rhs, len);
  }

  #if defined(__SSE4_1__)
  [[maybe_unused]] _HC_MEMCMP_FN(__memcmp_large_sse4_1) {
    $tail_return __cmp_loop_and_last_align<Gmi128>(lhs, rhs, len);
  }
  #endif // __SSE4_1__

  [[gnu::always_inline]] static _HC_MEMCMP_FN(__memcmp_dispatch) {
    if (len == 0)
      return 0;
    if (len == 1)
      $tail_return __cmp_block<u8>(lhs, rhs);
    if (len == 2)
      $tail_return __cmp_block<u16>(lhs, rhs);
    if (len == 3)
      $tail_return __cmp_block_seq<u16, u8>(lhs, rhs);
    if (len == 4)
      $tail_return __cmp_block<u32>(lhs, rhs);
    if (len == 5)
      $tail_return __cmp_block_seq<u32, u8>(lhs, rhs);
    if (len == 6)
      $tail_return __cmp_block_seq<u32, u16>(lhs, rhs);
    if (len == 7)
      $tail_return __cmp_first_last_block<u32>(lhs, rhs, 7);
    if (len == 8)
      $tail_return __cmp_block<u64>(lhs, rhs);
    if (len <= 16)
      $tail_return __cmp_first_last_block<u64>(lhs, rhs, len);
  #if defined(__SSE4_1__)
    $tail_return __memcmp_large_sse4_1(lhs, rhs, len);
  #else
    $tail_return __memcmp_large_gen(lhs, rhs, len);
  #endif
  }
} // namespace hc::rt

namespace hc::common {
  static inline i32 inline_memcmp(
   const void* lhs, const void* rhs, usize len) {
    __hc_invariant((lhs && rhs) || !len);
    if __expect_false(len == 0)
      return 0;
    return rt::__memcmp_dispatch(
      (const u8*)lhs, (const u8*)rhs, len);
  }
} // namespace hc::common

#undef _HC_MEMCMP_FN
