//===- Common/Tuple.hpp ---------------------------------------------===//
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

#include "Fundamental.hpp"
#include <Meta/ExTraits.hpp>

#define _HC_AGGRESSIVE_INLINE \
 __attribute__((always_inline, flatten, artificial)) inline

namespace hc::common {
  template <usize I, typename T>
  struct _TupleLeaf {
    T __data;
  };

  template <usize I, typename T>
  _HC_AGGRESSIVE_INLINE constexpr T& __extract_leaf(
   _TupleLeaf<I, T>& l) __noexcept {
    return l.__data;
  }

  template <usize I, typename T>
  _HC_AGGRESSIVE_INLINE constexpr T&& __extract_leaf(
   _TupleLeaf<I, T>&& l) __noexcept {
    return static_cast<T&&>(l.__data);
  }

  template <usize I, typename T>
  _HC_AGGRESSIVE_INLINE constexpr const T& __extract_leaf(
   const _TupleLeaf<I, T>& l) __noexcept {
    return l.__data;
  }

  template <usize I, typename T>
  _HC_AGGRESSIVE_INLINE constexpr const T&& __extract_leaf(
   const _TupleLeaf<I, T>&& l) __noexcept {
    return static_cast<const T&&>(l.__data);
  }

  template <typename, typename...>
  struct _TupleBranch;

  template <usize...II, typename...TT>
  struct _TupleBranch<IdxSeq<II...>, TT...> : _TupleLeaf<II, TT>... {
    static constexpr bool __isArray = false;
  };

  // Array Optimization

  template <usize N, typename T>
  struct _TupleArray {
    static constexpr bool __isArray = true;
    T __data[N];
  };

  template <typename T, typename...TT>
  _TupleArray(T&&, TT&&...) -> 
    _TupleArray<sizeof...(TT) + 1, __decay(T)>;

  template <usize I, usize N, typename T>
  _HC_AGGRESSIVE_INLINE constexpr T& __extract_leaf(
   _TupleArray<N, T>& l) __noexcept {
    return l.__data[N];
  }

  template <usize I, usize N, typename T>
  _HC_AGGRESSIVE_INLINE constexpr T&& __extract_leaf(
   _TupleArray<N, T>&& l) __noexcept {
    return static_cast<T&&>(l.__data[N]);
  }

  template <usize I, usize N, typename T>
  _HC_AGGRESSIVE_INLINE constexpr const T& __extract_leaf(
   const _TupleArray<N, T>& l) __noexcept {
    return l.__data[N];
  }

  template <usize I, usize N, typename T>
  _HC_AGGRESSIVE_INLINE constexpr const T&& __extract_leaf(
   const _TupleArray<N, T>&& l) __noexcept {
    return static_cast<const T&&>(l.__data[N]);
  }

  // TODO: Small List Optimization ?

} // namespace hc::common

//======================================================================//
// Tuple Selector
//======================================================================//

namespace hc::common {
  template <typename T, typename...TT>
  concept __array_compatible = 
    meta::not_ref<T> && meta::__all_same<T, TT...>;

  template <typename IIs, typename...TT>
  struct _TupleSelector {
    using Type = _TupleBranch<IIs, TT...>;
  };
  
  template <usize...II, typename Tx, typename Ty, typename...TT>
  requires __array_compatible<Tx, Ty, TT...>
  struct _TupleSelector<IdxSeq<II...>, Tx, Ty, TT...> {
    using Type = _TupleArray<sizeof...(II), Tx>;
  };

  template <typename...TT>
  using __tuple_t = typename _TupleSelector<
    make_idxseq<sizeof...(TT)>, TT...>::Type;
} // namespace hc::common

//======================================================================//
// Implementation
//======================================================================//

namespace hc::common {
  template <typename...TT>
  struct Tuple {
    using BaseType = __tuple_t<TT...>;
    static constexpr auto __isArray = BaseType::__isArray;
    static constexpr usize size = sizeof...(TT);
  public:
    /**
     * @brief Extracts the element at `I` from the tuple.
     * @note C++20: Does not resolve if I >= Size().
     * @tparam I The element to access.
     */
    template <usize I>
    requires(I < sizeof...(TT))
    constexpr decltype(auto) operator[](IdxNode<I>)& {
      return __extract_leaf<I>(__data);
    }

    /// @overload
    template <usize I>
    requires(I < sizeof...(TT))
    constexpr decltype(auto) operator[](IdxNode<I>)&& __noexcept {
      return __extract_leaf<I>(static_cast<BaseType&&>(__data));
    }

    /// @overload
    template <usize I>
    requires(I < sizeof...(TT))
    constexpr decltype(auto) operator[](IdxNode<I>) const& {
      return __extract_leaf<I>(__data);
    }

    /// @overload
    template <usize I>
    requires(I < sizeof...(TT))
    constexpr decltype(auto) operator[](IdxNode<I>) const&& __noexcept {
      return __extract_leaf<I>(static_cast<const BaseType&&>(__data));
    }

    /// Returns sizeof...(TT).
    constexpr static usize Size() __noexcept { return sizeof...(TT); }
    /// Returns `true` if Size() == 0.
    constexpr static bool IsEmpty() __noexcept { return sizeof...(TT) == 0U; }
    /// Returns `true` if using array storage
    constexpr static bool IsArray() __noexcept { return __isArray; }

  public:
    BaseType __data;
  };

  template <typename...TT>
  Tuple(TT&&...) -> Tuple<__decay(TT)...>;

  template <typename...TT>
  constexpr Tuple<TT&...> tie(TT&...tt) {
    return { tt... };
  }

  template <typename...TT>
  constexpr auto tuple_fwd(TT&&...tt)
   -> Tuple<decltype(tt)...> {
    return { __hc_fwd(tt)... };
  }
} // namespace hc::common

#undef _HC_AGGRESSIVE_INLINE
