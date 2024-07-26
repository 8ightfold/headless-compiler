//===- Meta/Unwrap.hpp ----------------------------------------------===//
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
//  A utility for generically returning on error.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Tuple.hpp>

/// Allows for rust style unwrapping of values.
/// @param obj The object to be unwrapped.
/// @param on_err The value to be used if `!obj`.
#define $unwrap(obj, on_err...) ({ \
  auto&& objU__ = (obj); \
  if __expect_false(!obj__) \
    return ::hc::__unwrap_fail(objU__, ##on_err); \
  ::hc::_FwdWrapper{*objU__}; \
}).get()

/// Unwrapping for void functions.
#define $unwrap_void(obj) \
 $unwrap(obj, ::hc::__void{})


//======================================================================//
// Forward Decls
//======================================================================//

namespace hc::common {
  template <typename>
  struct Err;

  template <typename, typename>
  struct Result;

  template <typename>
  struct Option;
} // namespace hc::common

//======================================================================//
// Implementation
//======================================================================//

namespace hc {
  template <typename...TT>
  struct _Wrapper {
    using _Ids = common::make_idxseq<sizeof...(TT)>;
  public:
    template <typename U>
    constexpr operator U() {
      return [this] <usize...II> 
       (common::IdxSeq<II...>) -> U {
        return __Ctor<U>(__hc_fwd(__tup[__i<II>])...);
      }(_Ids{});
    }
  
  private:
    template <typename U, typename...Args>
    constexpr static U __Ctor(Args&&...args) __noexcept {
      static_assert(__is_constructible(U, Args...),
        "The requested type is not constructible "
        "from the current arguments.");
      return U(__hc_fwd(args)...);
    }

  public:
    common::Tuple<TT...> __tup;
  };

  template <typename...TT>
  _Wrapper(TT&&...) -> _Wrapper<TT...>;

  template <typename T>
  struct _FwdWrapper {
    constexpr operator T() {
      return static_cast<T>(__data);
    }
    constexpr T get() {
      return static_cast<T>(__data);
    }
  public:
    T __data;
  };

  template <typename T>
  _FwdWrapper(T&&) -> _FwdWrapper<T>;

  [[gnu::always_inline, gnu::nodebug]]
  inline constexpr auto __unwrap_fail(auto&&, auto&&...args) __noexcept {
    return _Wrapper{__hc_fwd(args)...};
  }

  [[gnu::always_inline, gnu::nodebug]]
  inline constexpr void __unwrap_fail(auto&&, __void) __noexcept { }

  template <typename T, typename E>
  [[gnu::always_inline, gnu::nodebug]]
  constexpr auto __unwrap_fail(
   const common::Result<T, E>& R, auto&&...args) __noexcept {
    if constexpr (sizeof...(args) == 0) 
      return common::Err(R.err());
    else _Wrapper{__hc_fwd(args)...};
  }
} // namespace hc
