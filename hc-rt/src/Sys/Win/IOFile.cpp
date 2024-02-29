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
#include <Common/DynAlloc.hpp>
#include <Common/InlineMemcpy.hpp>
#include <Parcel/Skiplist.hpp>
#include "Filesystem.hpp"
#include "PathNormalizer.hpp"

#define $FileErr(e) S::FileResult::Err(e)

using namespace hc;
using namespace hc::sys;
namespace C = hc::common;
namespace P = hc::parcel;
namespace S = hc::sys;

namespace {
  constexpr usize max_files = RT_MAX_FILES;
  constinit P::Skiplist<WinIOFile, max_files> file_slots {};
} // namespace `anonymous`

//======================================================================//
// Implementation
//======================================================================//

namespace {
  WinIOFile* __nt_openfile(FileAdaptor& self, C::PtrRange<wchar_t> wpath, IIOMode flags) {
    __hc_invariant(!wpath.isEmpty());
    auto name  = win::UnicodeString::New(wpath);
    (void) name;
    return nullptr;
  }

  WinIOFile* __nt_openfile(FileAdaptor& self, C::StrRef path, IIOMode flags) {
    __hc_invariant(!path.isEmpty());
    auto wpath = $to_wstr(path.data());
    if (path.beginsWith("\\\\?\\"))
      // Assume the path was valid under Nt.
      wpath[1] = L'?';
    return __nt_openfile(self, wpath, flags);
  }
} // namespace `anonymous`

IIOFile* FileAdaptor::openFileRaw(C::StrRef path, C::StrRef flags) {
  const auto F = IIOFile::ParseModeFlags(flags);
  if (F == IIOMode::Err) {
    err = Error::eInval;
    invals[1] = true;
  }
  if (path.isEmpty()) {
    err = Error::eInval;
    invals[0] = true;
  }
  if (err == Error::eInval)
    return nullptr;
  
  // Skip any normalization if the path is in this format.
  if (path.beginsWith("\\\\?\\")) {
    auto* win_file = __nt_openfile(*this, path, F);
    return static_cast<IIOFile*>(win_file);
  }
  
  PathNormalizer normalizer;
  // If the path could not be normalized, return.
  if (!normalizer(path)) {
    this->err = normalizer.getLastError();
    return nullptr;
  }

  // Open the normalized file with the parsed flags.
  return __nt_openfile(*this, normalizer.getPath(), F);
}

bool FileAdaptor::closeFileRaw(IIOFile* file) {
  const auto F = ptr_cast<WinIOFile>(file);
  if (!file_slots.inRange(F)) {
    err = Error::eBadFD;
    return false;
  }
  if (auto E = F->close(); E.isErr()) {
    err = E.err();
    return false;
  }
  const bool R = file_slots.eraseRaw(F);
  err = R ? Error::eNone : Error::eBadFD;
  return R;
}

void FileAdaptor::clearError() {
  invals[0] = false;
  invals[1] = false;
  invals[2] = false;
  err = Error::eNone;
}

//======================================================================//
// Free Functions
//======================================================================//

IOResult<IIOFile*> S::open_file(C::StrRef path, IIOFileBuf& buf, C::StrRef flags) {
  FileAdaptor F(buf);
  if (IIOFile* file = F.openFileRaw(path, flags))
    return $Ok(file);
  return $Err(F.getLastError());
}

IOResult<> S::close_file(IIOFile* file) {
  if (!file)
    return $Err(Error::eInval);
  FileAdaptor F(file->getFileBuf());
  if (F.closeFileRaw(file))
    return $Ok();
  return $Err(F.getLastError());
}

usize S::available_files() {
  return max_files - file_slots.accumulateCount();
}

//======================================================================//
// Platform Functions
//======================================================================//

FileResult S::win_file_read(IIOFile* file, common::AddrRange in) {
  return $FileErr(0);
}

FileResult S::win_file_write(IIOFile* file, common::ImmAddrRange out) {
  return $FileErr(0);
}

IOResult<long> S::win_file_seek(IIOFile* file, long offset, int) {
  __hc_unreachable("`win_file_seek` unimplemented.");
  return $Err(Error::eNone);
}

[[gnu::flatten]]
IOResult<> S::win_file_close(IIOFile* file) {
  return close_file(file);
}
