//===- Common/DynAlloc.hpp ------------------------------------------===//
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
//  This file defines the generic interface for stack allocation.
//  Because Clang currently adds stacksaves around inlined alloca calls,
//  it sadly has to be done via macros.
//
//===----------------------------------------------------------------===//

#pragma once

#include "_DynAlloc.hpp"
#include "Lifetime.hpp"
#include "Memory.hpp"
#include "PtrRange.hpp"

#if 0
/// Disabled as "[t]he __builtin_alloca_with_align function must be
/// called at block scope." For now it won't be used.
# define __hc_typed_alloca(sz, ty) \
  (typename ty::pointer)__builtin_alloca_with_align( \
   ty::AllocationSize(sz), ty::totalAlign)
#else
# ifndef __BIGGEST_ALIGNMENT__
#  error __BIGGEST_ALIGNMENT__ must be defined.
# endif // __BIGGEST_ALIGNMENT__

# define __hc_typed_alloca(sz, ty) \
  (typename ty::pointer) __builtin_alloca(ty::AllocationSize(sz))
#endif

#define __dynalloc(sz, ty...) ({ \
 using tyU__ = ::hc::common::DynAllocation<ty>; \
 auto* __local_alloc = __hc_typed_alloca(sz, tyU__); \
 tyU__::New(__local_alloc, sz); })

#define $dynalloc(sz, ty...) (__dynalloc(sz, ##ty).__ident())
#define $zdynalloc(sz, ty...) (__dynalloc(sz, ##ty).__zeroMemory())

// TODO: $to_wstr -> $widen
// Add $narrow, add SIMD?
#define $to_wstr_sz(S, size) ({ \
  const usize lenU__ = size; \
  auto wstrU__ = $dynalloc(lenU__ + 1, wchar_t); \
  for (usize IU__ = 0; IU__ < lenU__; ++IU__) \
    wstrU__[IU__] = static_cast<wchar_t>(S[IU__]); \
  wstrU__[lenU__] = L'\0'; \
  wstrU__; \
})

#define $to_wstr(S) $to_wstr_sz(S, __builtin_strlen(S))

namespace hc::common {
  template <typename T>
  concept __is_trivial_alloc = 
    __is_trivially_constructible(T) &&
    __is_trivially_destructible(T);
  
  template <typename T>
  using StackPtr = DynAllocation<T>;

  template <typename T, usize Align>
  struct [[gsl::Pointer]] DynAllocation {
    static_assert(Align <= __BIGGEST_ALIGNMENT__);
    using value_type = T;
    using pointer = T*;
    static constexpr usize totalAlign = Align * ::__bitcount;
  public:
    [[nodiscard, gnu::always_inline, gnu::const]]
    static usize AllocationSize(usize size) __noexcept {
      // Alloca sometimes releases memory when passed a 0.
      // We make sure this can't happen (disaster).
      const usize out_size = 
        __expect_false(size < 1) ? 1 : size;
      return out_size * __sizeof(T);
    }

    [[nodiscard, gnu::always_inline]]
    static DynAllocation New(T* data, usize size) __noexcept {
      return { data, uptr(size) };
    }

    operator PtrRange<T>() const __noexcept {
      return {begin(), end()};
    }

    template <meta::is_same<T> U = T>
    requires meta::not_const<T>
    operator ImmPtrRange<T>() const __noexcept {
      return {begin(), end()};
    }

    template <typename...Args>
    DynAllocation& init(Args&...args) __noexcept
     requires(__is_trivial_alloc<T>) {
      return *this;
    }

    template <typename...Args>
    DynAllocation& init(Args&...args) __noexcept
     requires(!__is_trivial_alloc<T>) {
      if __expect_true(this->__active)
        return *this;
      else if __expect_false(this->isEmpty())
        return *this;
      this->__active = true;
      for (T* I = begin(), *E = end(); I != E; ++I)
        common::construct_at(I, args...);
      return *this;
    }

    DynAllocation& zeroMemory() __noexcept {
      if constexpr (!__is_trivial_alloc<T>) {
        if __expect_false(this->__active) 
          return *this;
      }
      if __expect_false(this->isEmpty())
        return *this;
      Mem::VSet(data(), 0, this->sizeInBytes());
      return *this;
    }

    [[nodiscard]] T& operator[](usize I) const __noexcept {
      __hc_invariant(__data != nullptr && I < __size);
      return this->__data[I];
    }

    /// VERY UNSAFE!!! Only use when you know the size...
    template <meta::is_same<T> U = T>
    requires meta::is_trivially_destructible<U>
    [[nodiscard]] T* release() __noexcept {
      T* out = this->__data;
      this->__data = nullptr;
      this->__size = 0;
      if constexpr (__is_trivial_alloc<T>)
        this->__active = false;
      return out;
    }

    // Nodiscard lifters

    [[nodiscard, gnu::always_inline]]
    DynAllocation& __ident() __noexcept { return *this; }

    [[nodiscard]] DynAllocation& __zeroMemory() __noexcept {
      $tail_return this->zeroMemory();
    }

    //==================================================================//
    // Observers
    //==================================================================//

    [[nodiscard, gnu::const]]
    T* data() const __noexcept {
      return this->__data;
    }

    [[nodiscard, gnu::const]]
    usize size() const __noexcept {
      return this->__size;
    }

    [[nodiscard, gnu::const]]
    usize sizeInBytes() const __noexcept {
      return this->AllocationSize(size());
    }

    [[nodiscard, gnu::const]]
    PtrRange<T> intoRange() const __noexcept {
      return {begin(), end()};
    }

    template <typename U>
    [[nodiscard]] U into() const __noexcept {
      return U::New(begin(), end());
    }

    [[nodiscard, gnu::const]]
    T* begin() const __noexcept {
      return data();
    }

    [[nodiscard, gnu::const]]
    T* end() const __noexcept {
      return data() + size();
    }

    [[nodiscard, gnu::always_inline, gnu::const]]
    explicit operator bool() const __noexcept {
      return !!this->__data;
    }

    [[nodiscard, gnu::const]]
    bool isEmpty() const __noexcept {
      return this->__data == nullptr;
    }
  
  private:
    DynAllocation(T* data, uptr size) 
     : __data(data), __size(size) { }

  public:
    constexpr ~DynAllocation() 
     requires(__is_trivial_alloc<T>) = default;
    
    constexpr ~DynAllocation() __noexcept
     requires(!__is_trivial_alloc<T>) {
      if __expect_true(!this->isEmpty() && this->__active)
        __destroy(__data, __data + __size);
    }
  
  private:
    T* __data = nullptr;
    uptr __size : __bitsizeof(uptr) - 1 = 0;
    __prefer_type(bool) uptr __active : 1 = __is_trivial_alloc<T>;
  };
} // namespace hc::common
