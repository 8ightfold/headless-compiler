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
#include "PathNormalizer.hpp"

using namespace hc;
using namespace hc::sys;
namespace B = hc::bootstrap;
namespace C = hc::common;
namespace P = hc::parcel;
namespace S = hc::sys;

namespace {
  bool __is_ascii(const char C) {
    return (C >= 'A' && C <= 'Z')
      || (C >= 'a' && C <= 'a');
  }

  /// We don't do any checking for validity here.
  /// For example, `NUL.txt` would return `true`.
  /// That aspect gets resolved in later normalization stages.
  /// Reserved: CON, PRN, AUX, NUL, COM[0-9], LPT[0-9]
  inline bool is_legacy_device(C::StrRef path) {
    if (path.size() < 3)
      return false;
    if (path.beginsWith("CON", "PRN", "AUX", "NUL"))
      return true;
    else if (path.beginsWith("COM", "LPT")) {
      const char C = path.dropFront(3).frontSafe();
      return (C >= '0' && C <= '9');
    }
    return false;
  }

  PathType deduce_dos_path_type(C::StrRef path) {
    if (path.beginsWith("UNC"))
      return PathType::DeviceUNC;
    else if (path.dropFront().frontSafe() == ':')
      return PathType::DosVolume;
    else if (is_legacy_device(path))
      return PathType::LegacyDevice;
    return PathType::DosDrive;
  }

  PathType deduce_path_type(C::StrRef path) {
    if (path.beginsWith("//", "\\\\")) {
      path = path.dropFront(2);
      if (path.beginsWith('.', '?'))
        return deduce_dos_path_type(
          path.dropFront(2));
      // Check if probably `//[name]/~`
      else if (__is_ascii(path.frontSafe()))
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

  char get_volume_letter(C::StrRef drive) {

    return '\0';
  }

  //=== Reformatting ===//

  void normalize_slashes(C::DynAllocation<char> P, C::StrRef S) {
    __hc_assert(P.size() == S.size() + 1U);
    C::inline_memcpy(P.data(), S.data(), S.size());
    for (char& C : P)
      C = __expect_true(C != '/') ? C : '\\';
  }
} // namespace `anonymous`

[[gnu::flatten]]
PathType PathNormalizer::GetPathType(C::StrRef S) {
  if __expect_false(S.isEmpty())
    return PathType::Unknown;
  return deduce_path_type(S);
}

void PathNormalizer::push(common::PtrRange<wchar_t> P) {
  const usize N = P.size();
  if (isNameTooLong(N))
    return;
  // Get the old end(). We will copy from here.
  wchar_t* const old_end = path.end();
  __hc_assertOrIdent(path.resizeUninit(N));
  C::inline_memcpy(old_end, P.data(), N);
}

void PathNormalizer::push(common::PtrRange<char> P) {
  const usize N = P.size();
  if (isNameTooLong(N))
    return;
  // Same as the wide version, except here we need to widen.
  wchar_t* const old_end = path.end();
  __hc_assertOrIdent(path.resizeUninit(N));
  (void) old_end;
  __hc_unreachable("push(PtrRange<char>) is unimplemented.");
}


bool PathNormalizer::operator()(C::StrRef S) {
  S = S.dropNull();
  path.clear();
  // TODO: Maybe too soon?
  if (S.size() > path.Capacity()) {
    err = Error::eNameTooLong;
    return false;
  }

  this->type = GetPathType(S);
  if (type == Unknown) {
    err = Error::eInvalName;
    return false;
  } else if (type == LegacyDevice) {
    if (S.endsWith(".txt", ".TXT"))
      S = S.dropBack(4);
  }
  
  auto P = $zdynalloc(S.size() + 1U, char);
  /// Check if we should immediately normalize.
  if (MMatch(type).is(LegacyDevice, DosVolume, QualDOS)) {
    normalize_slashes(P, S);
    S = P.into<C::StrRef>();
  }

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

bool PathNormalizer::operator()(const wchar_t* wpath) {
  __hc_unreachable("operator()(const wchar_t*) is unimplemented.");
  return false;
}
