//===- Sys/Core/Nt/CheckPacking.cpp ---------------------------------===//
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
//  Does checks to see if Nt types are correctly packed.
//
//===----------------------------------------------------------------===//

#include "Info.hpp"

using namespace hc::sys;
using namespace hc::sys::win;

#define PACKING_TEST(ty) static_assert(__fsiwrap_v<FS##ty##Info>, \
  #ty "'s fields are packed incorrectly. Please report this.")

namespace {
  static constexpr usize __fsisz = 16u;

  template <typename FSInfoType>
  using __fsiwrap = FSInfoClassWrapper<FSInfoType, __fsisz>;

  template <typename T>
  static constexpr bool __fsiwrap_v = 
    ($offsetof(__ex_data, __fsiwrap<T>) == sizeof(T));
} // namespace `anonymous`

PACKING_TEST(Attribute);
PACKING_TEST(Control);
PACKING_TEST(Device);
PACKING_TEST(DriverPath);
PACKING_TEST(FullSize);
PACKING_TEST(ObjectID);
PACKING_TEST(SectorSize);
PACKING_TEST(Size);
PACKING_TEST(Volume);
