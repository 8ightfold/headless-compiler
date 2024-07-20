//===- Sys/Atomic.hpp -----------------------------------------------===//
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
#include <Meta/Traits.hpp>

// For more info:
// https://www.cl.cam.ac.uk/~pes20/cpp/popl085ap-sewell.pdf
// https://www.cl.cam.ac.uk/~pes20/weakmemory/cacm.pdf


namespace hc::sys {
  enum class MemoryOrder : int {
    Relaxed = __ATOMIC_RELAXED,
    /* Consume != __ATOMIC_CONSUME, */
    Consume = __ATOMIC_ACQUIRE,
    Acquire = __ATOMIC_ACQUIRE,
    Release = __ATOMIC_RELEASE,
    AcqRels = __ATOMIC_ACQ_REL,
    SeqCnst = __ATOMIC_SEQ_CST,
  };

  template <typename T>
  struct [[clang::trivial_abi]] Atomic {
    static_assert(meta::is_arithmetic<T>);
    using enum MemoryOrder;
  private:
    static constexpr usize alignVal =
      (alignof(T) > sizeof(T)) ? alignof(T) : sizeof(T);
  
  public:
    constexpr Atomic() = default;
    __always_inline constexpr Atomic(T V) : data(V) {}

    Atomic(const Atomic&) = delete;
    Atomic& operator=(const Atomic&) = delete;

    __always_inline constexpr static bool IsAlwaysLockFree() {
      return __atomic_always_lock_free(sizeof(T), 0);
    }
    
    __always_inline bool isLockFree() const {
      return __atomic_is_lock_free(sizeof(T), &data);
    }

    /// A non-atomic variant of `.load()`.
    constexpr T get() const { return this->data; }
    /// A non-atomic variant of `.store(...)`.
    constexpr T set(T V) { return (this->data = V); }

    //==================================================================//
    // Operators
    //==================================================================//
  
    __always_inline operator T() {
      return __atomic_load_n(&data, __ATOMIC_SEQ_CST);
    }

    __always_inline T operator=(T V) {
      return __atomic_store_n(&data, V, __ATOMIC_SEQ_CST);
    }

    __always_inline T operator+=(T V) {
      return __atomic_add_fetch(&data, V, __ATOMIC_SEQ_CST);
    }

    __always_inline T operator-=(T V) {
      return __atomic_sub_fetch(&data, V, __ATOMIC_SEQ_CST);
    }

    __always_inline T operator++() {
      return __atomic_add_fetch(&data, 1, __ATOMIC_SEQ_CST);
    }

    __always_inline T operator--() {
      return __atomic_sub_fetch(&data, 1, __ATOMIC_SEQ_CST);
    }

    __always_inline T operator++(int) {
      return __atomic_fetch_add(&data, 1, __ATOMIC_SEQ_CST);
    }

    __always_inline T operator--(int) {
      return __atomic_fetch_sub(&data, 1, __ATOMIC_SEQ_CST);
    }

    //==================================================================//
    // Memory Ordered
    //==================================================================//

    __always_inline T add(T V, MemoryOrder ord = SeqCnst) {
      return __atomic_add_fetch(&data, V, int(ord));
    }

    __always_inline T sub(T V, MemoryOrder ord = SeqCnst) {
      return __atomic_sub_fetch(&data, V, int(ord));
    }

    __always_inline T inc(MemoryOrder ord = SeqCnst) {
      return __atomic_add_fetch(&data, 1, int(ord));
    }

    __always_inline T dec(MemoryOrder ord = SeqCnst) {
      return __atomic_sub_fetch(&data, 1, int(ord));
    }

    __always_inline T andFetch(T V, MemoryOrder ord = SeqCnst) {
      return __atomic_and_fetch(&data, V, int(ord));
    }

    __always_inline T orFetch(T V, MemoryOrder ord = SeqCnst) {
      return __atomic_or_fetch(&data, V, int(ord));
    }

    __always_inline T xorFetch(T V, MemoryOrder ord = SeqCnst) {
      return __atomic_xor_fetch(&data, V, int(ord));
    }

    __always_inline T nandFetch(T V, MemoryOrder ord = SeqCnst) {
      return __atomic_nand_fetch(&data, V, int(ord));
    }

    //////////////////////////////////////////////////////////////////////

    __always_inline T fetchAdd(T V, MemoryOrder ord = SeqCnst) {
      return __atomic_fetch_add(&data, V, int(ord));
    }

    __always_inline T fetchSub(T V, MemoryOrder ord = SeqCnst) {
      return __atomic_fetch_sub(&data, V, int(ord));
    }

    __always_inline T fetchInc(MemoryOrder ord = SeqCnst) {
      return __atomic_fetch_add(&data, 1, int(ord));
    }

    __always_inline T fetchDec(MemoryOrder ord = SeqCnst) {
      return __atomic_fetch_sub(&data, 1, int(ord));
    }

    __always_inline T fetchAnd(T V, MemoryOrder ord = SeqCnst) {
      return __atomic_fetch_and(&data, V, int(ord));
    }

    __always_inline T fetchOr(T V, MemoryOrder ord = SeqCnst) {
      return __atomic_fetch_or(&data, V, int(ord));
    }

    __always_inline T fetchXor(T V, MemoryOrder ord = SeqCnst) {
      return __atomic_fetch_xor(&data, V, int(ord));
    }

    __always_inline T fetchNand(T V, MemoryOrder ord = SeqCnst) {
      return __atomic_fetch_nand(&data, V, int(ord));
    }

    inline T fetchMax(T rhs, MemoryOrder ord = SeqCnst) {
      const T lhs  = __atomic_load_n(&data, int(ord));
      const T nmax = (lhs > rhs) ? lhs : rhs;
      return __atomic_exchange_n(&data, nmax, int(ord));
    }

    inline T fetchMin(T rhs, MemoryOrder ord = SeqCnst) {
      const T lhs  = __atomic_load_n(&data, int(ord));
      const T nmin = (lhs < rhs) ? lhs : rhs;
      return __atomic_exchange_n(&data, nmin, int(ord));
    }

    //////////////////////////////////////////////////////////////////////

    [[nodiscard]] __always_inline T load(MemoryOrder ord = SeqCnst) {
      return __atomic_load_n(&data, int(ord));
    }

    __always_inline T store(T V, MemoryOrder ord = SeqCnst) {
      return __atomic_store_n(&data, V, int(ord));
    }

    [[nodiscard]] __always_inline T xchg(T V, MemoryOrder ord = SeqCnst) {
      return __atomic_exchange_n(&data, V, int(ord));
    }

    [[nodiscard]] __always_inline T cmpxchg(
     T& expected, T V, MemoryOrder ord = SeqCnst) {
      return __atomic_compare_exchange_n(
        &data, &expected, V, false, int(ord), int(ord));
    }

    [[nodiscard]] __always_inline T cmpxchg(
     T& expected, T V, MemoryOrder write_ord, MemoryOrder read_ord) {
      return __atomic_compare_exchange_n(
        &data, &expected, V, false,
        int(write_ord), int(read_ord));
    }

  public:
    alignas(alignVal) T data {};
  };

  inline void atomic_thread_fence(MemoryOrder ord) {
    __atomic_thread_fence(int(ord));
  }

  inline void atomic_signal_fence([[maybe_unused]] MemoryOrder ord) {
#if __has_builtin(__atomic_signal_fence)
    __atomic_signal_fence(int(ord));
#else
    __asm__ volatile ("" ::: "memory");
#endif
  }
} // namespace hc::sys
