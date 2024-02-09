//===- Sys/Windows/NtFilesystem.hpp ---------------------------------===//
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

#include <Common/EnumBitwise.hpp>
#include "NtGeneric.hpp"
#include "NtHandles.hpp"

namespace hc::sys::win {
  enum class AccessMask : ULong {
    ReadData          = 0x000001UL,
    ReadEA            = 0x000008UL,
    ReadAttributes    = 0x000080UL,
    ReadControl       = 0x020000UL,

    WriteData         = 0x000002UL,
    WriteEA           = 0x000010UL,
    WriteDAC          = 0x040000UL,
    WriteOwner        = 0x080000UL,

    Delete            = 0x010000UL,
    Execute           = 0x000020UL,
    Sync              = 0x100000UL,

    StdRightsRequired = 0x0F0000UL,
    StdRightsRead     = ReadControl,
    StdRightsWrite    = ReadControl,
    StdRightsExec     = ReadControl,

    StdRightsAll      = 0x1F0000UL,
    SpRightsAll       = 0x00FFFFUL,
  };

  struct AccessMaskSpecific {
    static constexpr ULong max = ULong(AccessMask::SpRightsAll);
    static constexpr ULong npos = max + 1;
  public:
    __always_inline constexpr operator AccessMask() const {
      __hc_invariant(data < npos);
      return AccessMask(this->data);
    }
    __ndbg_inline explicit constexpr operator ULong() const {
      return this->data;
    }
  public:
    ULong data = max;
  };

  enum class FileAttribMask : ULong {
    ReadOnly            = 0x00001,
    Hidden              = 0x00002,
    System              = 0x00004,
    Directory           = 0x00010,
    Archive             = 0x00020,
    Device              = 0x00040,
    Normal              = 0x00080,
    Temporary           = 0x00100,
    SparseFile          = 0x00200,
    ReparsePoint        = 0x00400,
    Compressed          = 0x00800,
    Offline             = 0x01000,
    NotContentIndexed   = 0x02000,
    Encrypted           = 0x04000,
    IntegrityStream     = 0x08000,
    NoScrubData         = 0x20000,
  };

  enum class ObjAttribMask : ULong {
    Inherit             = 0x0002UL,
    Permanent           = 0x0010UL,
    Exclusive           = 0x0020UL,
    CaseInsensitive     = 0x0040UL,
    OpenIf              = 0x0080UL,
    OpenLink            = 0x0100UL,
    KernelHandle        = 0x0200UL,
    ForceAccessCheck    = 0x0400UL,
    IgnoreImpDeviceMap  = 0x0800UL,
    DoNotReparse        = 0x1000UL,
    __ValidAttributes   = 0x1FF2UL,
  };

  $MarkBitwise(AccessMask)
  $MarkBitwise(FileAttribMask)
  $MarkBitwise(ObjAttribMask)
  $MarkBitwiseEx(AccessMaskSpecific, ULong)

  //=== Info Classes ===//

  // TODO: Implement buffer shit
  enum class FileInfoClass {
    Directory = 1,
    FullDirectory, BothDirectory,
    Basic, Standard, Internal,
    EA,
    Access,
    Name, Rename, Link, Names,
    Disposition, Position,
    FullEA,
    Mode,
    Alignment,
    All,
    Allocation,
    EndOfFile,
    AlternateName,
    Stream, Pipe, PipeLocal, PipeRemote,
    MailslotQuery, MailslotSet,
    Compression,
    CopyOnWrite,
    Completion,
    MoveCluster,
    Quota,
    ReparsePoint,
    NetworkOpen,
    ObjectID,
    Tracking,
    OleDirectory,
    ContentIndex, InheritContentIndex,
    Ole, // Deprecated
    MaxValue
  };

  enum class FilesystemInfoClass {
    Volume = 1,
    Label,
    Size,
    Device,
    Attribute,
    Control,
    FullSize,
    ObjectID,
    DriverPath,
    VolumeFlags,
    SectorSize,
    DataCopy,
    MetadataSize,
    FullSizeInformationEx,
    Guid,
    MaxValue
  };

  //=== Misc. ===//

  struct IoStatusBlock {
    union {
      NtStatus status;
      void*    pad;
    };
    uptr info;
  };

  /// Reserved in most Nt functions.
  using IOAPCRoutinePtr = void(__stdcall*)(
    void* apc_ctx, IoStatusBlock* status, ULong);

  struct ObjectAttributes {
    ULong length  = sizeof(ObjectAttributes);
    FileObjHandle   root_directory = nullptr;
    UnicodeString*  object_name = nullptr;
    ObjAttribMask   attributes = ObjAttribMask::CaseInsensitive;
    void*           security_descriptor = nullptr;
    void*           security_QOS = nullptr;
  };

  struct BasicFileInfo {
    LargeInt        creation_time;
    LargeInt        last_access_time;
    LargeInt        last_write_time;
    LargeInt        change_time;
    FileAttribMask  file_attributes;
  };

  union FileSegmentElement {
    void* buffer;
    uptr  alignment;
  };
} // namespace hc::sys::win

namespace hc::sys {
  using ReadBuffer = common::PtrRange<char>;

  inline win::FileObjHandle __stdcall open_file(
   NtAccessMask mask, win::ObjectAttributes& attr,
   win::IoStatusBlock& io, win::LargeInt* alloc_size,
   NtFileAttribMask file_attr, win::ULong share_access,
   win::ULong create_disposition, win::ULong create_opts,
   void* ea_buffer = nullptr, win::ULong ea_len = 0UL)
  {
    win::FileObjHandle hout;
    win::NtStatus S = isyscall<NtSyscall::CreateFile>(
      &hout, mask, &attr, &io, alloc_size, 
      file_attr, share_access, 
      create_disposition, create_opts,
      ea_buffer, ea_len
    );
    return hout;
  }

  inline win::NtStatus __stdcall read_file(
   win::FileHandle handle, win::EventHandle event,
   win::IOAPCRoutinePtr apc, void* apc_ctx,
   win::IoStatusBlock& io, ReadBuffer buf, 
   win::LargeInt* offset = nullptr, 
   win::ULong* key = nullptr)
  {
    if (!handle) return -1;
    const usize buf_size = buf.size();
    return isyscall<NtSyscall::ReadFile>(
      handle.__data, event, apc, apc_ctx,
      &io, buf.data(), win::ULong(!buf_size ? 0 : (buf_size - 1)),
      offset, key
    );
  }

  __always_inline win::NtStatus __stdcall read_file(
   win::FileHandle handle, 
   win::IoStatusBlock& io, ReadBuffer buf, 
   win::LargeInt* offset = nullptr,
   win::ULong* key = nullptr) 
  {
    return read_file(
      handle, win::EventHandle::New(nullptr),
      win::IOAPCRoutinePtr(nullptr), nullptr, 
      io, buf, 
      offset, key
    );
  }

  __always_inline win::NtStatus __stdcall
   close(win::FileObjHandle handle) {
    if (!handle) return -1;
    return isyscall<NtSyscall::Close>(handle.__data);
  }
} // namespace hc::sys
