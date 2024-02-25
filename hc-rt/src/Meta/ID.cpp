//===- Meta/ID.cpp --------------------------------------------------===//
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

#include <Common/Casting.hpp>
#include <Meta/ID.hpp>

using namespace hc;
using namespace hc::meta;
namespace C = hc::common;
namespace M = hc::meta;

namespace {
  struct IUUID : UID {
    usize size;
    char name[];
  };

  const IUUID* X(const UID* self) {
    return ptr_cast<const IUUID>(self);
  }
} // namespace `anonymous`

usize UID::size() const {
  return X(this)->size;
}

const char* UID::name() const {
  return X(this)->name;
}

usize UID::hash_code() const {
  return reinterpret_cast<uptr>(this);
}