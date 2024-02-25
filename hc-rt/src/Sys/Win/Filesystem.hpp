//===- Sys/Win/Filesystem.hpp ---------------------------------------===//
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

#include <Sys/Core/Nt/Filesystem.hpp>

namespace hc::sys {
inline namespace __nt {
  inline win::FileObjHandle open_file(
   NtAccessMask mask,
   win::ObjectAttributes& attr,
   win::IoStatusBlock& io, 
   win::LargeInt* alloc_size,
   NtFileAttribMask file_attr, 
   NtFileShareMask share_access,
   NtCreateDisposition disposition, 
   NtCreateOptsMask create_opts 
    = NtCreateOptsMask::IsFile
  ) {
    win::FileObjHandle hout;
    io.status = isyscall<NtSyscall::CreateFile>(
      &hout, mask, &attr, &io, alloc_size, 
      file_attr, share_access, 
      disposition, create_opts,
      nullptr, win::ULong(0UL)
    );
    return hout;
  }

  inline win::NtStatus read_file(
   win::FileHandle handle,
   win::IoStatusBlock& io, 
   common::PtrRange<char> buf, 
   win::LargeInt* poffset = nullptr,
   win::ULong* key = nullptr
  ) {
    const usize buf_size = buf.size();
    win::LargeInt offset {};
    if (!poffset) poffset = &offset;
    return isyscall<NtSyscall::ReadFile>(
      $unwrap_handle(handle), win::EventHandle::New(nullptr),
      win::IOAPCRoutinePtr(nullptr), nullptr,
      &io, buf.data(), win::ULong(!buf_size ? 0 : (buf_size - 1)),
      poffset, key
    );
  }

  __always_inline win::NtStatus close_file(
   win::FileObjHandle handle
  ) {
    return isyscall<NtSyscall::Close>(
      $unwrap_handle(handle));
  }
} // inline namespace __nt
} // namespace hc::sys
