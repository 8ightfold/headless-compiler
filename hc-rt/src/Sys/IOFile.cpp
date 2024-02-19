//===- Sys/IOFile.cpp -----------------------------------------------===//
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
#include <Sys/IOFile.hpp>

#define $FileErr(e) S::FileResult::Err(e)

using namespace hc;
using namespace hc::sys;
namespace C = hc::common;
namespace S = hc::sys;

IIOMode S::IIOFile::ParseModeFlags(C::StrRef S) {
  static constexpr auto E = IIOMode::None;
  S = S.dropNull();
  if __expect_false(!S.beginsWithAny("rwa"))
    return E;
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
      return E;
    }
  }
  if __expect_false(mmode_count != 1)
    return E;
  return flags;
}

FileResult S::IIOFile::readUnlocked(C::AddrRange data) {
  
  return $FileErr(0);
}

FileResult S::IIOFile::writeUnlocked(C::ImmAddrRange data) {
  if __expect_false(!canWrite()) {
    err = true;
    return $FileErr(eBadFD);
  }

  last_op = IIOOp::Read;
  const auto u8data = data.intoImmRange<u8>();

  if (buf_mode == BufferMode::None) {
    auto R = writeUnlockedNone(u8data);
    flushUnlocked();
    return R;
  } else if (buf_mode == BufferMode::Full) {
    return writeUnlockedFull(u8data);
  } else /* BufferMode::Line */ {
    return writeUnlockedLine(u8data);
  }
}

FileResult S::IIOFile::flushUnlocked() {
  return $FileErr(0);
}

// impl

FileResult S::IIOFile::writeUnlockedNone(C::ImmPtrRange<u8> data) {
  return $FileErr(0);
}

FileResult S::IIOFile::writeUnlockedLine(C::ImmPtrRange<u8> data) {
  return $FileErr(0);
}

FileResult S::IIOFile::writeUnlockedFull(C::ImmPtrRange<u8> data) {
  return $FileErr(0);
}
