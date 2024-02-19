//===- Common/Checked.hpp -------------------------------------------===//
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
//  This file implements a basic checked arithmetic type. When an 
//  operation is invalid, it just doesn't happen.
//
//===----------------------------------------------------------------===//

#pragma once

#include "Limits.hpp"

HC_HAS_BUILTIN(add_overflow);
HC_HAS_BUILTIN(sub_overflow);

namespace hc::common {
  template <typename Int, bool Capped>
  struct _Checked {
    using SelfType = _Checked;
    using BaseType = meta::RemoveCVRef<Int>;
    static constexpr bool isUnsigned = meta::is_unsigned<BaseType>;
    static constexpr bool useUFlow = isUnsigned && !Capped;
  public:
    constexpr _Checked() : __data(0) {}
    constexpr _Checked(Int I) : __data(I) {}

    __always_inline constexpr operator BaseType() const {
      return static_cast<BaseType>(__data);
    }
    __always_inline explicit operator bool() const {
      return static_cast<bool>(__data);
    }

    template <meta::is_integral U>
    requires(!meta::is_same<BaseType, U>)
    explicit constexpr operator U() const {
      if $is_consteval() {
        if __expect_false(!canConvertTo<U>())
          $unreachable_msg("Conversion impossible.");
      }
      return static_cast<U>(__data);
    }

    template <meta::is_integral U, typename T>
    __always_inline static constexpr
     U ConvertTo(BaseType B) {
      return static_cast<U>(_Checked{B});
    }

    //=== Checks ===//

    template <typename U>
    static constexpr bool IsSameSignedness() {
      return isUnsigned == meta::is_unsigned<U>;
    }

    template <typename U>
    static constexpr bool IsSmaller() {
      return sizeof(U) < sizeof(Int);
    }

    template <typename U>
    constexpr bool canConvertTo() const {
      if constexpr (meta::is_same<BaseType, U>)
        return true;
      if constexpr (!IsSmaller<U>() || Capped)
        return true;
      if constexpr (isUnsigned)
        return __data <= Max<U>;
      // else:
      return (__data <= Max<U>) 
        && (__data >= Min<U>);
    }

    constexpr friend _Checked operator-(_Checked lhs, BaseType rhs) {
      const BaseType C = lhs.__data;
      if constexpr (useUFlow) {
        /// Since unsigned values can over/underflow,
        /// we don't even check here (Capped == false).
        return {C - rhs};
      } else if constexpr (isUnsigned) {
        /// We know it must be capped here.
        return (C > rhs) 
          ? _Checked{C - rhs}
          : _Checked{0};
      } else if constexpr (Capped) {
        BaseType out;
        if (!__builtin_sub_overflow(C, rhs, &out)) [[likely]] {
          return {out};
        } else [[unlikely]] {
          if (rhs < 0)
            return {Max<BaseType>};
          else
            return {Min<BaseType>};
        }
      } else {
        BaseType out;
        if __expect_false(
         __builtin_sub_overflow(C, rhs, &out))
          return lhs;
        return {out};
      }
    }

    template <meta::is_integral U>
    requires(!meta::is_same<BaseType, U>)
    constexpr friend _Checked operator-(_Checked lhs, U rhs) {
      return lhs - BaseType(_Checked<U, Capped>{rhs});
    }

    template <meta::is_integral U>
    __always_inline constexpr friend auto operator-(U lhs, _Checked rhs) {
      return _Checked<U, Capped>{lhs} - rhs.__data;
    }
  
  public:
    Int __data;
  };

  template <bool IsCapped, typename Int>
  requires meta::is_integral<meta::RemoveCVRef<Int>>
  constexpr auto Checked(Int&& I) noexcept {
    return _Checked<Int, IsCapped>{__hc_fwd(I)};
  }

  template <bool IsCapped>
  constexpr bool Checked(bool B) noexcept { 
    return B;
  }
} // namespace hc::common
