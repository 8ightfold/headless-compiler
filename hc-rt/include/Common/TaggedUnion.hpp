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
  struct _TaggedUnionBase {};

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

#define __Union_as_0(name) \
  constexpr auto as_##name()& \
   -> __add_lvalue_reference(__m##name) { \
    __hc_invariant(this->__tag == __M::name); \
    return this->__data.name; \
  } \
  constexpr auto as_##name() const& \
   -> __add_lvalue_reference(const __m##name) { \
    __hc_invariant(this->__tag == __M::name); \
    return this->__data.name; \
  } \
  constexpr auto as_##name()&& \
   -> __add_rvalue_reference(__m##name) { \
    __hc_invariant(this->__tag == __M::name); \
    return __hc_move(this->__data.name); \
  }
#define __Union_as_(name, ...) __VA_OPT__(__Union_as_0(name))
#define __Union_as(n_v) __Union_as_ n_v
#define __Union_ass(n_vs...) $PP_map(__Union_as, n_vs)

// #define __Union_cass_(name, ...) \
//   case __M::name: \
//     ::hc::common::construct_at( \
//       &this->__data.name, \
//       __rhs.__data.name); \
//   break;
// #define __Union_cass(n_v) __Union_cass_ n_v
// #define __Union_casss(n_vs...) $PP_map(__Union_cass, n_vs)

#define __Union_mass_(name, ...) \
  case __M::name: \
    ::hc::common::construct_at( \
      &this->__data.name, \
      __hc_move(__rhs.__data.name)); \
  break;
#define __Union_mass(n_v) __Union_mass_ n_v
#define __Union_masss(n_vs...) $PP_map(__Union_mass, n_vs)

#define __Union_dtor_(name, ...) \
  case __M::name: this->__data.name.~__m##name(); break;
#define __Union_dtor(n_v) __Union_dtor_ n_v
#define __Union_dtors(n_vs...) $PP_map(__Union_dtor, n_vs)

#define $Union(__name, fields...) \
struct __name : ::hc::common::_TaggedUnionBase { \
  using __SelfType = __name; \
  enum class __M { __nullState, __Union_enums(fields) }; \
  __Union_tdefs(fields) \
private: \
  __M __tag = __M::__nullState; \
  union __Union { \
    constexpr __Union() { } \
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
  constexpr void __resetData() __noexcept { \
    this->__clearData(); \
    this->__tag = __M::__nullState; \
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
    __rhs.__resetData(); \
  } \
  constexpr __name& operator=(__name&& __rhs) __noexcept { \
    this->__clearData(); \
    switch ((this->__tag = __rhs.__tag)) { \
      __Union_masss(fields) \
      default: break; \
    } \
    __rhs.__resetData(); \
    return *this; \
  } \
  constexpr ~__name() { \
    this->__resetData(); \
  } \
  constexpr bool __isEmpty() const { \
    return this->__tag == __M::__nullState; \
  } \
  constexpr __M __getTag() const { \
    return this->__tag; \
  } \
  __Union_ass(fields) \
}


// #define $match(val...) \
// if (bool __match_b = !(val).__isEmpty(); __match_b) \
//   for (auto&& __match_ex = (val); \
//    (void)(__match_ex), __match_b; __match_b = false) \
//     switch(__match_ex.__getTag())

#define $match(val...) \
if (auto&& __match_ex = (val); !__match_ex.__isEmpty()) \
  for (bool __match_b = true; __match_b; __match_b = false) \
    switch(__match_ex.__getTag())

#define __Union_arm_i(__case) \
for (bool __match_c##__case = true; \
 ({ if (!__match_c##__case) break; __match_c##__case; }); \
 __match_c##__case = false)

#define $arm(__case) break; \
case ::hc::meta::RemoveCVRef<decltype(__match_ex)>::__M::__case: \
  __Union_arm_i(__case)

#define __Union_armv_n0(__v) __v
#define __Union_armv_n1(__vs...) [__vs]
#define __Union_armv_ni(__v, __x...) \
  $PP_if_valued(__x)( \
    __Union_armv_n1, __Union_armv_n0)( \
      __v __VA_OPT__(,) __x)

/// Extract the name from `__v`.
#define __Union_armv_n(__vs...) __Union_armv_ni(__vs)

#define __Union_armv_i(__case, __v) \
for (auto& __Union_armv_n($PP_rm_parens(__v)) \
  = __match_ex.as_##__case(); \
 __match_c##__case; __match_c##__case = false)

#define $armv(__case, __v) \
$arm(__case) __Union_armv_i(__case, __v)

#define $default break; \
default: __Union_arm_i(__case)
