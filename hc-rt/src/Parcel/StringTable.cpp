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

#include <Parcel/StringTable2.hpp>

using namespace hc;
using namespace hc::parcel;

namespace {
  static constexpr usize __maxDepthFactor = 2;

} // namespace `anonymous`


void IStringTable::sortLexicographically(bool keep_sorted) {
  this->flags.ksorted = keep_sorted;
}

void IStringTable::unsort() {
  __hc_todo("unsort");
}
