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
#pragma clang system_header

#include <Common/Align.hpp>
#include <Common/Fundamental.hpp>
#include <Common/Memory.hpp>

namespace hc {
namespace common {
template <typename> struct PtrRange;
} // namespace common

namespace bootstrap {

struct [[gsl::Pointer]] UnicodeString {
  u16 __size = 0, __size_max = 0; // In bytes
  wchar_t* buffer = nullptr;
public:
  [[nodiscard]] static UnicodeString New(wchar_t* str);
  [[nodiscard]] static UnicodeString New(wchar_t* str, usize max);
  [[nodiscard]] static UnicodeString New(com::PtrRange<wchar_t> R);
  usize getSize() const { return __size / sizeof(wchar_t); }
  usize getMaxSize() const { return __size_max / sizeof(wchar_t); }
  bool isEqual(const UnicodeString& rhs) const;
  com::PtrRange<const wchar_t> intoImmRange() const;
  com::PtrRange<wchar_t> intoRange() const;
  const wchar_t& frontSafe() const;
  const wchar_t& backSafe()  const;
};

template <usize N>
struct [[gsl::Owner]] StaticUnicodeString : UnicodeString {
  static constexpr u16 sizeMax = u16(N);
public:
  constexpr StaticUnicodeString(u16 init_len = 0U) :
   UnicodeString(
     init_len * sizeof(wchar_t), 
     sizeMax  * sizeof(wchar_t), __buffer)
  {
    __hc_invariant(init_len <= N);
  }

  constexpr StaticUnicodeString(wchar_t C, auto...CC) :
   StaticUnicodeString(u16(sizeof...(CC) + 1)), 
   __buffer{C, static_cast<wchar_t>(CC)...} {
    static_assert(sizeof...(CC) < N);
    // UnicodeString::size = u16(sizeof...(CC) + 1);
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
  UnicodeString& asBase() {
    return *static_cast<UnicodeString*>(this);
  }
  const UnicodeString& asBase() const {
    return *static_cast<const UnicodeString*>(this);
  }

  UnicodeString* operator->() {
    return static_cast<UnicodeString*>(this);
  }
  const UnicodeString* operator->() const {
    return static_cast<const UnicodeString*>(this);
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

//////////////////////////////////////////////////////////////////////////

template <class Char, Char...CC>
__ndbg_inline constexpr auto operator ""_UStr() noexcept {
  constexpr usize N = sizeof...(CC) + 1;
  using Type = StaticUnicodeString<Align::Up(N)>;
  return Type::New(static_cast<wchar_t>(CC)..., L'\0');
}

} // namespace bootstrap
} // namespace hc
