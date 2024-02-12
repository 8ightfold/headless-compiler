//===- Sys/Core/Nt/Filesystem.hpp -----------------------------------===//
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
#include "Generic.hpp"
#include "Handles.hpp"

namespace hc::sys::win {
  enum class AccessMask : ULong {
    ReadData          = 0x000001,
    ReadAttributes    = 0x000080,
    ReadEA            = 0x000008,
    ReadControl       = 0x020000,

    WriteData         = 0x000002,
    WriteAttributes   = 0x000100,
    WriteEA           = 0x000010,
    WriteDAC          = 0x040000,
    WriteOwner        = 0x080000,

    Delete            = 0x010000,
    Execute           = 0x000020,
    Sync              = 0x100000,

    StdRightsRequired = 0x0F0000,
    StdRightsRead     = ReadControl,
    StdRightsWrite    = ReadControl,
    StdRightsExec     = ReadControl,

    StdRightsAll      = 0x1F0000,
    SpRightsAll       = 0x00FFFF,
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
    ULong data = 0UL;
  };

  enum class CreateDisposition : ULong {
    Supersede           = 0x00000,
    Open                = 0x00001,
    Create              = 0x00010,
    OpenIf              = 0x00011,
    Overwrite           = 0x00100,
    OverwriteIf         = 0x00101,
    MaxValue            = OverwriteIf,
  };

  enum class CreateOptsMask : ULong {
    IsDirectory         = 0x000001,
    IsFile              = 0x000040,
    SequentialOnly      = 0x000004,
    RandomAccess        = 0x000800,
    WriteThrough        = 0x000002,
    NoBuffering         = 0x000008,
    SyncIOAlert         = 0x000010,
    SyncIONoAlert       = 0x000020,
    DeleteOnClose       = 0x001000,
    ReserveOplock       = 0x100000,
  };

  enum class FileAttribMask : ULong {
    None                = 0x00000,
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

  enum class FileShareMask : ULong {
    None                = 0b00000,
    Read                = 0b00001,
    Write               = 0b00010,
    Delete              = 0b00100,
    All                 = 0b00111,
  };

  enum class ObjAttribMask : ULong {
    Inherit             = 0x00002,
    Permanent           = 0x00010,
    Exclusive           = 0x00020,
    CaseInsensitive     = 0x00040,
    OpenIf              = 0x00080,
    OpenLink            = 0x00100,
    KernelHandle        = 0x00200,
    ForceAccessCheck    = 0x00400,
    IgnoreImpDeviceMap  = 0x00800,
    DoNotReparse        = 0x01000,
    __ValidAttributes   = 0x01FF2,
  };

  $MarkBitwise(AccessMask)
  $MarkBitwise(CreateOptsMask)
  $MarkBitwise(FileAttribMask)
  $MarkBitwise(FileShareMask)
  $MarkBitwise(ObjAttribMask)
  $MarkBitwiseEx(AccessMaskSpecific, ULong)

  inline constexpr AccessMask GenericReadAccess =
    AccessMask::StdRightsRead | AccessMask::ReadData 
   | AccessMask::ReadAttributes | AccessMask::ReadEA | AccessMask::Sync;

  inline constexpr AccessMask GenericWriteAccess =
    AccessMask::StdRightsWrite | AccessMask::WriteData 
   | AccessMask::WriteAttributes | AccessMask::WriteEA | AccessMask::Sync;
  
  inline constexpr AccessMask GenericExecAccess =
    AccessMask::StdRightsExec | AccessMask::Execute
   | AccessMask::ReadAttributes | AccessMask::Sync;

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
    explicit operator bool() const {
      return $NtSuccess(this->status);
    }
  public:
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
