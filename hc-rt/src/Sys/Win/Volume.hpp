//===- Sys/Win/Volume.hpp -------------------------------------------===//
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

#include <Sys/Core/Nt/Info.hpp>
#include "Filesystem.hpp"

namespace hc::sys {
inline namespace __nt {
  template <win::is_fsinfo FSType, usize ExBytes = 0U>
  using NtQueryWrapper = win::FSInfoClassWrapper<FSType, ExBytes>;

  template <win::is_fsinfo FSType>
  __global usize __fsinf_exbytes = (FSType::isDynamic ? 16U : 0U);

  template <win::is_fsinfo FSType, 
    usize ExBytes = __fsinf_exbytes<FSType>>
  inline auto query_volume_info(
    win::FileObjHandle handle,
    win::IoStatusBlock& io) 
   -> NtQueryWrapper<FSType, ExBytes> {
    using RetType = NtQueryWrapper<FSType, ExBytes>;
    constexpr win::ULong ICWlen = sizeof(RetType);
    if (!handle) {
      // STATUS_INVALID_HANDLE
      io.status = 0xC0000008;
      return RetType{};
    }
    RetType ICW {};
    if constexpr (_HC_CHECK_INVARIANTS
     && win::__fsi_hasFAM<FSType>) {
      RetType* pICW = &ICW;
      __hc_invariant((*pICW)->FAM_len 
        == RetType::famSizeBytes);
    }
    io.status = isyscall<
     NtSyscall::QueryVolumeInformationFile>(
      handle.get(), &io,
      &ICW, ICWlen, ICW.GetInfoClass()
    );
    return ICW;
  }

  inline win::FileObjHandle get_volume_handle(
   win::UnicodeString& name,
   win::IoStatusBlock& io
  ) {
    // Check if we have a name, and if it ends in a backslash.
    if (name.backSafe() != L'\\') {
      // STATUS_OBJECT_NAME_INVALID
      io.status = 0xC0000033;
      return nullptr;
    }
    // See ReactOS's `kernel32/*/volume.c` for all opts.
    win::ObjectAttributes attr { .object_name = &name };
    auto mask   = NtAccessMask::Sync;
    auto fattr  = NtFileAttribMask::None;
    auto share  = NtFileShareMask::None;
    auto dispo  = NtCreateDisposition::Open;
    auto opts   = 
      NtCreateOptsMask::IsDirectory   |
      NtCreateOptsMask::SyncIONoAlert |
      NtCreateOptsMask::OpenForBackup;
    // Return a handle to the volume.
    return open_file(
      mask, attr, io, nullptr, 
      fattr, share, dispo, opts);
  }

  template <win::is_fsinfo FSType, 
    usize ExBytes = __fsinf_exbytes<FSType>>
  inline auto query_create_volume_info(
    win::UnicodeString& name,
    win::IoStatusBlock& io)
   -> NtQueryWrapper<FSType, ExBytes> {
    using RetType = NtQueryWrapper<FSType, ExBytes>;
    auto handle = get_volume_handle(name, io);
    if (!handle || $NtFail(io.status))
      return RetType{};
    // Now forward the handle.
    RetType ICW = query_volume_info<
      FSType, ExBytes>(handle, io);
    close_file(handle);
    return ICW;
  }
} // inline namespace __nt
} // namespace hc::sys
