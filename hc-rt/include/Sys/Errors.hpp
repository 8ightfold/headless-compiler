//===- Sys/Errors.hpp -----------------------------------------------===//
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
//
//  Defines GCC style error codes, as well as an abstraction around
//  platform specific errors.
//  SysErr::*:
//    OpaqueError.hpp 
//    {PLATFORM}/OpaqueError.cpp
//  SysErr::GetOpaqueError:
//    {PLATFORM}/PlatformStatus.cpp
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Fundamental.hpp>

namespace hc::sys {

enum class Error : int {
  eNone  = 0,   // The default, no error
  ePerms = 1,   // Operation requires special priveleges
  eNoEntry,     // File/dir expected to exist, but doesn't
  ePIO,         // Physical read/write error
  eBadFD,       // Closed FD/insufficient perms (eg. read on write only)
  eNoMem,       // No virtual memory, rare in static mode
  eAccDenied,   // File perms do not allow the operation
  eFault,       // Access violation, not guaranteed to be returned
  eInval,       // Invalid argument for library function.
  eNFiles,      // Maximum files allotted for the process
  eMaxFiles,    // Maximum files allotted for the system
  eInvalName,   // Invalid filename encountered during normalization.
  eNameTooLong, // Unnormalized filepath was larger than `RT_PATH_MAX`.
  eUnsupported, // Filepath type not supported.
  eResName,     // Filepath uses a reserved name.
  eSetOSError,  // OS error, accessed with `SysErr::GetLastError()`
  MaxValue,
};

enum class ErrorGroup : uptr {
  Unknown     = 0, // Null/Unknown
  GNULike     = 1, // Found in `Error`.
  OSError     = 2, // Platform specific errors.
  UserDefined = 3, // User registered errors.
};

enum class ErrorSeverity : u8 {
  Unset       = 0,
  Success     = 1,
  Info        = 2,
  Warning     = 3,
  Error       = 4,
};

/// Implementation defined error identifier.
/// TODO: Add _EOpqErrorID
using OpqErrorID  = u64;
using OpqErrorTy  = const void*;
/// Pointer to an opaque error representation.
using OpaqueError = const struct IOpaqueError*;

//====================================================================//
// Opaque Errors
//====================================================================//

template <typename T>
concept is_valid_opqerr = (__sizeof(T) < 8)
  && __is_trivially_constructible(T)
  && __is_trivially_constructible(T, const T&)
  && __is_trivially_assignable(T&, const T&)
  && __is_trivially_destructible(T);

/// Wrapper class for system errors.
class SysErr {
  using enum ErrorGroup;
  friend struct _OSErr;
public:
  /// Converts some ID error to a platform-specific handle.
  static OpaqueError GetOpaqueError(OpqErrorID ID);
  /// Converts GCC Error to an opaque handle.
  static OpaqueError GetOpaqueError(Error E);

  /// Returns an opaque pointer if an error occured,
  /// otherwise returns a `nullptr`.
  static OpaqueError GetLastError();
  /// Unsets the error if set, otherwise does nothing.
  static void ResetLastError();
  /// Gets last error (if exists) and resets.
  static OpaqueError TakeLastError() {
    OpaqueError last_err = GetLastError();
    ResetLastError();
    return last_err;
  }

  /// Gets a name for an ID. `nullptr` if invalid.
  static const char* GetErrorName(OpqErrorID ID);
  /// Gets a name for an error. Empty if invalid.
  static const char* GetErrorName(OpaqueError);

  /// Gets a description for an ID. `nullptr` if invalid.
  static const char* GetErrorDescription(OpqErrorID ID);
  /// Gets a description for an error. `nullptr` if invalid.
  static const char* GetErrorDescription(OpaqueError);
  /// Gets the `ErrorGroup` of an opaque handle.
  static ErrorGroup GetErrorGroup(OpaqueError);

  /// Gets a name for an error. Empty if invalid.
  static const char* GetErrorNameSafe(auto E) {
    const auto name = SysErr::GetErrorName(E);
    return name ? name : "";
  }

  /// Gets a description for an error. Empty if invalid.
  static const char* GetErrorDescriptionSafe(auto E) {
    const auto desc = SysErr::GetErrorDescription(E);
    return desc ? desc : "";
  }

  /// Defines a custom user error, returns `nullptr` if invalid.
  /// Overwrites are only valid when `force` is `true`.
  template <is_valid_opqerr T, usize N>
  static OpaqueError RegisterUserError(
   T V, const char(&S)[N], bool force = false) {
    const OpqErrorTy G  = ErrorTy(V);
    const OpqErrorID ID = ErrorID(V);
    return RegisterUserError(G, ID, S, force);
  }

private:
  static OpaqueError RegisterUserError(
    OpqErrorTy G, OpqErrorID ID, 
    const char* S, bool force);
  
  template <typename T>
  static inline constinit char errID = 1;

  template <typename T>
  static inline OpqErrorTy ErrorTy(const T& = T()) {
    return &SysErr::errID<T>;
  }

  template <typename T>
  static inline OpqErrorID ErrorID(T V) {
    static constexpr bool is_pow2 = !(sizeof(T) & (sizeof(T) - 1));
    static_assert(is_pow2, "sizeof(Err) must be a power of 2.");
    using TypeVal = uintty_t<T>;
    auto preID = __builtin_bit_cast(TypeVal, V);
    return static_cast<OpqErrorID>(preID);
  }

private:
  static void SetLastError(OpqErrorID ID);
  static void SetLastError(OpaqueError);
};

} // namespace hc::sys
