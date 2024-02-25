//===- Meta/Intrusive.hpp -------------------------------------------===//
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
//  This file defines some methods for accessing the internals of an
//  object. It allows for explicit access and access via ADL.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Fundamental.hpp>

#define $data(ex...)      __get_data(ex)
#define $size(ex...)      __get_size(ex)
#define $capacity(ex...)  __get_capacity(ex)

namespace hc::meta {
  template <typename T, usize N>
  constexpr T* __get_data(T(&arr)[N]) {
    return arr;
  }

  template <typename T, usize N>
  constexpr usize __get_size(T(&)[N]) {
    return N;
  }

  template <typename T, usize N>
  constexpr usize __get_capacity(T(&)[N]) {
    return N;
  }

  struct IADL {
    template <typename T>
    static constexpr decltype(auto) Data(T&& V) {
      if constexpr (requires { V.data(); }) {
        return V.data();
      } else {
        return __get_data(__hc_fwd(V));
      }
    }

    template <typename T>
    static constexpr auto Size(T&& V) {
      if constexpr (requires { V.size(); }) {
        return V.size();
      } else if constexpr (requires { V.Size(); }) {
        return V.Size();
      } else {
        return __get_size(__hc_fwd(V));
      }
    }

    template <typename T>
    static constexpr auto Capacity(T&& V) {
      if constexpr (requires { V.capacity(); }) {
        return V.capacity();
      } else if constexpr (requires { V.Capacity(); }) {
        return V.Capacity();
      } else {
        return __get_capacity(__hc_fwd(V));
      }
    }

    template <typename T>
    static constexpr auto CapacityPtr(T&& V) {
      return IADL::Data(__hc_fwd(V))
        + IADL::Capacity(__hc_fwd(V));
    }

    template <typename T>
    static constexpr auto& SizeRef(T&& V) {
      if constexpr (requires { V.__size; }) {
        return V.__size;
      } else {
        return __get_size_ref(__hc_fwd(V));
      }
    }
  };
} // namespace hc::meta
