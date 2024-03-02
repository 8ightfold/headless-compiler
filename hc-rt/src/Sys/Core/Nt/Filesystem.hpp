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

#include "Structs.hpp"

namespace hc::sys::win {
  enum class CreateDisposition : ULong {
    Supersede           = 0x00000,
    Open                = 0x00001,
    Create              = 0x00010,
    OpenIf              = 0x00011,
    Overwrite           = 0x00100,
    OverwriteIf         = 0x00101,
    MaxValue
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
    OpenByFileID        = 0x002000,
    OpenForBackup       = 0x004000,
    OpenForSpaceQuery   = 0x008000,
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

  $MarkBitwise(CreateOptsMask)
  $MarkBitwise(FileAttribMask)
  $MarkBitwise(FileShareMask)

  inline constexpr AccessMask GenericReadAccess =
    AccessMask::StdRightsRead | AccessMask::ReadData 
   | AccessMask::ReadAttributes | AccessMask::ReadEA | AccessMask::Sync;

  inline constexpr AccessMask GenericWriteAccess =
    AccessMask::StdRightsWrite | AccessMask::WriteData 
   | AccessMask::WriteAttributes | AccessMask::WriteEA | AccessMask::Sync;
  
  inline constexpr AccessMask GenericExecAccess =
    AccessMask::StdRightsExec | AccessMask::Execute
   | AccessMask::ReadAttributes | AccessMask::Sync;

  //====================================================================//
  // Info Classes
  //====================================================================//

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

  enum class FSInfoClass {
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
    // ...
    SectorSize,
    DataCopy,
    MetadataSize,
    FullSizeEx,
    Guid,
    MaxValue
  };

  //====================================================================//
  // Misc.
  //====================================================================//

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
