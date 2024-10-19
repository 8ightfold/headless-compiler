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
#include <Bootstrap/Win64KernelDefs.hpp>
#include <Parcel/StringTable.hpp>
#include <Sys/Args.hpp>
#include <Sys/Win/Volume.hpp>
#include "PathNormalizer.hpp"

#ifndef __XCRT__
# include <cstdio>
# define $printf(...) std::printf(__VA_ARGS__)
#else
# define $printf(...) (void(0))
#endif

using namespace hc;
using namespace hc::sys;
namespace B = hc::bootstrap;
namespace C = hc::common;
namespace P = hc::parcel;
namespace S = hc::sys;

namespace {
  struct PathSlicer {
    PathSlicer(PathNormalizer* N, C::StrRef P) : N(N), P(P) {}
  private:
    PathNormalizer* N;
    C::StrRef P;
  };
} // namespace `anonymous`

//======================================================================//
// Deduction
//======================================================================//

namespace {
  inline bool __is_control(const char C) {
    return (C < ' ' || C == '\x7F');
  }

  inline bool __is_upper(const char C) {
    return (C >= 'A' && C <= 'Z');
  }

  inline bool __is_lower(const char C) {
    return (C >= 'a' && C <= 'z');
  }

  inline bool __is_numeric(const char C) {
    return (C >= '0' && C <= '9');
  }

  inline bool __is_alpha(const char C) {
    return __is_upper(C) || __is_lower(C);
  }

  inline bool __is_alnum(const char C) {
    return __is_alpha(C) || __is_numeric(C);
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

  /// We don't do any checking for validity here.
  /// CHANGED: For example, `NUL.txt` would return `true` (now `false`).
  /// That aspect gets resolved in later normalization stages.
  /// Reserved: CON, PRN, AUX, NUL, COM[0-9], LPT[0-9]
  inline bool is_legacy_device(C::StrRef path) {
    // TODO: Extend to lowercase?
    if (path.size() < 3)
      return false;
    // Enters if back is [0-9].
    if (__is_numeric(path.backSafe())) {
      // Make sure it's actually a legacy device :P
      if (!path.dropBack().endsWith("COM", "LPT"))
        return false;
      const char C = path.dropBack(4).backSafe();
      return MMatch(C).is('\0', '\\', '/');
    } else if (path.endsWith("CON", "PRN", "AUX", "NUL")) {
      const char C = path.dropBack(3).backSafe();
      return MMatch(C).is('\0', '\\', '/');
    }
    return false;
  }

  PathType dos_volume_or_unk(C::StrRef path) {
    return __is_upper(path.frontSafe()) ?
      PathType::DosVolume : PathType::Unknown;
  }

  PathType deduce_dos_path_type(C::StrRef path) {
    if (path.beginsWith("UNC"))
      return PathType::DeviceUNC;
    else if (path.dropFront().frontSafe() == ':')
      return dos_volume_or_unk(path);
    else if (is_legacy_device(path))
      return PathType::LegacyDevice;
    else if (path.beginsWith("Volume{"))
      return PathType::GUIDVolume;
    return PathType::DosDrive;
  }

  PathType deduce_path_type(C::StrRef path) {
    // Empty paths are always relative.
    if __expect_false(path.isEmpty())
      return PathType::DirRel;
    if (path.beginsWith("//", "\\\\")) {
      path = path.dropFront(2);
      if (path.beginsWith('.', '?'))
        return deduce_dos_path_type(
          path.dropFront(2));
      // Check if probably `//[name]/~`
      else if (__is_alpha(path.frontSafe()))
        return PathType::UNCNamespace;
      return PathType::Unknown;
    } else if (path.beginsWith("/", "\\")) {
      path = path.dropFront();
      if (path.beginsWith("??", "GLOBAL??"))
        return PathType::NtNamespace;
      // else: `/~`
      return PathType::CurrDriveRel;
    } else if (is_legacy_device(path)) {
      return PathType::LegacyDevice;
    } else if (path.dropFront().frontSafe() == ':') {
      if (path.dropFront(2).beginsWith('/', '\\'))
        return PathType::QualDOS;
      // else: `[volume]:~`
      return PathType::DriveRel;
    }
    // else: `~`
    return PathType::DirRel;
  }
} // namespace `anonymous`

[[gnu::flatten]]
PathType PathNormalizer::PredictPathType(C::StrRef S) {
  if __expect_false(S.isEmpty())
    return PathType::Unknown;
  return deduce_path_type(S);
}

//======================================================================//
// Reformatting
//======================================================================//

namespace {
  __always_inline char normalize_pchar(const char C) {
    return __expect_true(C != '/') ? C : '\\';
  }
} // namespace `anonymous`

void PathNormalizer::appendPathSlice(C::StrRef S) {
  S = S.dropNull();
  if (isNameTooLong(S.size()) || S.isEmpty())
    return;
  for (char C : S) {
    if __expect_false(!__is_valid_pchar(C))
      this->err = Error::eInvalName;
    this->push(C);
  }
}
void PathNormalizer::appendPath(C::StrRef S) {
  S = S.dropNull();
  if (isNameTooLong(S.size()) || S.isEmpty())
    return;
  for (char C : S) {
    C = normalize_pchar(C);
    if __expect_false(__is_special_pchar(C))
      this->err = Error::eInvalName;
    this->push(C);
  }
}

void PathNormalizer::push(C::ImmPtrRange<wchar_t> P) {
  const usize N = P.size();
  if (isNameTooLong(N) || P.isEmpty())
    return;
  // Get the old end(). We will copy from here.
  wchar_t* const old_end = path.end();
  __hc_assertOrIdent(path.resizeUninit(path.size() + N));
  C::inline_memcpy(old_end, P.data(), N * sizeof(wchar_t));
}

void PathNormalizer::push(C::ImmPtrRange<char> P) {
  // Same as the wide version, except here we need to widen.
  auto S = C::StrRef(P).dropNull();
  auto WP = $to_wstr_sz(S.data(), S.size());
  this->push(WP);
}

//======================================================================//
// Core
//======================================================================//

namespace {
  char current_drive_letter() {
    const wchar_t prefix = Args::WorkingDirW()[0];
    return static_cast<char>(prefix);
  }
} // namespace `anonymous`

void PathNormalizer::removePathPrefix(C::StrRef& S, bool do_push) {
  if (MMatch(type).is(
   GUIDVolume, DosDrive, 
   DosVolume, DeviceUNC)) {
    S.dropFrontMut(4);
    // Add Nt prefix.
    if (do_push)
      this->push(L"\\??\\");
  } else if (MMatch(type).is(
   UNCNamespace, NtNamespace)) {
    S.dropFrontMut();
    bool inject_global = 
      S.consumeFront("GLOBAL");
    // Eat front only if matching.
    S.consumeFront("??");
    S.dropFrontMut();
    // Force the type to be a UNC path if
    // it was originally resolved as Nt.
    if (S.beginsWith("UNC")) {
      this->type = UNCNamespace;
      inject_global = false;
    }
    // Add a global specifier if necessary.
    if (do_push) {
      this->push(L'\\');
      if (inject_global)
        this->push(L"GLOBAL");
      this->push(L"??\\");
    }
  }
}

void PathNormalizer::resolveGlobalroot(C::StrRef& S) {
  if (S.consumeFront("GLOBALROOT")) {
    if (__is_valid_pchar(S.dropFront().frontSafe())) {
      S.dropFrontMut();
      this->type = UNCNamespace;
      return;
    }
    this->type = PredictPathType(S);
    this->removePathPrefix(S, false);
  }
}

void PathNormalizer::appendAbsolutePath(C::StrRef S) {
  
}

bool PathNormalizer::doNormalization(C::StrRef S) {
  this->type = PredictPathType(S);
  if (type == Unknown) {
    err = Error::eInvalName;
    return false;
  }

  // First pass to remove special prefixes.
  // Second pass to remove GLOBALROOT.
  this->removePathPrefix(S);
  this->resolveGlobalroot(S);
  // Remove UNC prefix.
  if (MMatch(type).is(GUIDVolume, DosDrive)) {
    // TODO: Check if even possible to fully support (stretch goal).
    err = Error::eUnsupported;
    return false;
  } else if (MMatch(type).is(
   DeviceUNC, UNCNamespace)) {
    if (S.consumeFront("UNC"))
      S.dropFrontMut();
    this->push(L"UNC\\");
  }

  // GUIDVolume
  // DosVolume
  // DeviceUNC
  // UNCNamespace
  // LegacyDevice

  if (MMatch(type).is(
   DeviceUNC, UNCNamespace, LegacyDevice)) {
    this->push(L"@@");
    this->push(S);
    // TODO: Add support...?
    err = Error::eUnsupported;
    return false;
  }

  // NtNamespace
  if (type == NtNamespace) {
    this->push(L'=');
  }

  // QualDOS
  // DriveRel
  // CurrDriveRel
  // DirRel

  this->push(L'@');
  this->push(S);
  (void) current_drive_letter();

  return false;
}

bool PathNormalizer::operator()(C::StrRef S) {
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

/*
bool PathNormalizer::operator()(C::StrRef S) {
  S = S.dropNull();
  path.clear();

  this->type = PredictPathType(S);
  if (type == Unknown) {
    err = Error::eInvalName;
    return false;
  } else if (MMatch(type).is(UNCNamespace, DeviceUNC, DosDrive)) {
    err = Error::eUnsupported;
    return false;
  } else if (type == LegacyDevice) {
    if (S.endsWith(".txt", ".TXT"))
      S = S.dropBack(4);
  }
  
  auto P = $zdynalloc(S.size() + 1U, char);
  /// Check if we should immediately normalize.
  if (MMatch(type).is(LegacyDevice, DosVolume, QualDOS))
    NormalizeSlashes(S, P);

  switch (type) {
   case QualDOS:
    this->push(L"\\??\\");
    [[fallthrough]];
   case DosVolume:
    if (P[2] == '?')
      P[1] = '?';
    [[fallthrough]];
   case LegacyDevice:
    if (is_legacy_device(S))
      this->push(L"\\\\.\\");
    this->push(P);
    return didError();
   default: break;
  }

  return false;
}
*/
