//===- Sys/File.hpp -------------------------------------------------===//
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
//  Public API for file operations. Implementation is in IOFile.hpp.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/PtrRange.hpp>
#include <Common/Result.hpp>
#include <Common/StrRef.hpp>
#include <Std/cstdio>
#include <Sys/Errors.hpp>

namespace hc {
namespace sys {

template <typename T = void>
using IOResult = com::Result<T, Error>;

struct FileAdaptor {
  enum { invalArgMax = 3 };
  using InvalArgsType = bool(&)[invalArgMax];
public:
  // TODO:
  constexpr FileAdaptor(IIOFileBuf& buf) : buf(&buf) { (void) this->buf; }
  /// Opens a file, same flag syntax as `std::fopen`'s extended mode.
  IIOFile* openFileRaw(com::StrRef path, com::StrRef flags);
  /// Closes a file, returns `true` if handle was valid.
  bool closeFileRaw(IIOFile* file);
  /// Returns the last error, if there was one.
  Error getLastError() const { return err; }
  /// Returns the positions for `Error::eInval`.
  /// Indexes are `true` if they were invalid.
  auto invalArgsPos() const
   -> const bool(&)[invalArgMax] { 
    return invals;
  }
  /// Resets any error info.
  void clearError();
private:
  IIOFileBuf* buf;
  Error err = Error::eNone;
  bool invals[invalArgMax] {};
};

/// Opens a file, same flag syntax as `std::fopen`'s extended mode.
IOResult<IIOFile*> open_file(com::StrRef path, IIOFileBuf& buf, com::StrRef flags);
/// Closes a file, returns `true` if handle was valid.
IOResult<> close_file(IIOFile* file);
/// Returns the number of open file slots.
usize available_files();

} // namespace sys

using RawIOFile = sys::IIOFile;
using IOFile    = RawIOFile*;

//======================================================================//
// Standard File Handles
//======================================================================//

namespace sys {
extern constinit IIOFile* pout;
extern constinit IIOFile* perr;
extern constinit IIOFile* pin;
} // namespace sys

using sys::pout;
using sys::perr;
using sys::pin;

// TODO: Temporary example
sys::IOResult<int> write_file(IOFile file, com::StrRef data);

} // namespace hc
