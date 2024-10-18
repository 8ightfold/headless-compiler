//===- Std/__tuple/tuple_element.hpp --------------------------------===//
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

#include <Std/cstddef>

namespace std {

template <size_t I, class T>
struct tuple_element;

template <size_t I, class T>
using tuple_element_t __nodebug = typename tuple_element<I, T>::type;

template <size_t I, class T>
struct tuple_element<I, const T> {
  using type __nodebug = const tuple_element_t<I, T>;
};

template <size_t I, class T>
struct tuple_element<I, volatile T> {
  using type __nodebug = volatile tuple_element_t<I, T>;
};

template <size_t I, class T>
struct tuple_element<I, const volatile T> {
  using type __nodebug = const volatile tuple_element_t<I, T>;
};

} // namespace std
