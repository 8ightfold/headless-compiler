//===- Std/__tuple/tuple_size.hpp -----------------------------------===//
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

#include <Meta/Traits.hpp>
#include <Std/__type_traits/integral_constant.hpp>

namespace std {

template <class T>
struct tuple_size;

template <typename T>
requires hc::meta::only_const<T>
struct tuple_size<const T> :
 public index_constant<tuple_size<T>::value> {};

template <typename T>
requires hc::meta::only_volatile<T>
struct tuple_size<volatile T> :
 public index_constant<tuple_size<T>::value> {};

template <typename T>
struct tuple_size<const volatile T> :
 public index_constant<tuple_size<T>::value> {};

template <typename T>
__global size_t tuple_size_v = tuple_size<T>::value;

} // namespace std
