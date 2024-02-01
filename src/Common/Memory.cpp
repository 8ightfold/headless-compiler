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

#include <Common/Features.hpp>
#include <Common/Casting.hpp>
#include <Common/Memory.hpp>

// TODO: Use prefetching/SIMD?

#undef _HC_MEMCPY_FN
#undef _HC_COPY_BLOCK

#define _HC_MEMCPY_FN(name) \
 void name(char* __restrict dst, \
  const char* __restrict src, usize len = 0)
#define _HC_COPY_BLOCK(size, args...) \
 __copy_block<size>(args, len)

using namespace hc;
namespace C = ::hc::common;

//=== Memcpy Backend ===//
namespace {
  template <usize BlockSize>
  _HC_MEMCPY_FN(__copy_block) {
    __builtin_memcpy_inline(dst, src, BlockSize);
    (void)len;
  }

  template <usize BlockSize>
  _HC_MEMCPY_FN(__copy_last_block) {
    const usize off = len - BlockSize;
    $tail_return _HC_COPY_BLOCK(BlockSize, dst + off, src + off);
  }

  template <usize BlockSize>
  _HC_MEMCPY_FN(__copy_overlap_block) {
    _HC_COPY_BLOCK(BlockSize, dst, src);
    $tail_return __copy_last_block<BlockSize>(dst, src, len);
  }

  template <usize BlockSize, usize Align = BlockSize>
  _HC_MEMCPY_FN(__copy_aligned_blocks) {
    // Check if Align = N^2.
    static_assert((Align & (Align - 1)) == 0);
    __copy_block<BlockSize>(dst, src);
    // Offset from Last Aligned.
    const usize offla = uptr(dst) & (Align - 1U);
    const usize limit = len + offla - BlockSize;
    for (usize off = BlockSize; off < limit; off += BlockSize)
      __copy_block<BlockSize>(
        C::__assume_aligned<BlockSize>(dst - offla + off), 
        src - offla + off);
    // Copy last block.
    $tail_return __copy_last_block<BlockSize>(dst, src, len);
  }

  _HC_MEMCPY_FN(__memcpy_small) {
    if(len == 1)
      $tail_return _HC_COPY_BLOCK(1, dst, src);
    if(len == 2)
      $tail_return _HC_COPY_BLOCK(2, dst, src);
    if(len == 3)
      $tail_return _HC_COPY_BLOCK(3, dst, src);
    // else:
    $tail_return _HC_COPY_BLOCK(4, dst, src);
  }

  __always_inline _HC_MEMCPY_FN(__memcpy_dispatch) {
    if(len == 0)
      return;
    if(len < 5)
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
} // namespace `anonymous`

//=== Memset Backend ===//
namespace {
  // TODO: Add this
} // namespace `anonymous`

void* C::Mem::VCopy(void* __restrict dst, const void* __restrict src, usize len) {
  if __expect_false(!dst || !src) return nullptr;
  __memcpy_dispatch(ptr_cast<char>(dst), ptr_cast<const char>(src), len);
  return dst;
}

void* C::Mem::VSet(void* dst, int ch, usize len) {
  if __expect_false(!dst) return nullptr;
  return dst;
}
