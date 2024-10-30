//===- Sys/Win/Nt/Utils.hpp -----------------------------------------===//
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
//  Trait utilities for several syscall bridges.
//
//===----------------------------------------------------------------===//

#pragma once

#include "Generic.hpp"
#include <Meta/Traits.hpp>
#include <Std/__algorithm/min.hpp>
#include <Std/__algorithm/max.hpp>

namespace hc::sys::win {

/// @brief Alternate user facing type.
/// @tparam User The type on the user's side.
/// @tparam Kern The type the kernel recieves.
template <class User, class Kern>
struct Alt {
  static_assert(meta::not_same<User, Kern>,
    "Alt<> types cannot be the same!");
};

/// @brief Either type is valid.
/// @tparam A The preferred type for `Set`.
/// @tparam B The alternate type.
template <class A, class B>
struct Or {
  static_assert(meta::not_same<A, B>,
    "Or<> types cannot be the same!");
};

struct ISizeBool { };

/// @brief When used for `NtSet*`, a `bool` sets the size.
/// @tparam Q The type used for Query.
template <class Q>
struct SizeBool : ISizeBool { };

/// Splits types between `NtQuery*` and `NtSet*`.
/// @tparam Q The type used for Query.
/// @tparam S The type used for Set.
template <class Q, class S>
struct Var {
  static_assert(meta::not_same<Q, S>,
    "Var<> types cannot be the same!");
};

//////////////////////////////////////////////////////////////////////////
// [Query|Set]Type

template <typename T> struct QSType {
  using Type = T;
  static constexpr bool needsCasting = false;
  static constexpr usize argMin = 1;
  static constexpr usize argMax = 1;
public:
  static constexpr usize Size(T&) noexcept {
    return sizeof(T);
  }
  static constexpr T& Arg(T& V) noexcept {
    return V;
  }
};

template <typename T> struct QSType<T[]> {
  using Type = T[];
  static constexpr bool needsCasting = false;
  static constexpr usize argMin = 1;
  static constexpr usize argMax = 2;
public:
  template <usize N>
  static constexpr usize Size(T(&A)[N]) noexcept {
    return sizeof(A);
  }
  static constexpr usize Size(com::PtrRange<T> R) noexcept {
    return R.sizeInBytes();
  }
  static constexpr usize Size(T* lhs, T* rhs) noexcept {
    return com::ptr_distance_bytes(lhs, rhs);
  }

  template <usize N>
  static constexpr T* Arg(T(&A)[N]) noexcept {
    return A;
  }
  static constexpr T* Arg(com::PtrRange<T> R) noexcept {
    return R.data();
  }
  static constexpr T* Arg(T* lhs, T*) noexcept {
    return lhs;
  }
};

template <> struct QSType<void> {
  using Type = void;
  static constexpr bool needsCasting = false;
  static constexpr usize argMin = 0;
  static constexpr usize argMax = 0;
public:
  static constexpr usize Size() noexcept {
    return 0;
  }
  static constexpr __void Arg() noexcept {
    return {};
  }
};

template <class User, class Kern>
struct QSType<Alt<User, Kern>> {
  using Type = User;
  static constexpr bool needsCasting = true;
  static constexpr usize argMin = 1;
  static constexpr usize argMax = 1;
public:
  static constexpr usize Size(User&) noexcept {
    return sizeof(Kern);
  }
  static constexpr Kern Arg(User& V) noexcept {
    return static_cast<Kern>(V);
  }
};

template <class A, class B>
struct QSType<Or<A, B>>
 : public QSType<A>, public QSType<B> {
  using _A = QSType<A>;
  using _B = QSType<B>;
public:
  using Type = A;
  static constexpr bool needsCasting = false;
  static constexpr usize argMin
    = std::min(_A::argMin, _B::argMin);
  static constexpr usize argMax
    = std::max(_A::argMax, _B::argMax);
public:
  using _A::Size;
  using _B::Size;
  using _A::Arg;
  using _B::Arg;
};

template <> struct QSType<ISizeBool> {
  using Type = bool;
  static constexpr bool needsCasting = false;
  static constexpr usize argMin = 1;
  static constexpr usize argMax = 1;
public:
  static constexpr usize Size(bool B) noexcept {
    return B ? 1U : 0U;
  }
  static constexpr bool Arg(bool B) noexcept {
    return B;
  }
};

template <class Q>
struct QSType<SizeBool<Q>> : public QSType<ISizeBool> { };

//////////////////////////////////////////////////////////////////////////
// [Query|Set]Info

template <typename T> struct QSInfo {
  using QType = QSType<T>;
  using SType = QSType<T>;
};

template <class A, class B>
struct QSInfo<Or<A, B>> {
  using QType = QSType<A>;
  using SType = QSType<Or<A, B>>;
};

template <class Q>
struct QSInfo<SizeBool<Q>> {
  using QType = QSType<Q>;
  using SType = QSType<ISizeBool>;
};

template <class Q, class S>
struct QSInfo<Var<Q, S>> {
  using QType = QSType<Q>;
  using SType = QSType<S>;
};

template <typename QS>
using QSQueryType = typename QS::QType;

template <typename QS>
using QSSetType = typename QS::SType;

} // namespace hc::sys::win
