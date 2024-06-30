//===- Sys/Win/PathNormalizer.cpp -----------------------------------===//
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

#include <Common/InlineMemcpy.hpp>
#include <Common/MMatch.hpp>
#include <Common/Option.hpp>
#include <Common/TaggedUnion.hpp>
#include <Bootstrap/Win64KernelDefs.hpp>
// #include <Parcel/StringTable.hpp>
#include <Parcel/StringTable2.hpp>
#include <Sys/Args.hpp>
#include <Sys/Win/Volume.hpp>
#include "PathNormalizer.hpp"

// For more info:
// https://learn.microsoft.com/en-us/dotnet/standard/io/file-path-formats
// https://superuser.com/questions/368602/difference-between-netbios-host-name-and-computer-name
// https://en.wikipedia.org/wiki/IPv6
// https://en.wikipedia.org/wiki/Fully_qualified_domain_name


#ifndef __XCRT__
# include <cstdio>
# define $printf(...) std::printf(__VA_ARGS__)
#else
# define $printf(...) (void(0))
#endif

using namespace hc;
using namespace hc::sys;
namespace B = hc::bootstrap;
namespace P = hc::parcel;
namespace S = hc::sys;

enum class S::UNCPrefixType : u32 {
  Unknown,
  HostName,
  NetBIOS,
  IPv4,
  IPv6,
  FQDN,
};

struct S::PathDeductionCtx {

};

//======================================================================//
// Identification
//======================================================================//

namespace {
  constexpr usize maxPathSlices = 64;
  using PathSliceType = P::StaticVec<StrRef, maxPathSlices>;

  inline bool __is_control(const char C) {
    return (C < ' ' || C == '\x7F');
  }

  __always_inline bool __is_upper(const char C) {
    return (C >= 'A' && C <= 'Z');
  }

  __always_inline bool __is_lower(const char C) {
    return (C >= 'a' && C <= 'z');
  }

  __always_inline bool __is_alpha(const char C) {
    return __is_upper(C) || __is_lower(C);
  }

  __always_inline bool __is_numeric(const char C) {
    return (C >= '0' && C <= '9');
  }

  inline bool __is_alnum(const char C) {
    return __is_alpha(C) || __is_numeric(C);
  }

  inline bool __is_hex_upper(const char C) {
    return __is_numeric(C) || (C >= 'A' && C <= 'F');
  }

  inline bool __is_hex_lower(const char C) {
    return __is_numeric(C) || (C >= 'a' && C <= 'f');
  }

  __always_inline bool __is_hex(const char C) {
    return __is_numeric(C)   || 
      (C >= 'A' && C <= 'F') ||
      (C >= 'a' && C <= 'f');
  }

  inline bool __is_special_pchar(const char C) {
    return __is_control(C) || MMatch(C).is(
     '"', '*', '/', ':', '<', '>', '?', '|');
  }

  inline bool __is_valid_pchar(const char C) {
    if __expect_true(__is_alnum(C))
      return true;
    return (C != '\\' && !__is_special_pchar(C));
  }

#define $ConsumeMultiChars(path, chars...) do { \
 if __expect_false(!path.consumeFront(chars)) \
  return false; \
} while (0)
#define $ConsumeDirSep(path) do { \
 if __expect_false(!path.consumeFront('/', '\\')) \
  return false; \
} while (0)

  ////////////////////////////////////////////////////////////////////////
  /// Checks if path begins with `(//|\\)[.?][/\]`.
  inline bool is_device_path(StrRef path) {
    $ConsumeDirSep(path);
    $ConsumeDirSep(path);
    $ConsumeMultiChars(path, '.', '?');
    return path.beginsWith('/', '\\');
  }

  ////////////////////////////////////////////////////////////////////////
  /// Checks if path begins with `[a-zA-Z]:`.
  inline bool is_volume(StrRef path) {
    if (!__is_alpha(path.frontSafe()))
      return false;
    return path.dropFront().beginsWith(':');
  }

  ////////////////////////////////////////////////////////////////////////
  /// Consumes a GUID hex block.
  template <usize N, bool ConsumeSep = true>
  bool consume_guid_hex(StrRef& path) {
    if __expect_false(path.size() < (N + ConsumeSep))
      return false;
    if constexpr (ConsumeSep) {
      if (path[N] != '-')
        return false;
    }
    for (usize I = 0; I < N; ++I) {
      if __expect_true(__is_hex(path[I]))
        continue;
      return false;
    }
    
    path.dropFrontMut(N);
    if constexpr (ConsumeSep)
      return path.consumeFront('-');
    else
      return true;
  }

#define $ConsumeGUIDHex(path, size) do { \
 if __expect_false(!consume_guid_hex<size>(path)) \
  return false; \
} while (0)

  /// Consumes a GUID.
  inline bool consume_guid_inner(StrRef& path) {
    // 32 hex + 4 sep
    if __expect_false(path.size() <= 36)
      return false;
    $ConsumeGUIDHex(path, 8);
    $ConsumeGUIDHex(path, 4);
    $ConsumeGUIDHex(path, 4);
    $ConsumeGUIDHex(path, 4);
    return consume_guid_hex<12, false>(path);
  }

#undef $ConsumeGUIDHex

  /// Checks if is `Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}`.
  inline bool is_volume_guid(StrRef path) {
    if (!path.consumeFront("Volume{"))
      return false;
    if (!consume_guid_inner(path))
      return false;
    return path.beginsWith('}');
  }

  ////////////////////////////////////////////////////////////////////////
  /// Checks if path begins with `[/\]??[/\]`.
  inline bool is_nt_path(StrRef path) {
    $ConsumeDirSep(path);
    path.consumeFront("GLOBAL");
    $ConsumeMultiChars(path, "??");
    return path.beginsWith('/', '\\');
  }

  ////////////////////////////////////////////////////////////////////////
  inline UNCPrefixType consume_unc_ipv4(StrRef& path) {
    const auto eat_number = [&path] -> bool {
      usize val = 0;
      if __expect_false(path.consumeUnsigned(val)) {
        __hc_unreachable("Not a number.");
        return false;
      } else if __expect_false(val > 255) {
        __hc_unreachable("Out of range.");
        return false;
      }
      return true;
    };
    for (int I = 0; I < 3; ++I) {
      if __expect_false(!eat_number())
        return UNCPrefixType::Unknown;
      if (!path.consumeFront('.'))
        return UNCPrefixType::Unknown;
    }
    if (!eat_number())
      return UNCPrefixType::Unknown;
    return UNCPrefixType::IPv4;
  }

  inline UNCPrefixType consume_unc_ipv6(StrRef& path) {
    // 8 groups of 4 nibbles (128 bits).
    static constexpr usize maxIPv6 = 8;
    const usize path_size = path.size();
    // The byte offset.
    usize path_idx = 0, block_idx = 0;
    isize remaining_blocks = isize(maxIPv6);
    // Denotes ommitted zeros.
    bool used_double_colon = false;

    for (; path_idx < path_size; ++path_idx) {
      const char C = path[path_idx];
      if (__is_hex(C)) {
        ++block_idx;
        if __expect_false(block_idx > 4) {
          __hc_unreachable("Invalid block size.");
          return UNCPrefixType::Unknown;
        }
        continue;
      } else if __expect_false(C != ':') {
        __hc_unreachable("Invalid character.");
        return UNCPrefixType::Unknown;
      }
      if (block_idx == 0) {
        if __expect_false(used_double_colon) {
          __hc_unreachable("Ambiguous address.");
          return UNCPrefixType::Unknown;
        }
        used_double_colon = true;
        continue;
      }
      block_idx = 0;
      if ((--remaining_blocks) < 0) {
        __hc_unreachable("Too many blocks.");
        return UNCPrefixType::Unknown;
      }
    }

    if (!used_double_colon && remaining_blocks != 0)
      return UNCPrefixType::Unknown;
    // RFC 5952 requires that double colons are not used for 
    // a single section of zeros.
    if (used_double_colon && remaining_blocks <= 1)
      return UNCPrefixType::Unknown;
    path.dropFrontMut(path_idx);
    return UNCPrefixType::IPv6;
  }

  inline UNCPrefixType is_ipv4(StrRef path) {
    // Mininum size for v6 (::N).
    __hc_invariant(path.size() >= 3);
    for (const char C : path.takeFront(4)) {
      if (__is_numeric(C))
        continue;
      if (C == ':' || __is_hex(C))
        return UNCPrefixType::IPv6;
      if (C == '.')
        return UNCPrefixType::IPv4;
      // else:
      return UNCPrefixType::Unknown;
    }
    return UNCPrefixType::IPv6;
  }

  /// Selects whether to consume an IPv4 or IPv6 address, if possible.
  inline UNCPrefixType consume_unc_ip(StrRef& path) {
    // Mininum size for v6 (::N).
    if (path.size() < 3)
      return UNCPrefixType::Unknown;
    // Select version to parse as. We double parse at
    // the moment, but this may be changed later.
    UNCPrefixType ip_ver = is_ipv4(path);
    switch (ip_ver) {
     case UNCPrefixType::IPv4:
      return consume_unc_ipv4(path);
     case UNCPrefixType::IPv6:
      return consume_unc_ipv6(path);
     default:
      return UNCPrefixType::Unknown;
    }
  }

  /// Consumes a server or hostname.
  inline UNCPrefixType consume_unc_server_host(StrRef& path) {
    auto name_type = UNCPrefixType::HostName;
    usize path_idx = 1, dot_count = 0;
    // Ensure it begins with [a-zA-Z].
    if (!__is_alpha(path.frontSafe()))
      return UNCPrefixType::Unknown;
    for (const char C : path.dropFront()) {
      if (C == '\\')
        break;
      else if (C == '_')
        name_type = UNCPrefixType::NetBIOS;
      else if (C == '$') {
        using enum UNCPrefixType;
        if (!MMatch(name_type).is(HostName, NetBIOS)) {
          __hc_unreachable("Invalid NetBIOS name.");
          return UNCPrefixType::Unknown;
        }
        name_type = UNCPrefixType::NetBIOS;
        ++path_idx;
        break;
      } else if (C == '.') {
        name_type = UNCPrefixType::FQDN;
        ++dot_count;
      } else {
        __hc_invariant(__is_alnum(C));
      }
      ++path_idx;
    }

    if (dot_count > 2) {
      __hc_unreachable("Invalid domain name");
      return UNCPrefixType::Unknown;
    } else if (name_type == UNCPrefixType::FQDN) {
      __hc_assert(dot_count == 2);
      // If ends with a dot:
      if (path[path_idx - 1] == '.') {
        __hc_unreachable("Invalid domain name.");
        return UNCPrefixType::Unknown;
      }
    }

    path.dropFrontMut(path_idx);
    if (!path.beginsWith('\\'))
      return UNCPrefixType::Unknown;
    return name_type;
  }

  /// Handles hostnames, NetBIOS machine names, IPs and FQDNs.
  inline bool consume_unc_prefix(StrRef& path) {
    using enum UNCPrefixType;
    if (consume_unc_ip(path) != Unknown)
      return true;
    return (consume_unc_server_host(path) != Unknown);
  }

  /// Consumes a share (name or `[drive]$`).
  inline bool consume_unc_share(StrRef& path) {
    usize path_idx = 0;
    for (const char C : path) {
      if (MMatch(C).is('/', '\\'))
        break;
      if (C == '$' && path_idx == 1) {
        ++path_idx;
        break;
      }
      if __expect_false(!__is_alnum(C))
        return false;
      ++path_idx;
    }
    __hc_invariant(path_idx > 0);
    path.dropFrontMut(path_idx);
    return true;
  }

  /// Checks if path matches the UNC spec.
  /// They look like `\\[host|IP|FQDM]\[share]\~`.
  /// Must use backslashes, as they are used to distinguish FQDNs.
  /// `$` is also used in the place of `:` for the same reason.
  inline bool is_unc_path(StrRef path) {
    if (!path.consumeFront("\\\\"))
      return false;
    if __expect_false(!consume_unc_prefix(path))
      return false;
    $ConsumeMultiChars(path, '\\');
    return consume_unc_share(path);
  }

  /// Checks if path is a legacy device.
  /// Reserved: CON, PRN, AUX, NUL, COM[0-9], LPT[0-9]
  inline bool is_legacy_device(StrRef path) {
    if (path.size() == 3)
      return path.beginsWith("CON", "PRN", "AUX", "NUL");
    else if (path.size() == 4) {
      $ConsumeMultiChars(path, "COM", "LPT");
      return __is_numeric(path.front());
    }
    return false;
  }

  ////////////////////////////////////////////////////////////////////////
  /// Checks if path begins with a possible DOSDrive or LegacyDevice.
  /// Assumes the device path prefix has been removed.
  PathType deduce_dos_drive_type(StrRef path) {
    if __expect_false(path.isEmpty())
      return PathType::Unknown;
    usize path_idx = 0;
    for (const char C : path) {
      if (__is_alnum(C)) {
        ++path_idx;
        continue;
      }
      if (MMatch(C).is('/', '\\'))
        break;
      // Invalid character:
      return PathType::Unknown;
    }
    if (path_idx == 0)
      return PathType::Unknown;
    // Grab whatever's at the front:
    StrRef S = path.takeFront(path_idx);
    // Recurse on DosDevices symlink.
    if (S.isEqual("DosDevices"))
      return deduce_dos_drive_type(
        path.dropFront(path_idx + 1));
    if (is_legacy_device(S))
      return PathType::LegacyDevice;
    return PathType::DosDrive;
  }

  /// Tests for GUIDVolume, DOSDrive, DosVolume, LegacyDevice, and DeviceUNC.
  /// Assumes the device path prefix has been removed.
  PathType deduce_device_path_type(StrRef path, char type) {
    if (is_volume(path)) {
      path.dropFrontMut(2);
      // Device paths must be absolute.
      return path.beginsWithAny("/\\") ?
        PathType::DosVolume : PathType::Unknown;
    } else if (is_volume_guid(path)) {
      return PathType::GUIDVolume;
    }
    // At this point, all paths must be of type `.`
    // We ensure drives have the proper syntax.
    if __expect_false(type != '.')
      return PathType::Unknown;
    return path.beginsWith("UNC") ?
      PathType::DeviceUNC :
      deduce_dos_drive_type(path);
  }

  /// A mostly accurate method of determining the type of
  /// a given path. Doesn't check for the existance of the
  /// path, just if it *could* exist.
  PathType deduce_path_type(StrRef path) {
    // Refers to the current folder.
    if __expect_false(path.isEmpty())
      return PathType::DirRel;
    // QualDOS, DriveRel, DirRel
    if (!path.beginsWithAny("/\\")) {
      if (!is_volume(path))
        return PathType::DirRel;
      path.dropFrontMut(2);
      if (path.isEmpty() || path.beginsWithAny("/\\"))
        return PathType::QualDOS;
      return PathType::DriveRel;
    }

    // GUIDVolume, DosDrive, DosVolume,
    // DeviceUNC, NtNamespace, UNCNamespace
    if (is_device_path(path)) {
      const char C = path[2];
      path.dropFrontMut(4);
      return deduce_device_path_type(path, C);
    } else if (is_nt_path(path))
      return PathType::NtNamespace;
    else if (is_unc_path(path))
      return PathType::UNCNamespace;
    else if (!path.dropFront().beginsWith('/', '\\'))
      return PathType::CurrDriveRel;
    // LegacyDevice
    return PathType::Unknown;
  }
} // namespace `anonymous`

PathType PathNormalizer::PredictPathType(StrRef S) {
  $tail_return deduce_path_type(S.dropNull());
}

//======================================================================//
// Reformatting
//======================================================================//

void PathNormalizer::push(ImmPtrRange<wchar_t> P) {
  const usize N = P.size();
  if (isNameTooLong(N) || P.isEmpty())
    return;
  // Get the old end(). We will copy from here.
  wchar_t* const old_end = path.end();
  __hc_assertOrIdent(path.resizeUninit(path.size() + N));
  inline_memcpy(old_end, P.data(), N * sizeof(wchar_t));
}

void PathNormalizer::push(ImmPtrRange<char> P) {
  // Same as the wide version, except here we need to widen.
  auto S = StrRef(P).dropNull();
  auto WP = $to_wstr_sz(S.data(), S.size());
  this->push(WP);
}

//======================================================================//
// Core
//======================================================================//

namespace {
  char current_drive_letter() {
    const wchar_t prefix = Args::WorkingDir()[0];
    return static_cast<char>(prefix);
  }
} // namespace `anonymous`

void PathNormalizer::applyRelativePath(P::IStaticVec<StrRef>& V) {
  switch (this->type) {
   case DriveRel: {

    break;
  }
   case CurrDriveRel: {

    break;
  }
   case DirRel: {

    break;
  }
   default:
    return;
  }
}

bool PathNormalizer::doNormalization(StrRef S) {
  this->type = deduce_path_type(S);
  if (type == Unknown) {
    err = Error::eInvalName;
    return false;
  }

  PathSliceType path_slices {};
  if (MMatch(type).is(DriveRel, CurrDriveRel, DirRel))
    this->applyRelativePath(path_slices);

  return false;
}

bool PathNormalizer::operator()(StrRef S) {
  path.clear();
  this->err = Error::eNone;
  bool R = doNormalization(S.dropNull());
  this->push(L'\0');
  return R;
}

bool PathNormalizer::operator()(ImmPathRef wpath) {
  path.clear();
  this->err = Error::eNone;
  __hc_todo("operator()(ImmPathRef)", false);
}
