//===- Meta/ID.hpp --------------------------------------------------===//
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
//  Defines a method of uniquely identifying arbitrary types without RTTI.
//
//===----------------------------------------------------------------===//

#pragma once

#include "Objects.hpp"

#define $typeid(ty...) (*::hc::meta::typeID<ty>)

//=== Static ===//
namespace hc::meta {
  using UUID = const struct UID*;

  struct UID {
    usize size() const;
    const char* name() const;
    usize hash_code() const;
    constexpr bool before(const UID& other) const {
      return this < &other;
    }
    constexpr bool operator==(const UID& other) const {
      return this == &other;
    }
  };

  template <usize N>
  struct [[gnu::packed]] IDName : UID {
    static_assert(N > 0);
  public:
    usize size;
    char name[N];
  };

  template <typename T>
  __visibility(hidden) constexpr auto __get_idname() {
    static constexpr usize Sz = sizeof(__PRETTY_FUNCTION__);
    static constexpr usize F  = sizeof("auto hc::meta::__get_idname() [T =");
    static constexpr usize B  = sizeof("]");
    static constexpr usize Total = Sz - F - B;
    constexpr auto pretty = __PRETTY_FUNCTION__;
    return [&] <usize...II> (common::IdxSeq<II...>) {
      return IDName<Total + 1> {
        .size = Total,
        .name = {pretty[II + F]...}
      };
    } (common::make_idxseq<Total>());
  }

  template <typename T>
  class TypeID {
    // Not `constexpr` to avoid merged definitions.
    static inline constinit
      auto ID = __get_idname<T>(); 
  public:
    static constexpr UUID GetUUID() {
      return &ID;
    }
    static constexpr const auto& GetID() {
      return ID;
    }
  };

  template <typename T>
  inline constexpr UUID typeID = &TypeID<T>::GetID();
} // namespace hc::meta

//=== Dynamic ===//
namespace hc::meta {
  // TODO: Static registry
} // namespace hc::meta
