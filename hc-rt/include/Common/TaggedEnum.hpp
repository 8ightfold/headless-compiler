//===- Common/TaggedEnum.hpp ------------------------------------===//
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
//  A utility for defining enums with autogenerated reflection info.
//
//===----------------------------------------------------------------===//

#pragma once
#pragma clang system_header

#include "Fundamental.hpp"
#include <Meta/Preproc.hpp>

#define $MarkPrefix(type, prefix) \
inline constexpr usize __refl_markprefix(enum type) { \
  return sizeof(prefix) - 1; \
}

#define $MarkName(type) \
inline constexpr auto& __refl_markname(enum type) { \
  return __hc_stringify(type); \
}

#define __Enum_def_(name, type...) name __VA_OPT__(:) type
#define __Enum_name_(name, ...) name

#define __Enum_def(ty_n) __Enum_def_ ty_n
#define __Enum_name(ty_n) __Enum_name_ ty_n

#define __Enum_value_(name, val...) name __VA_OPT__(=) val,
#define __Enum_value(n_v) __Enum_value_ n_v
#define __Enum_values(n_vs...) $PP_map(__Enum_value, n_vs)

#define __Enum_case_(name, ...) case Ty::name: return #name;
#define __Enum_case(n_v) __Enum_case_ n_v
#define __Enum_cases(n_vs...) $PP_map(__Enum_case, n_vs)

#define $EnumFieldNames(name, fields...) \
constexpr const char* __refl_fieldname(enum name E) { \
  using Ty = name; \
  switch (E) { \
    __Enum_cases(fields) \
    default: return nullptr; \
  } \
}

#define __Enum_idx_(name, ...) Ty::name,
#define __Enum_idx(n_v) __Enum_idx_ n_v
#define __Enum_idxs(n_vs...) $PP_map(__Enum_idx, n_vs)

#define $EnumFieldArray(name, fields...) \
inline constexpr auto& __refl_fieldarray(enum name) { \
  using Ty = name; \
  static constexpr name A[] { \
    __Enum_idxs(fields) \
    name() \
  }; \
  return A; \
}

#define $EnumFields(name, fields...) \
 $EnumFieldNames(name, fields) \
 $EnumFieldArray(name, fields) \
 $MarkName(name)

/// Creates an enum with extra information.
/// TODO: Add example
#define $Enum(name_type, fields...) \
enum __Enum_def(name_type) { __Enum_values(fields) }; \
$EnumFields(__Enum_name(name_type), fields) \
enum __Enum_def(name_type)

#define $StrongEnum(name_type, fields...) \
enum class __Enum_def(name_type) { __Enum_values(fields) }; \
$EnumFields(__Enum_name(name_type), fields) \
enum class __Enum_def(name_type)
