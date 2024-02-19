//===- Parcel/Skiplist.hpp ------------------------------------------===//
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
//  A colony-style skiplist with a statically sized array backing.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Limits.hpp>
#include <Common/RawLazy.hpp>
#include "BitList.hpp"

namespace hc::parcel {
  template <typename T, usize N>
  struct Skiplist {
    using SelfType = Skiplist;
    using Type = CC::RawLazy<T>;
    using DataType = Type[N];
  public:

  public:
    DataType   __data {};
    BitList<N> __bits;
  };
} // namespace hc::parcel
