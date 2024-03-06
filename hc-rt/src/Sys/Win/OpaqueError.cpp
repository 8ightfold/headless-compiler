//===- Sys/Win/OpaqueError.hpp --------------------------------------===//
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

#include <Meta/Unwrap.hpp>
#include <Sys/OpaqueError.hpp>

using namespace hc;
using namespace hc::sys;

//======================================================================//
// Implementation
//======================================================================//

namespace {
  struct IGNUError : IOpaqueError {
    constexpr IGNUError(const char* V, const char* M, 
      ErrorSeverity S = ErrorSeverity::Error) :
     IOpaqueError(V, M, S) {}
  public:
    ErrorGroup getErrorGroup() const override {
      return ErrorGroup::GNULike;
    }
  };

  constexpr IGNUError gnuTable[] {
    $NewOpqErr("Success", "No error.", ErrorSeverity::Success),
    $NewOpqErr("Perms", "Operation requires special priveleges."),
    $NewOpqErr("NoEntry", "File or directory expected to exist, but doesn't."),
    $NewOpqErr("PhysicalIO", "Physical read/write error."),
    $NewOpqErr("BadFileDescriptor", "Operations on closed file or insufficient perms."),
    $NewOpqErr("NoMemory", "No virtual memory."),
    $NewOpqErr("AccessDened", "File permissions do not allow the operation."),
    $NewOpqErr("Segfault", "Access violation."),
    $NewOpqErr("InvalidArgument", "Invalid argument for library function."),
    $NewOpqErr("NoFileSlots", "Maximum files allotted for the process."),
    $NewOpqErr("MaxFiles", "Maximum files allotted for the system."),
    $NewOpqErr("InvalidFilepath", "Invalid filepath encountered during normalization."),
    $NewOpqErr("FilepathTooLong", "Unnormalized filepath was larger than RT_PATH_MAX."),
    $NewOpqErr("UnsupportedFilepath", "Filepath type not supported."),
    $NewOpqErr("ReservedFilename", "Filepath uses a reserved name."),
    $NewOpqErr("OSError", "OS error, accessed with SysErr::GetLastError().")
  };

  thread_local OpaqueError __lasterr_ = nullptr;
} // namespace `anonymous`

OpaqueError SysErr::RegisterUserError(
 OpqErrorTy G, OpqErrorID ID, const char* S, bool force) {
  __hc_todo("RegisterUserError", nullptr);
}

//======================================================================//
// Getters/Setters
//======================================================================//

OpaqueError SysErr::GetLastError() {
  return __lasterr_;
}

void SysErr::SetLastError(OpqErrorID ID) {
  const auto E = GetOpaqueError(ID);
  SetLastError(E);
}

void SysErr::SetLastError(OpaqueError E) {
  __lasterr_ = E;
}

void SysErr::ResetLastError() {
  __lasterr_ = nullptr;
}

//======================================================================//
// Error Info
//======================================================================//

OpaqueError SysErr::GetOpaqueError(Error E) {
  const auto I = usize(E);
  if (I < usize(Error::MaxValue))
    return &gnuTable[I];
  return nullptr;
}

const char* SysErr::GetErrorName(OpqErrorID ID) {
  const auto E = GetOpaqueError(ID);
  return GetErrorName(E);
}

const char* SysErr::GetErrorName(OpaqueError E) {
  if (!E) return nullptr;
  return E->error_val;
}

const char* SysErr::GetErrorDescription(OpqErrorID ID) {
  const auto E = GetOpaqueError(ID);
  return GetErrorDescription(E);
}

const char* SysErr::GetErrorDescription(OpaqueError E) {
  if (!E) return nullptr;
  return E->message;
}

ErrorGroup SysErr::GetErrorGroup(OpaqueError E) {
  if (!E) return ErrorGroup::Unknown;
  return E->getErrorGroup();
}
