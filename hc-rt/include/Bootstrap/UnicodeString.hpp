//===- Bootstrap/UnicodeString.hpp ----------------------------------===//
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
//  Implementation of Window's UNICODE_STRING, but type-safe.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Fundamental.hpp>
#include <Common/Memory.hpp>
#include <Common/PtrRange.hpp>

namespace hc {
namespace common {
  template <typename> struct PtrRange;
} // namespace common

namespace bootstrap {
  struct [[gsl::Pointer]] Win64UnicodeString {
    u16 size = 0, size_max = 0; // In bytes
    wchar_t* buffer = nullptr;
  public:
    static Win64UnicodeString New(wchar_t* str);
    static Win64UnicodeString New(wchar_t* str, usize max);
    static Win64UnicodeString New(common::PtrRange<wchar_t> R);
    usize getSize() const { return size / sizeof(wchar_t); }
    usize getMaxSize() const { return size_max / sizeof(wchar_t); }
    bool isEqual(const Win64UnicodeString& rhs) const;
    com::PtrRange<wchar_t> intoRange() const;
    const wchar_t& frontSafe() const;
    const wchar_t& backSafe()  const;
  };

  template <usize N>
  struct [[gsl::Owner]] StaticUnicodeString : Win64UnicodeString {
    static constexpr u16 sizeMax = u16(N);
  public:
    constexpr StaticUnicodeString(u16 init_len = 0U) :
     Win64UnicodeString(
       init_len * sizeof(wchar_t), 
       sizeMax  * sizeof(wchar_t), __buffer)
    {
      __hc_invariant(init_len <= N);
    }

    constexpr StaticUnicodeString(wchar_t C, auto...CC) :
     StaticUnicodeString(u16(sizeof...(CC) + 1)), 
     __buffer{C, static_cast<wchar_t>(CC)...} {
      static_assert(sizeof...(CC) < N);
      // Win64UnicodeString::size = u16(sizeof...(CC) + 1);
    }

    constexpr StaticUnicodeString(char C, auto...CC) :
     StaticUnicodeString(static_cast<wchar_t>(C), CC...) {
      static_assert(sizeof...(CC) < N);
    }
    
    template <usize M>
    constexpr StaticUnicodeString(const wchar_t(&A)[M]) :
     StaticUnicodeString((!A[M - 1]) ? (M - 1) : M) {
      static_assert(M <= N);
      common::__array_memcpy(__buffer, A);
    }
  
  public:
    Win64UnicodeString& asBase() {
      return *static_cast<Win64UnicodeString*>(this);
    }
    const Win64UnicodeString& asBase() const {
      return *static_cast<const Win64UnicodeString*>(this);
    }

    Win64UnicodeString* operator->() {
      return static_cast<Win64UnicodeString*>(this);
    }
    const Win64UnicodeString* operator->() const {
      return static_cast<const Win64UnicodeString*>(this);
    }

  public:
    wchar_t __buffer[N] {};
  };

  template <usize N>
  StaticUnicodeString(const wchar_t(&)[N]) 
    -> StaticUnicodeString<N + 1>;

  template <typename T, typename...TT>
  StaticUnicodeString(T, TT...) 
    -> StaticUnicodeString<sizeof...(TT) + 2>;
} // namespace bootstrap
} // namespace hc
