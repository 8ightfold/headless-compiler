//===- Common/TaggedUnion.hpp -----------------------------------===//
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
//  A utility for defining rust style variants.
//  TODO: Add copy ctor/assign?
//
//===----------------------------------------------------------------===//

#pragma once
#pragma clang system_header

#include "Fundamental.hpp"
#include "Lifetime.hpp"
#include "Tuple.hpp"
#include <Meta/Preproc.hpp>

namespace hc::common {
  struct _TaggedUnionNull {};

  template <typename...TT>
  struct _TaggedUnionField {
    using type = Tuple<__remove_cvref(TT)...>;
  };

  template <typename T>
  struct _TaggedUnionField<T> {
    using type = __remove_cvref(T);
  };

  template <>
  struct _TaggedUnionField<> {
    using type = _TaggedUnionNull;
  };

  template <typename...TT>
  using __taggedunion_field = 
    typename _TaggedUnionField<TT...>::type;
} // namespace hc::common

#define __Union_enum_(name, ...) name,
#define __Union_enum(n_v) __Union_enum_ n_v
#define __Union_enums(n_vs...) $PP_map(__Union_enum, n_vs)

#define __Union_td_(name, ty...) \
  using __m##name = ::hc::common::__taggedunion_field<ty>;
#define __Union_tdef(n_v) __Union_td_ n_v
#define __Union_tdefs(n_vs...) $PP_map(__Union_tdef, n_vs)

#define __Union_ud_(name, ...) __m##name name;
#define __Union_udef(n_v) __Union_ud_ n_v
#define __Union_udefs(n_vs...) $PP_map(__Union_udef, n_vs)

#define __Union_ctor_(name, xx...) \
  static constexpr __SelfType name(__VA_OPT__(auto&&...args)) { \
    __SelfType V {__M::name}; \
    __VA_OPT__(::hc::common::construct_at( \
      &V.__data.name, __hc_fwd(args)...);) \
    return V; \
  }
#define __Union_ctor(n_v) __Union_ctor_ n_v
#define __Union_ctors(n_vs...) $PP_map(__Union_ctor, n_vs)

// #define __Union_cass_(name, ...) \
//   case __M::name: this->__data.name = \
//     __rhs.__data.name; \
//   break;
// #define __Union_cass(n_v) __Union_cass_ n_v
// #define __Union_casss(n_vs...) $PP_map(__Union_cass, n_vs)

#define __Union_mass_(name, ...) \
  case __M::name: this->__data.name = \
    __hc_move(__rhs.__data.name); \
  break;
#define __Union_mass(n_v) __Union_mass_ n_v
#define __Union_masss(n_vs...) $PP_map(__Union_mass, n_vs)

#define __Union_dtor_(name, ...) \
  case __M::name: this->__data.name.~__m##name(); break;
#define __Union_dtor(n_v) __Union_dtor_ n_v
#define __Union_dtors(n_vs...) $PP_map(__Union_dtor, n_vs)

#define $Union(__name, fields...) \
struct __name { \
  using __SelfType = __name; \
  enum class __M { __nullState, __Union_enums(fields) }; \
  __Union_tdefs(fields) \
private: \
  __M __tag = __M::__nullState; \
  union __Union { \
    constexpr ~__Union() { } \
    char __default_; \
    __Union_udefs(fields) \
  } __data; \
private: \
  constexpr __name(__M tag) : __tag(tag) {} \
  constexpr void __clearData() __noexcept { \
    switch (this->__tag) { \
      __Union_dtors(fields) \
      default: break; \
    } \
  } \
public: \
  __Union_ctors(fields) \
  constexpr __name() = default; \
  __name(const __name&) = delete; \
  constexpr __name(__name&& __rhs) : __tag(__rhs.__tag) { \
    switch (this->__tag) { \
      __Union_masss(fields) \
      default: break; \
    } \
  } \
  constexpr __name& operator=(__name&& __rhs) __noexcept { \
    this->__clearData(); \
    switch ((this->__tag = __rhs.__tag)) { \
      __Union_masss(fields) \
      default: break; \
    } \
    return *this; \
  } \
  constexpr ~__name() { \
    this->__clearData(); \
    __tag = __M::__nullState; \
  } \
}
