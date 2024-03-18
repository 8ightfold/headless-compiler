//===- Parcel/StringTable.hpp ---------------------------------------===//
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
//  A container which allows for simple creation of statically-backed
//  strings in a dynamic context. Used as the base for intern tables.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/StrRef.hpp>
#include <Common/Strings.hpp>
#include <Parcel/StaticVec.hpp>

namespace hc::parcel {
  template <CC::__char_type Char, usize BufferSize>
  struct [[gsl::Owner]] IStringTable :
   protected StaticVec<Char, BufferSize> {
    using BaseType = StaticVec<Char, BufferSize>;
    using SelfType = IStringTable;
  public:
    constexpr IStringTable() : BaseType() {}

  public:
    
  };

  template <usize N>
  using StringTable = IStringTable<char, N>;

  template <usize N>
  using WStringTable = IStringTable<wchar_t, N>;

} // namespace hc::parcel
