//===- Sys/Win/IOFile.cpp -------------------------------------------===//
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
#include <Common/InlineMemcpy.hpp>
#include <Sys/Win/IOFile.hpp>

using namespace hc;
using namespace hc::sys;
namespace C = hc::common;
namespace S = hc::sys;

/// r: Read, w: Write, a: Append, 
/// +: Plus, b: Binary, x: Exclude.
IIOMode S::IIOFile::ParseModeFlags(C::StrRef S) {
  static constexpr auto err = IIOMode::None;
  S = S.dropNull();
  if __expect_false(!S.beginsWithAny("rwa"))
    return err;
  auto flags = IIOMode::None;
  /// Checks if each main mode has only been used once.
  int mmode_count = 0;
  for (char C : S) {
    switch (C) {
     case 'a':
      flags |= IIOMode::Append;
      ++mmode_count;
      break;
     case 'r':
      flags |= IIOMode::Read;
      ++mmode_count;
      break;
     case 'w':
      flags |= IIOMode::Write;
      ++mmode_count;
      break;
     case '+':
      flags |= IIOMode::Plus;
      break;
     case 'b':
      flags |= IIOMode::Binary;
      break;
     case 'x':
      flags |= IIOMode::Exclude;
      break;
     default:
      return err;
    }
  }
  if __expect_false(mmode_count != 1)
    return err;
  return flags;
}

FileResult S::IIOFile::readUnlocked(C::AddrRange data) {
  return FileResult::Err(0);
}

FileResult S::IIOFile::writeUnlocked(C::ImmutAddrRange data) {
  return FileResult::Err(0);
}

FileResult S::IIOFile::flushUnlocked() {
  return FileResult::Err(0);
}
