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

#pragma once

#include "Features.hpp"
#include "Memory.hpp"
#include <Std/memory>

namespace hc::common {
  using ::std::construct_at;
  using ::std::destroy_at;

  template <typename T>
  __always_inline constexpr T*
   construct_at_ref(T& ref, auto&&...args) __noexcept {
    return common::construct_at(
      common::__addressof(ref), __hc_fwd(args)...);
  }

  template <typename T>
  __always_inline constexpr void
   destroy_at_ref(T& ref) __noexcept {
    common::destroy_at(common::__addressof(ref));
  }

  template <bool>
  struct _DestroyAdaptor {
    template <typename It>
    static constexpr void __destroy(It I, It E) __noexcept {
      for (; I != E; ++I) 
        common::destroy_at(common::__addressof(*I));
    }
  };

  template <>
  struct _DestroyAdaptor<true> {
    template <typename It>
    [[gnu::nodebug, gnu::always_inline]]
    inline static constexpr void
     __destroy(It, It) __noexcept { }
  };

  template <typename It>
  [[gnu::artificial]]
  inline constexpr void __destroy(It I, It E) __noexcept {
    using type = __remove_cvref(decltype(*I));
    if $is_consteval() {
      return _DestroyAdaptor<false>::__destroy(I, E);
    }
    _DestroyAdaptor<__is_trivially_destructible(type)>::__destroy(I, E);
  }

  template <typename It>
  [[gnu::always_inline]]
  inline constexpr void __destroy(It I, usize len) __noexcept {
    __destroy(I, I + len);
  }
} // namespace hc::common
