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

#include <Sys/Win/IOFile.hpp>
#include <Common/Casting.hpp>
#include <Common/InlineMemcpy.hpp>
#include <Parcel/Skiplist.hpp>
#include "Filesystem.hpp"

#define $FileErr(e) S::FileResult::Err(e)

using namespace hc;
using namespace hc::sys;
namespace C = hc::common;
namespace P = hc::parcel;
namespace S = hc::sys;

namespace {
  constexpr usize max_files = HC_MAX_FILE_SLOTS;
  constinit P::ALSkiplist<WinIOFile, max_files> file_slots {};
} // namespace `anonymous`

FileResult S::win_file_read(IIOFile* file, common::AddrRange in) {
  return $FileErr(0);
}

FileResult S::win_file_write(IIOFile* file, common::ImmAddrRange out) {
  return $FileErr(0);
}

IOResult<long> S::win_file_seek(IIOFile* file, long offset, int) {
  __hc_unreachable("`win_file_seek` unimplemented.");
  return $Err(0);
}

int S::win_file_close(IIOFile* file) {
  const auto F = ptr_cast<WinIOFile>(file);
  if (!file_slots.inRange(F))
    return int(Error::eBadFD);
  return 0;
}

//=== Implementation ===//

IIOFile* File::openFileRaw(C::StrRef filename, C::StrRef flags) {
  const auto F = IIOFile::ParseModeFlags(flags);
  if (filename.isEmpty()) {
    // TODO: Set error?
    return nullptr;
  }

  // Check if the path uses UNC paths. We assume this will not
  // be the case most of the time.
  if (!filename.beginsWith("\\??\\", "\\GLOBAL??\\")) {

  }

  // Check for `/.` and `/..` yk

  return nullptr;
}

bool File::closeFileRaw(IIOFile* file) {
  const auto F = ptr_cast<WinIOFile>(file);
  if (!file_slots.inRange(F)) {
    err = Error::eBadFD;
    return false;
  }
  F->close();
  const bool R = file_slots.eraseRaw(F);
  err = R ? Error::eNone : Error::eBadFD;
  return R;
}

IIOFile* S::open_file(C::StrRef path, IIOFileBuf& buf, C::StrRef flags) {
  const auto F = IIOFile::ParseModeFlags(flags);
  if (path.isEmpty()) {
    // TODO: Set error?
    return nullptr;
  }

  // Check if the path uses NT paths. We assume this will not
  // be the case most of the time.
  if (!path.beginsWith("\\??\\", "\\GLOBAL??\\")) {

  }

  // Check for `/.` and `/..` yk

  return nullptr;
}

common::VErr<Error> S::close_file(IIOFile* file) {
  const auto F = ptr_cast<WinIOFile>(file);
  if (!file_slots.inRange(F))
    return false;
  F->close();
  return file_slots.eraseRaw(F);
}
