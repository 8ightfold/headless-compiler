//===- VolumeInfoDumper.hpp -----------------------------------------===//
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

#include <Sys/Win/Volume.hpp>
#include <Sys/Win/Filesystem.hpp>
#include <Sys/Args.hpp>
// TODO: Remove this...
#include <cstdio>

#define MAX_PATH RT_STRICT_MAX_PATH
#define $ChkFlag(V, F) \
if (bool(V & F)) {  \
  std::printf("%s-%s\n", Prefix(), #F); \
}

using namespace hc;
namespace C = hc::common;
namespace S = hc::sys;
namespace W = hc::sys::win;

inline const char* commonNtErrors(W::NtStatus S) {
  return S::SysErr::GetErrorNameSafe(S);
}

struct VolumeInfoBinding {
  VolumeInfoBinding(
    W::FileObjHandle H,
    W::IoStatusBlock& IO) :
   handle(H), io(IO) { }
public:
  template <typename FSType, 
    usize N = S::__fsinf_exbytes<FSType>>
  auto load() {
    return S::query_volume_info<FSType, N>(handle, io);
  }

  bool failed(const char* S = nullptr) const {
    bool B = $NtFail(io.status);
    if (B && S) {
      std::printf("\e[0;31m`%s` failed with ", S);
      if (auto Err = commonNtErrors(io.status)) {
        std::printf("\e[1;93m");
        std::printf("%s\n", Err);
      } else {
        std::printf("[0x%.8X]\n", io.status);
      }
      std::printf("\e[0m");
    }
    io.status = 0x00000000;
    return B;
  }

public:
  W::FileObjHandle  handle;
  W::IoStatusBlock& io;
};

struct ScopedFileHandle {
  explicit ScopedFileHandle(
    W::FileObjHandle H) :
   __handle(H) { }
  
  ~ScopedFileHandle() {
    if (__handle)
      S::close_file(__handle);
  }

  operator W::FileObjHandle() const {
    return this->__handle;
  }
  explicit operator bool() const {
    return static_cast<bool>(this->__handle);
  }

private:
  W::FileObjHandle __handle;
};

inline const char* getPathType(S::PathType type) {
  using S::PathType;
  switch (type) {
   case PathType::GUIDVolume:   return "GUIDVolume";
   case PathType::DosDrive:     return "DosDrive";
   case PathType::DosVolume:    return "DosVolume";
   case PathType::DeviceUNC:    return "DeviceUNC";
   case PathType::UNCNamespace: return "UNCNamespace";
   case PathType::NtNamespace:  return "NtNamespace";
   case PathType::LegacyDevice: return "LegacyDevice";
   case PathType::QualDOS:      return "QualDOS";
   case PathType::DriveRel:     return "DriveRel";
   case PathType::CurrDriveRel: return "CurrDriveRel";
   case PathType::DirRel:       return "DirRel";
   default:                     return "Unknown";
  }
}

inline const char* getPathType(C::StrRef S) {
  return getPathType(S::PathNormalizer::GetPathType(S));
}

inline const char* Prefix() { return "  "; }

template <typename CharType>
bool hasPrintableCharacters(C::PtrRange<CharType> R) {
  for (CharType C : R) {
    if (C >= CharType(' ') && C <= CharType('~'))
      return true;
  }
  return false;
}

inline void printBoolean(W::Boolean V, const char* Ex = "") {
  if (Ex && *Ex)
    std::printf(" %s: ", Ex);
  std::printf("%s\n", (V != 0) ? "true" : "false");
}

inline void printLargeInt(W::LargeInt V, const char* Ex = "") {
  using LLIType = long long;
  if (Ex && *Ex)
    std::printf(" %s: ", Ex);
  std::printf("%lli\n", LLIType(V.quad));
}
inline void printLargeInt(W::ULargeInt V, const char* Ex = "") {
  using LLUType = unsigned long long;
  if (Ex && *Ex)
    std::printf(" %s: ", Ex);
  std::printf("%llu\n", LLUType(V.quad));
}

inline void printPtrRange(C::ImmPtrRange<char> R, const char* Ex = "") {
  if (R.isEmpty() || !hasPrintableCharacters(R)) return;
  if (Ex && *Ex)
    std::printf(" %s: ", Ex);
  std::printf("%.*s\n", int(R.size()), R.data());
}
inline void printPtrRange(C::ImmPtrRange<wchar_t> R, const char* Ex = "") {
  if (R.isEmpty() || !hasPrintableCharacters(R)) return;
  if (Ex && *Ex)
    std::printf(" %s: ", Ex);
  std::printf("%.*ls\n", int(R.size()), R.data());
}

inline void printPathType(C::StrRef S) {
  std::printf("%s: ", getPathType(S));
  printPtrRange(S);
}

inline void printVFSAttribMask(W::VFSAttribMask V) {
  using enum W::VFSAttribMask;
  std::printf(" -Filesystem Attributes:\n");
  $ChkFlag(V, CaseSensitiveSearch)
  $ChkFlag(V, CasePreservedNames)
  $ChkFlag(V, UnicodeOnDisk)
  $ChkFlag(V, PersistentACLs)
  $ChkFlag(V, FileCompression)
  $ChkFlag(V, VolumeQuotas)
  $ChkFlag(V, AllowsSparseFiles)
  $ChkFlag(V, AllowsReparsePoints)
  $ChkFlag(V, AllowsRemoteStorage)
  $ChkFlag(V, ReturnsCleanupInfo)
  $ChkFlag(V, AllowsPOSIXOps)
  $ChkFlag(V, VolumeIsCompressed)
  $ChkFlag(V, AllowsObjectIDs)
  $ChkFlag(V, AllowsEncryption)
  $ChkFlag(V, NamedStreams)
  $ChkFlag(V, ReadOnlyVolume)
  $ChkFlag(V, SequentialWriteOnce)
  $ChkFlag(V, AllowsTransactions)
  $ChkFlag(V, AllowsHardLinks)
  $ChkFlag(V, AllowsExAttribs)
  $ChkFlag(V, AllowsOpenByFileID)
  $ChkFlag(V, AllowsUSNJournal)
  $ChkFlag(V, AllowsIntegStreams)
  $ChkFlag(V, AllowsBlkRefCounts)
  $ChkFlag(V, AllowsSparseVDL)
  $ChkFlag(V, DAXVolume)
  $ChkFlag(V, AllowsGhosting)
}

inline void printVFSControlFlags(W::VFSControlMask V) {
  using enum W::VFSControlMask;
  std::printf(" -Control Flags:\n");
  $ChkFlag(V, QuotaNone)
  $ChkFlag(V, QuotaTrack)
  $ChkFlag(V, QuotaEnforce)
  $ChkFlag(V, ContentIdxDisabled)
  $ChkFlag(V, LogQuoteThreshold)
  $ChkFlag(V, LogQuotaLimit)
  $ChkFlag(V, LogVolumeThreshold)
  $ChkFlag(V, LogVolumeLimit)
  $ChkFlag(V, QuotasIncomplete)
  $ChkFlag(V, QuotasRebuilding)
}

inline void printControlInfo(W::FSControlInfo& V) {
  using LLUType = long long unsigned;
  std::printf("Volume Control Info:\n");
  std::printf(" Free Space: [%llu, %llu]\n",
    LLUType(V.free_space_end), LLUType(V.free_space_start));
  std::printf(" Free Space Threshold: %llu\n",
    LLUType(V.free_space_threshold));
  std::printf(" Quota Threshold: %llu\n",
    LLUType(V.default_quota_threshold));
  std::printf(" Quota Limit: %llu\n",
    LLUType(V.default_quota_limit));
  printVFSControlFlags(V.fs_control_flags);
}

inline const char* getDeviceTypeStr(W::DeviceType V) {
  using enum W::DeviceType;
  switch (V) {
   case UTCPPort:           return "UTCPPort";
   case ACPI:               return "ACPI";
   case Battery:            return "Battery";
   case Beep:               return "Beep";
   case BusExtender:        return "BusExtender";
   case CDRom:              return "CDRom";
   case CDRomFilesystem:    return "CDRomFilesystem";
   case Changer:            return "Changer";
   case Controller:         return "Controller";
   case Datalink:           return "Datalink";
   case DFS:                return "DFS";
   case DFSFilesystem:      return "DFSFilesystem";
   case DFSVolume:          return "DFSVolume";
   case Disk:               return "Disk";
   case DiskFilesystem:     return "DiskFilesystem";
   case DVD:                return "DVD";
   case Filesystem:         return "Filesystem";
   case FIPS:               return "FIPS";
   case FullscreenVideo:    return "FullscreenVideo";
   case InPort:             return "InPort";
   case Keyboard:           return "Keyboard";
   case KS:                 return "KS";
   case KSec:               return "KSec";
   case Mailslot:           return "Mailslot";
   case MassStorage:        return "MassStorage";
   case MIDIIn:             return "MIDIIn";
   case MIDIOut:            return "MIDIOut";
   case Modem:              return "Modem";
   case Mouse:              return "Mouse";
   case MultiUNCProvider:   return "MultiUNCProvider";
   case NamedPipe:          return "NamedPipe";
   case Network:            return "Network";
   case NetworkBrowser:     return "NetworkBrowser";
   case NetworkFilesystem:  return "NetworkFilesystem";
   case NetworkRedirector:  return "NetworkRedirector";
   case Null:               return "Null";
   case ParallelPort:       return "ParallelPort";
   case PhysNetcard:        return "PhysNetcard";
   case Printer:            return "Printer";
   case Scanner:            return "Scanner";
   case Screen:             return "Screen";
   case Serenum:            return "Serenum";
   case SerialMousePort:    return "SerialMousePort";
   case SerialPort:         return "SerialPort";
   case SmartCard:          return "SmartCard";
   case Sound:              return "Sound";
   case Streams:            return "Streams";
   case Tape:               return "Tape";
   case TapeFilesystem:     return "TapeFilesystem";
   case TerminalServer:     return "TerminalServer";
   case Transport:          return "Transport";
   case Unknown:            return "Unknown";
   case VirtualDosMachine:  return "VirtualDosMachine";
   case Video:              return "Video";
   case VirtualDisk:        return "VirtualDisk";
   case WaveIn:             return "WaveIn";
   case WaveOut:            return "WaveOut";
   default:                 return "Invalid";
  }
}

inline void printDeviceCharacteristics(W::VFSDeviceMask V) {
  using enum W::VFSDeviceMask;
  std::printf(" -Device Characteristics:\n");
  $ChkFlag(V, RemovableMedia)
  $ChkFlag(V, ReadOnlyDevice)
  $ChkFlag(V, FloppyDiskette)
  $ChkFlag(V, WriteOnceMedia)
  $ChkFlag(V, RemoteDevice)
  $ChkFlag(V, IsMounted)
  $ChkFlag(V, VirtualVolume)
  $ChkFlag(V, AutogennedName)
  $ChkFlag(V, SecureOpen)
  $ChkFlag(V, PNPDevice)
  $ChkFlag(V, TSDevice)
  $ChkFlag(V, WebDAVDevice)
  $ChkFlag(V, ClusterSharedVolume)
  $ChkFlag(V, AllowsTraversal)
  $ChkFlag(V, PortableDevice)
}

inline void printObjectUUID(W::GUID& UUID, bool upper = false) {
  const char* HexTable[2] = {
    "0123456789abcdef",
    "0123456789ABCDEF"
  };
  std::printf(" UUID: {%.8lx-%.4hx-%.4hx-",
    UUID.prefix, UUID.groupA, UUID.groupB);
  for (int I = 0; I < 8; ++I) {
    const u8 B = UUID.postfix[I];
    std::putchar(HexTable[upper][B & 0xF]);
    std::putchar(HexTable[upper][B >> 4]);
  }
  std::printf("}\n");
}

inline void printSectorSizeFlags(W::VFSSSizeMask V) {
  using enum W::VFSSSizeMask;
  std::printf(" Sector Size Flags:\n");
  $ChkFlag(V, AlignedDevice)
  $ChkFlag(V, PartAlignedOnDevice)
  $ChkFlag(V, NoSeekPenalty)
  $ChkFlag(V, TrimEnabled)
}

inline void printSectorSizeInfo(W::FSSectorSizeInfo& V) {
  std::printf(" Bytes Per Sector:\n");
  std::printf("  Logical  BPS: %u\n", V.logical_BPS);
  std::printf("  Physical BPS For Atomicity: %u\n", V.phys_BPS_atomicity);
  std::printf("  Physical BPS For Performance: %u\n", V.phys_BPS_perf);
  std::printf("  FS Effective Physical BPS For Atomicity: %u\n", V.fs_eff_phys_BPS_atomicity);
  std::printf(" Sector Align Offset: %u\n", V.sector_align_offset);
  std::printf(" Partition Align Offset: %u\n", V.partition_align_offset);
  printSectorSizeFlags(V.flags);
}


inline void printVolumeInfo(const wchar_t* drive_str) {
  wchar_t* const mdrive_str = const_cast<wchar_t*>(drive_str);
  auto name = W::UnicodeString::New(mdrive_str);
  W::IoStatusBlock io {};
  ScopedFileHandle handle(
    S::get_volume_handle(name, io));
  if (!handle || $NtFail(io.status)) {
    std::printf(
      "Unable to open volume \"%ls\".\n", 
      drive_str);
    return;
  }

  VolumeInfoBinding V(handle, io);
  std::printf("Info for \"%ls\":\n", drive_str);
  $scope {
    auto attr = V.load<W::FSAttributeInfo, 64>();
    if (V.failed("AttributeInfo")) break;
    std::printf("Volume Attributes:\n");
    printVFSAttribMask(attr->fs_attribs);
    printPtrRange(attr->intoRange(), "Filesystem Name");
  } $scope {
    auto ctrl = V.load<W::FSControlInfo>();
    if (V.failed("ControlInfo")) break;
    printControlInfo(ctrl);
  } $scope {
    auto dev = V.load<W::FSDeviceInfo, 8>();
    if (V.failed("DeviceInfo")) break;
    std::printf("Device Info:\n");
    std::printf(" Device Type: %s\n", 
      getDeviceTypeStr(dev->device_type));
    printDeviceCharacteristics(dev->characteristics);
  } $scope {
    auto drv = V.load<W::FSDriverPathInfo, MAX_PATH>();
    if (V.failed("DriverPath")) break;
    std::printf("Driver Path Info:\n");
    printBoolean(drv->in_path, "In Path");
    printPtrRange(drv->intoRange(), "Path");
  } $scope {
    auto obj = V.load<W::FSObjectIDInfo>();
    if (V.failed("ObjectID")) break;
    std::printf("Object ID:\n");
    printObjectUUID(obj->object_id);
    // std::printf(" Extended Info: ...\n");
    (void) obj->extended_info;
  } $scope {
    auto ssz = V.load<W::FSSectorSizeInfo>();
    if (V.failed("SectorSize")) break;
    std::printf("Sector Size Info:\n");
    printSectorSizeInfo(ssz);
  } $scope {
    auto sz = V.load<W::FSSizeInfo>();
    if (V.failed("Size")) break;
    std::printf("Volume Size:\n");
    printLargeInt(sz->total_alloc_units, "Total Allocation Units");
    printLargeInt(sz->alloc_units, "Active Allocation Units");
    std::printf(" Sectors Per Allocation Unit: %u\n",
      sz->sectors_per_alloc_unit);
    std::printf(" Bytes Per Sector: %u\n", sz->bytes_per_sector);
  } $scope {
    auto vol = V.load<W::FSVolumeInfo, MAX_PATH>();
    if (V.failed("Volume")) break;
    std::printf("Volume Info:\n");
    printLargeInt(vol->creation_time, "Creation Time");
    std::printf(" Serial Number: %u\n", vol->serial_number);
    printBoolean(vol->supports_objects, "Supports Objects");
    printPtrRange(vol->intoRange(), "Volume Name");
  }
}

inline void printVolumeInfo(const char* drive_string) {
  return printVolumeInfo($to_wstr(drive_string).data());
}
