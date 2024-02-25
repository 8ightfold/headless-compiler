//===- Sys/Core/Nt/Structs.hpp --------------------------------------===//
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

#include "Generic.hpp"

#define $NtExtract(name) using Nt##name = win::name

namespace hc::sys::win {
  // Generic
  enum class AccessMask         : ULong;
  struct     AccessMaskSpecific;
  enum class ObjAttribMask      : ULong;
  struct     ObjectAttributes;
  // Filesystem
  enum class CreateDisposition  : ULong;
  enum class CreateOptsMask     : ULong;
  enum class FileAttribMask     : ULong;
  enum class FileShareMask      : ULong;
  enum class FileInfoClass;
  enum class FSInfoClass;
  struct     FSAttributeInfo;
  struct     FSControlInfo;
  struct     FSDeviceInfo;
  struct     FSDriverPathInfo;
  struct     FSFullSizeInfo;
  struct     FSObjectIDInfo;
  struct     FSSectorSizeInfo;
  struct     FSSizeInfo;
  struct     FSVolumeInfo;
  struct     IoStatusBlock;
  struct     BasicFileInfo;
  union      FileSegmentElement;
  // Mutant
} // namespace hc::sys::win

namespace hc::sys {
inline namespace __nt {
  $NtExtract(AccessMask);
  $NtExtract(AccessMaskSpecific);
  $NtExtract(CreateDisposition);
  $NtExtract(CreateOptsMask);
  $NtExtract(FileAttribMask);
  $NtExtract(FileShareMask);
  $NtExtract(ObjAttribMask);
  $NtExtract(FileInfoClass);
  $NtExtract(FSInfoClass);
} // inline namespace __nt
} // namespace hc::sys

#undef $NtExtract
