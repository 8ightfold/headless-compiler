//===- Sys/Windows/NtStructs.hpp ------------------------------------===//
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

#include "NtGeneric.hpp"

#define $NtExtract(name) using Nt##name = win::name

namespace hc::sys::win {
  // Filesystem
  enum class AccessMask         : ULong;
  struct     AccessMaskSpecific;
  enum class FileAttribMask     : ULong;
  enum class ObjAttribMask      : ULong;
  enum class FileInfoClass;
  enum class FilesystemInfoClass;
  struct     IoStatusBlock;
  struct     BasicFileInfo;
  union      FileSegmentElement;
} // namespace hc::sys::win

namespace hc::sys {
  // Filesystem
  $NtExtract(AccessMask);
  $NtExtract(AccessMaskSpecific);
  $NtExtract(FileAttribMask);
  $NtExtract(ObjAttribMask);
  $NtExtract(FileInfoClass);
  $NtExtract(FilesystemInfoClass);
} // namespace hc::sys

#undef $NtExtract
