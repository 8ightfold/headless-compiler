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

#define $NewGErr(val, msg, sev) \
 $NewOpqErr(ErrorGroup::GNULike, val, msg, \
  OpqErrorExtra {.severity = ErrorSeverity::sev})

using namespace hc;
using namespace hc::sys;

namespace {
  constexpr IOpaqueError gnuTable[] {
    $NewGErr("", "No error.", Success),
    $NewGErr("", "Operation requires special priveleges.", Error),
    $NewGErr("", "File or directory expected to exist, but doesn't.", Error),
    $NewGErr("", "Physical read/write error.", Error),
    $NewGErr("", "Operations on closed file or insufficient perms.", Error),
    $NewGErr("", "No virtual memory.", Error),
    $NewGErr("", "File permissions do not allow the operation.", Error),
    $NewGErr("", "Access violation.", Error),
    $NewGErr("", "Invalid argument for library function.", Error),
    $NewGErr("", "Maximum files allotted for the process.", Error),
    $NewGErr("", "Maximum files allotted for the system.", Error),
    $NewGErr("", "Invalid filename encountered during normalization.", Error),
    $NewGErr("", "Unnormalized filepath was larger than RT_PATH_MAX.", Error),
    $NewGErr("", "Filepath type not supported.", Error),
    $NewGErr("", "OS error, accessed with SysErr::GetLastError().", Error)
  };
} // namespace `anonymous`

OpaqueError SysErr::RegisterUserError(
 OpqErrorTy G, OpqErrorID ID, const char* S, bool force) {
  __hc_todo("RegisterUserError", nullptr);
}

// TODO: getters/setters

OpaqueError SysErr::GetLastError() {
  __hc_todo("GetLastError", nullptr);
}

void SysErr::SetLastError(OpqErrorID ID) {
  __hc_todo("SetLastError");
}

void SysErr::SetLastError(OpaqueError E) {
  __hc_todo("SetLastError");
}

void SysErr::ResetLastError() {
  __hc_todo("ResetLastError");
}

// error info

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
  return $unwrap(E, "").error_val;
}

const char* SysErr::GetErrorDescription(OpqErrorID ID) {
  const auto E = GetOpaqueError(ID);
  return GetErrorDescription(E);
}

const char* SysErr::GetErrorDescription(OpaqueError E) {
 return $unwrap(E, "").message;
}

OpqErrorID SysErr::GetErrorID(OpaqueError E) {
  __hc_todo("GetErrorDescription", 0U);
}

ErrorGroup SysErr::GetErrorGroup(OpaqueError E) {
  if (!E)
    return ErrorGroup::Unknown;
  if (E->error_class.isUserDefined())
    return ErrorGroup::UserDefined;
  return E->error_class.group;    
}
