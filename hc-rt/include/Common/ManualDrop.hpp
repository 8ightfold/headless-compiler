//===- Common/ManualDrop.hpp ----------------------------------------===//
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
//  Defines a wrapper which constructs an object but does not destroy it.
//
//===----------------------------------------------------------------===//

#pragma once

#include "RawLazy.hpp"

namespace hc::common {

template <typename T>
struct ManualDrop : public RawLazy<T> {
  using BaseType = RawLazy<T>;
  using SelfType = ManualDrop;
  using BaseType::dtor;
private:
  using BaseType::ctor;
public:
  constexpr ManualDrop(auto&&...args) : BaseType() {
    (void) BaseType::ctor(FWD(args)...);
  }

  inline void drop() {
    BaseType::dtor(); 
  }
};

} // namespace hc::common
