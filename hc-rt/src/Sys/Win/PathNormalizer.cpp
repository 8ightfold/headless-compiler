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
#include <Bootstrap/Win64KernelDefs.hpp>
#include <Sys/Args.hpp>
#include <Sys/Win/Volume.hpp>
#include "PathNormalizer.hpp"

using namespace hc;
using namespace hc::sys;
namespace B = hc::bootstrap;
namespace C = hc::common;
namespace P = hc::parcel;
namespace S = hc::sys;

// TODO: Needs some refactoring for sure...

//======================================================================//
// Deduction
//======================================================================//

namespace {
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

  /// We don't do any checking for validity here.
  /// For example, `NUL.txt` would return `true`.
  /// That aspect gets resolved in later normalization stages.
  /// Reserved: CON, PRN, AUX, NUL, COM[0-9], LPT[0-9]
  inline bool is_legacy_device(C::StrRef path) {
    if (path.size() < 3)
      return false;
    if (path.beginsWith("COM", "LPT")) {
      const char C = path.dropFront(3).frontSafe();
      return (C >= '0' && C <= '9');
    } else if (path.beginsWith("CON", "PRN", "AUX", "NUL"))
      return true;
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
PathType PathNormalizer::GetPathType(C::StrRef S) {
  if __expect_false(S.isEmpty())
    return PathType::Unknown;
  return deduce_path_type(S);
}

//======================================================================//
// Reformatting
//======================================================================//

namespace {
  void normalize_slashes(C::DynAllocation<char> P, C::StrRef S) {
    __hc_assert(P.size() == S.size() + 1U);
    C::inline_memcpy(P.data(), S.data(), S.size());
    for (char& C : P)
      C = __expect_true(C != '/') ? C : '\\';
  }
} // namespace `anonymous`

void PathNormalizer::NormalizeSlashes(C::StrRef& S, StrDyn P) {
  normalize_slashes(P, S);
  S = P.into<C::StrRef>();
}

void PathNormalizer::NormalizeSlashes(C::StrRef S, WStrDyn P) {
  auto SP = $zdynalloc(P.size(), char);
  NormalizeSlashes(S, SP);
  // TODO: Implement widen
  for (usize I = 0; I < P.size(); ++I)
    P[I] = static_cast<wchar_t>(SP[I]);
}

void PathNormalizer::push(C::ImmPtrRange<wchar_t> P) {
  const usize N = P.size();
  if (isNameTooLong(N) || P.isEmpty())
    return;
  // Get the old end(). We will copy from here.
  wchar_t* const old_end = path.end();
  __hc_assertOrIdent(path.resizeUninit(N));
  C::inline_memcpy(old_end, P.data(), N);
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
    const wchar_t prefix = Args::WorkingDir()[0];
    return static_cast<char>(prefix);
  }
} // namespace `anonymous`

void PathNormalizer::removePathPrefix(C::StrRef& S) {
  if (MMatch(type).is(
   GUIDVolume, DosDrive, 
   DosVolume, DeviceUNC)) {
    S.dropFrontMut(4);
    // Add Nt prefix.
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
    this->push(L'\\');
    if (inject_global)
      this->push(L"GLOBAL");
    this->push(L"??\\");
  }
}

void PathNormalizer::resolveGlobalroot(C::StrRef& S) {
  if (S.consumeFront("GLOBALROOT")) {
    if (__is_alpha(S.dropFront().frontSafe())) {
      S.dropFrontMut();
      this->type = UNCNamespace;
      return;
    }
    this->type = GetPathType(S);
    this->removePathPrefix(S);
  }
}

bool PathNormalizer::operator()(C::StrRef S) {
  S = S.dropNull();
  path.clear();

  this->type = GetPathType(S);
  if (type == Unknown) {
    err = Error::eInvalName;
    return false;
  }

  // First pass to remove special prefixes.
  // Second pass to remove GLOBALROOT.
  this->removePathPrefix(S);
  this->resolveGlobalroot(S);
  // Remove UNC prefix.
  if (type == DosDrive) {
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
  // NtNamespace
  // LegacyDevice
  // QualDOS
  // DriveRel
  // CurrDriveRel
  // DirRel

  return false;
}

bool PathNormalizer::operator()(ImmPathRef wpath) {
  __hc_unreachable("operator()(ImmPathRef) is unimplemented.");
  return false;
}

/*
bool PathNormalizer::operator()(C::StrRef S) {
  S = S.dropNull();
  path.clear();

  this->type = GetPathType(S);
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