//===- Common/Unwrap.hpp --------------------------------------------===//
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

#include "Tuple.hpp"

#define $unwrap(obj, on_err...) ({ \
  if __expect_false(!obj) \
    return ::hc::__unwrap_fail(obj, ##on_err); \
  *obj; })

//=== Forward Decls ===//
namespace hc::common {
  template <typename>
  struct Err;

  template <typename, typename>
  struct Result;

  template <typename>
  struct Option;
} // namespace hc::common

//=== Implementation ===//
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

  [[gnu::always_inline, gnu::nodebug]]
  inline constexpr auto __unwrap_fail(auto&&, auto&&...args) __noexcept {
    return _Wrapper{__hc_fwd(args)...};
  }

  template <typename T, typename E>
  [[gnu::always_inline, gnu::nodebug]]
  constexpr auto __unwrap_fail(
   const common::Result<T, E>& R, auto&&...args) __noexcept {
    if constexpr (sizeof...(args) == 0) 
      return common::Err(R.err());
    else _Wrapper{__hc_fwd(args)...};
  }
} // namespace hc
