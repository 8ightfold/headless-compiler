//===- Common/Intrusive.hpp -----------------------------------------===//
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
//  This file defines some methods for reflecting over objects via ADL.
//
//===----------------------------------------------------------------===//

#pragma once

#include "Features.hpp"
#include "Intrusive.hpp"

#define $reflexpr(type...) ::hc::Refl<__remove_cvref(type)>{}

namespace hc {
  template <typename T, T Tag>
  struct _ReflFields {
    using SelfType = _ReflFields<T, Tag>;
    static constexpr auto& __fieldArray = __refl_fieldarray(Tag);
    static constexpr usize __fieldCount = common::IADL::Size(__fieldArray) - 1;
  public:
    static constexpr auto Name(const T& V) {
      auto field_name = __refl_fieldname(V);
      if __expect_false(!field_name) 
        return "??";
      if constexpr (requires { __refl_markprefix(V); }) {
        return field_name + __refl_markprefix(V);
      } else {
        return field_name;
      }
    }

    static constexpr auto NameAt(auto I) {
      __hc_invariant(I < __fieldCount);
      return SelfType::Name(__fieldArray[I]);
    }

    static constexpr auto Count() {
      return __fieldCount;
    }

    static constexpr auto operator[](auto I) {
      __hc_invariant(I < __fieldCount);
      return __fieldArray[I];
    }
  };

  template <typename T, T Tag = T()>
  struct Refl {
    using SelfType  = Refl<T, Tag>;
    using FieldType = _ReflFields<T, Tag>;
    static constexpr FieldType __fields { };
  public:
    static constexpr auto& Name() {
      return __refl_markname(Tag);
    }

    static constexpr const FieldType& Fields() {
      return SelfType::__fields;
    }
  };
} // namespace hc
