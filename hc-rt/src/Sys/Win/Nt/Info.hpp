//===- Sys/Win/Nt/Volume.hpp ----------------------------------------===//
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
#pragma clang system_header

#include "Structs.hpp"
#include "Filesystem.hpp"

#define $AssocType(ty, val) \
  template <> \
  struct FSInfoAssoc<ty> {  \
    static constexpr FSInfoClass \
      value = FSInfoClass::val; \
  }
#define $DeclInfoType(ty) \
  template <> \
  struct IsFSInfoType<ty> {  \
    static constexpr bool value = true; \
  }
#define $DeclFSTy(name) \
 struct FS##name##Info; \
 $AssocType(FS##name##Info, name); \
 $DeclInfoType(FS##name##Info); \
 struct FS##name##Info
#define $FSIsDyn(B) static constexpr bool isDynamic = B
#define $FAMLen(name) \
 union [[gnu::packed]] { ULong name; ULong FAM_len; }
#define $FAMArr __counted_by(FAM_len) wchar_t
#define $FSIntoRange(name) \
 WNameRange intoRange() { \
  return WNameRange::New(this->name, \
    (this->FAM_len / sizeof(name[0]))); \
   }

namespace hc::sys::win {

enum class VFSAttribMask : ULong {
  CaseSensitiveSearch = 0x00000001,
  CasePreservedNames  = 0x00000002,
  UnicodeOnDisk       = 0x00000004,
  PersistentACLs      = 0x00000008,
  FileCompression     = 0x00000010,
  VolumeQuotas        = 0x00000020,
  AllowsSparseFiles   = 0x00000040,
  AllowsReparsePoints = 0x00000080,
  AllowsRemoteStorage = 0x00000100,
  ReturnsCleanupInfo  = 0x00000200,
  AllowsPOSIXOps      = 0x00000400,
  // ...
  VolumeIsCompressed  = 0x00008000,
  AllowsObjectIDs     = 0x00010000,
  AllowsEncryption    = 0x00020000,
  NamedStreams        = 0x00040000,
  ReadOnlyVolume      = 0x00080000,
  SequentialWriteOnce = 0x00100000,
  AllowsTransactions  = 0x00200000,
  AllowsHardLinks     = 0x00400000,
  AllowsExAttribs     = 0x00800000,
  AllowsOpenByFileID  = 0x01000000,
  AllowsUSNJournal    = 0x02000000,
  AllowsIntegStreams  = 0x04000000,
  AllowsBlkRefCounts  = 0x08000000,
  AllowsSparseVDL     = 0x10000000,
  DAXVolume           = 0x20000000,
  AllowsGhosting      = 0x40000000,
};

// enum class
enum class VFSControlMask : ULong {
  QuotaNone           = 0x00000,
  QuotaTrack          = 0x00001,
  QuotaEnforce        = 0x00002,
  ContentIdxDisabled  = 0x00008,
  LogQuoteThreshold   = 0x00010,
  LogQuotaLimit       = 0x00020,
  LogVolumeThreshold  = 0x00040,
  LogVolumeLimit      = 0x00080,
  QuotasIncomplete    = 0x00100,
  QuotasRebuilding    = 0x00200,
};

enum class VFSDeviceMask : ULong {
  RemovableMedia      = 0x00001,
  ReadOnlyDevice      = 0x00002,
  FloppyDiskette      = 0x00004,
  WriteOnceMedia      = 0x00008,
  RemoteDevice        = 0x00010,
  IsMounted           = 0x00020,
  VirtualVolume       = 0x00040,
  AutogennedName      = 0x00080,
  SecureOpen          = 0x00100,
  PNPDevice           = 0x00800,
  TSDevice            = 0x01000,
  WebDAVDevice        = 0x02000,
  ClusterSharedVolume = 0x10000,
  AllowsTraversal     = 0x20000,
  PortableDevice      = 0x40000,
};

enum class VFSSSizeMask : ULong {
  AlignedDevice       =	0b0001,
  PartAlignedOnDevice = 0b0010,
  NoSeekPenalty       =	0b0100,
  TrimEnabled         =	0b1000,
};

$MarkBitwise(VFSAttribMask);
$MarkBitwise(VFSControlMask);
$MarkBitwise(VFSDeviceMask);
$MarkBitwise(VFSSSizeMask);

//======================================================================//
// Classes
//======================================================================//

enum class DeviceType : ULong {
  // Normal
  _8042Port           = 0x0027,
  ACPI                = 0x0032,
  Battery             = 0x0029,
  Beep                = 0x0001,
  BusExtender         = 0x002a,
  CDRom               = 0x0002,
  CDRomFilesystem     = 0x0003,
  Changer             = 0x0030,
  Controller          = 0x0004,
  Datalink            = 0x0005,
  DFS                 = 0x0006,
  DFSFilesystem       = 0x0035,
  DFSVolume           = 0x0036,
  Disk                = 0x0007,
  DiskFilesystem      = 0x0008,
  DVD                 = 0x0033,
  Filesystem          = 0x0009,
  FIPS                = 0x003a,
  FullscreenVideo     = 0x0034,
  InPort              = 0x000a,
  Keyboard            = 0x000b,
  KS                  = 0x002f,
  KSec                = 0x0039,
  Mailslot            = 0x000c,
  MassStorage         = 0x002d,
  MIDIIn              = 0x000d,
  MIDIOut             = 0x000e,
  Modem               = 0x002b,
  Mouse               = 0x000f,
  MultiUNCProvider    = 0x0010,
  NamedPipe           = 0x0011,
  Network             = 0x0012,
  NetworkBrowser      = 0x0013,
  NetworkFilesystem   = 0x0014,
  NetworkRedirector   = 0x0028,
  Null                = 0x0015,
  ParallelPort        = 0x0016,
  PhysNetcard         = 0x0017,
  Printer             = 0x0018,
  Scanner             = 0x0019,
  Screen              = 0x001c,
  Serenum             = 0x0037,
  SerialMousePort     = 0x001a,
  SerialPort          = 0x001b,
  SmartCard           = 0x0031,
  Sound               = 0x001d,
  Streams             = 0x001e,
  Tape                = 0x001f,
  TapeFilesystem      = 0x0020,
  TerminalServer      = 0x0038,
  Transport           = 0x0021,
  Unknown             = 0x0022,
  VirtualDosMachine   = 0x002c,
  Video               = 0x0023,
  VirtualDisk         = 0x0024,
  WaveIn              = 0x0025,
  WaveOut             = 0x0026,

  // Extended
  InfiniBand          = 0x003b,
  VMBus               = 0x003e,
  CryptProvider       = 0x003f,
  WPD                 = 0x0040,
  BlueTooth           = 0x0041,
  MT_Composite        = 0x0042,
  MT_Transport        = 0x0043,
  Biometric           = 0x0044,
  PMI                 = 0x0045,
  EnhancedStorage     = 0x0046,
  DevAPI              = 0x0047,
  GPIO                = 0x0048,
  USBEx               = 0x0049,
  Console             = 0x0050,
  NearFieldProximity  = 0x0051,
  SystemEnv           = 0x0052,
  VirtualBlock        = 0x0053,
  PointOfService      = 0x0054,
  StorageReplication  = 0x0055,
  TrustedEnv          = 0x0056,
  UCM                 = 0x0057,
  UCM_TCPCI           = 0x0058,
  PersistentMemory    = 0x0059,
  NV_DIMM             = 0x005a,
  Holographic         = 0x005b, // ?
  SDFXHCI             = 0x005c,

  // Old Names
  EHSTOR              = 0x0046, // EnhancedStorage
  USBEX               = 0x0049,
  NFP                 = 0x0051,
};

struct [[gnu::packed]] GUID {
  u32 prefix;
  u16 groupA;
  u16 groupB;
  u8  postfix[8];
};

//======================================================================//
// Definitions
//======================================================================//

template <typename>
struct FSInfoAssoc {
  $compile_failure(FSInfoAssoc,
    "Invalid filesystem info class.");
};

template <typename>
struct IsFSInfoType {
  static constexpr bool value = false;
};

$DeclFSTy(Attribute) {
  $FSIsDyn(true);
  VFSAttribMask fs_attribs;
  Long max_component_name_len;
  $FAMLen(name_len);
  $FAMArr fs_name[];
public:
  $FSIntoRange(fs_name)
};

$DeclFSTy(Control) {
  $FSIsDyn(false);
  LargeInt free_space_start;
  LargeInt free_space_threshold;
  LargeInt free_space_end;
  LargeInt default_quota_threshold;
  LargeInt default_quota_limit;
  VFSControlMask fs_control_flags;
};

$DeclFSTy(Device) {
  $FSIsDyn(false);
  DeviceType device_type;
  VFSDeviceMask characteristics;
};

$DeclFSTy(DriverPath) {
  $FSIsDyn(true);
  Boolean in_path;
  $FAMLen(name_len);
  $FAMArr driver_name[];
public:
  $FSIntoRange(driver_name)
};

$DeclFSTy(FullSize) {
  // AU:  allocation_unit[s]
  // AAU: available_AU
  $FSIsDyn(false);
  LargeInt total_AU;
  LargeInt caller_AAU;
  LargeInt actual_AAU;
  ULong    sectors_per_AU;
  ULong    bytes_per_sector;
};

$DeclFSTy(ObjectID) {
  $FSIsDyn(false);
  GUID object_id;
  ubyte extended_info[48];
};

$DeclFSTy(SectorSize) {
  // BPS: bytes_per_sector[_for]*
  $FSIsDyn(false);
  ULong logical_BPS;
  ULong phys_BPS_atomicity;
  ULong phys_BPS_perf;
  ULong fs_eff_phys_BPS_atomicity;
  VFSSSizeMask flags;
  ULong sector_align_offset;
  ULong partition_align_offset;
};

$DeclFSTy(Size) {
  $FSIsDyn(false);
  LargeInt total_alloc_units;
  LargeInt alloc_units;
  ULong    sectors_per_alloc_unit;
  ULong    bytes_per_sector;
};

$DeclFSTy(Volume) {
  $FSIsDyn(true);
  LargeInt creation_time;
  ULong    serial_number;
  $FAMLen(label_len);
  Boolean  supports_objects;
  $FAMArr  volume_name[];
public:
  $FSIntoRange(volume_name)
};

template <typename T>
concept is_fsinfo = IsFSInfoType<T>::value;

template <typename FSInfoType>
concept __fsi_hasFAM = requires {
  FSInfoType::FAM_len;
};

template <is_fsinfo FSInfoType>
__global FSInfoClass fsInfoAssoc 
  = FSInfoAssoc<FSInfoType>::value;

//======================================================================//
// Wrapper
//======================================================================//

template <typename FSInfoType, usize ExBytes = 0U>
constexpr FSInfoType __fsi_FAM_init() {
  if constexpr (__fsi_hasFAM<FSInfoType>)
    return {.FAM_len = ExBytes * sizeof(wchar_t)};
  return {};
}

template <typename FSInfoType>
__always_inline constexpr usize
 __fsi_FAM_get(FSInfoType& V) {
  if constexpr (__fsi_hasFAM<FSInfoType>)
    return V.FAM_len;
  return 0;
}

template <is_fsinfo FSInfoType, usize ExBytes = 0U>
struct FSInfoClassWrapper {
  using SelfType = FSInfoClassWrapper;
  static constexpr usize famSize = ExBytes;
  static constexpr usize famSizeBytes = famSize * sizeof(wchar_t);
  static constexpr auto infoClass = fsInfoAssoc<FSInfoType>;
public:
  static FSInfoClass GetInfoClass() { return infoClass; }
  usize size() const { return famSize; }
  usize sizeInBytes() const { return famSizeBytes; }
  FSInfoType* operator->() { return &body; }
  const FSInfoType* operator->() const { return &body; }
  operator FSInfoType&() { return body; }
  operator const FSInfoType&() const { return body; }
public:
  FSInfoType body = __fsi_FAM_init<FSInfoType, ExBytes>();
  wchar_t __ex_data[ExBytes];
};

template <is_fsinfo FSInfoType>
struct FSInfoClassWrapper<FSInfoType, 0U> {
  static_assert(!FSInfoType::isDynamic,
    "This type has a flexible array member, "
    "please explicitly specify a buffer size.");
  using SelfType = FSInfoClassWrapper;
  static constexpr usize famSize = 0U;
  static constexpr usize famSizeBytes = 0U;
  static constexpr auto infoClass = fsInfoAssoc<FSInfoType>;
public:
  static FSInfoClass GetInfoClass() { return infoClass; }
  usize size() const { return famSize; }
  usize sizeInBytes() const { return famSizeBytes; }
  FSInfoType* operator->() { return &body; }
  const FSInfoType* operator->() const { return &body; }
  operator FSInfoType&() { return body; }
  operator const FSInfoType&() const { return body; }
public:
  FSInfoType body = __fsi_FAM_init<FSInfoType>();
};

} // namespace hc::sys::win

#undef $AssocType
#undef $DeclFSTy
#undef $FSIsDyn
#undef $FAMLen
#undef $FAMArr
#undef $FSIntoRange
