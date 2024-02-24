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
} // namespace `anonymous`

[[gnu::flatten]]
PathType PathNormalizer::GetPathType(C::StrRef path) {
  if __expect_false(path.isEmpty())
    return PathType::Unknown;
  return deduce_path_type(path);
}

bool PathNormalizer::operator()(C::StrRef path) {
  this->type = GetPathType(path);
  if (getType() == PathType::Unknown) {
    err = Error::eInvalName;
    return false;
  }
  
  return false;
}
