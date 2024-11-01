//===- Memory/Box.hpp -----------------------------------------------===//
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

#include <__config.inc>
#include <Common/Casting.hpp>
#include <Std/new>

namespace XCRT_NAMESPACE {

enum HeapAllocFlags : u32 {
  HA_None         = 0x0,
  HA_NoSerialize  = 0x1,
  HA_Exceptions   = 0x4,
  HA_ZeroMemory   = 0x8,
};

void* box_heap_alloc(usize size, HeapAllocFlags flags = HA_Exceptions);
bool box_heap_free(void* ptr, bool no_serialize = false);

template <typename T>
__nodebug inline T* box_new(usize extra = 0, auto&&...args) {
  void* P = box_heap_alloc(sizeof(T) + extra);
  if ((uptr(P) & alignof(T)) != 0U)
    $unreachable_msg("Insufficient alignment!");
  return new(P) T { __hc_fwd(args)... };
}

template <typename T>
inline void box_delete(T* P) {
  if (!P) return;
  P->~T();
  box_heap_free(P);
}

// TODO: In the future, use a standard rt alias.
template <typename T> struct Box {
  constexpr Box() = default;
  constexpr Box(std::nullptr_t) : Box() {}

  explicit Box(T* P) : data(P) {}
  
  Box(const Box&) = delete;
  Box(Box&& rhs) noexcept : Box(rhs.release()) {}

  Box& operator=(const Box&) = delete;
  Box& operator=(Box&& rhs) noexcept {
    this->clear();
    this->data = rhs.release();
    return *this;
  }

  ~Box() { box_delete(this->data); }

public:
  static Box New(auto&&...args) {
    T* P = box_new<T>(0, __hc_fwd(args)...);
    return Box(P);
  }

  static Box NewExtra(usize N, auto&&...args) {
    const usize extra = T::ExtraSize(N);
    T* P = box_new<T>(extra, __hc_fwd(args)...);
    return Box(P);
  }

public:
  T* get() const noexcept {
    return this->data;
  }

  T* release() noexcept {
    T* out = data;
    this->data = nullptr;
    return out;
  }

  void clear() noexcept {
    box_delete(this->release());
  }

  T& operator*()& {
    __hc_invariant(data != nullptr);
    return *get();
  }

  const T& operator*() const& {
    __hc_invariant(data != nullptr);
    return *get();
  }

  T* operator->()& {
    __hc_invariant(data != nullptr);
    return get();
  }

  const T* operator->() const& {
    __hc_invariant(data != nullptr);
    return get();
  }

  explicit operator bool() const {
    return !!this->data;
  }

private:
  T* data = nullptr;
};

template <typename T> struct Box<T[]> {
  $compile_failure(T,
    "Arrays are currently unsupported.");
};

//////////////////////////////////////////////////////////////////////////
// Setup

bool setup_heap_funcs();

} // namespace XCRT_NAMESPACE
