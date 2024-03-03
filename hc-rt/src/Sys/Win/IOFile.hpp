//===- Sys/Win/IOFile.hpp -------------------------------------------===//
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

#pragma once

#include <Sys/IOFile.hpp>
#include <Sys/Core/Nt/Filesystem.hpp>

namespace hc::sys {
  __global wchar_t consoleFileName[] = L"CON";
  __global wchar_t consoleInpFileName[] = L"$CONIN";
  __global wchar_t consoleOutFileName[] = L"$CONOUT";

  FileResult     win_file_read(IIOFile* file, common::AddrRange in);
  FileResult     win_file_write(IIOFile* file, common::ImmAddrRange out);
  IOResult<long> win_file_seek(IIOFile* file, long offset, int);
  IOResult<>     win_file_close(IIOFile* file);

  struct WinIOFile : IIOFile {
    constexpr WinIOFile(int fd,
      IIOFileBuf& buf, BufferMode buf_mode,
      IIOMode mode, bool is_owned = false) :
     IIOFile(
      &win_file_read, &win_file_write,
      &win_file_seek, &win_file_close,
      buf, buf_mode, mode, is_owned),
     fd(fd), io_block() { }

  public:
    int fd;
    win::IoStatusBlock io_block;
  };
} // namespace hc::sys