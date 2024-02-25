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
//
//===----------------------------------------------------------------===//

#pragma once

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
    eSetOSError,  // OS error, accessed with `SysErr::GetLastError()`.
    MaxValue,
  };

  enum class ErrorGroup {
    GNULike,          // Found in `Error`.
    PlatformSpecific, // OS errors.
    UserDefined,      // User registered errors.
  };

  /// Pointer to an opaque error representation.
  using OpaqueError = struct IOpaqueError*;

  /// Wrapper class for system errors.
  class SysErr {
    friend struct WinSysErr;
    friend struct LinuxSysErr;
  public:
    /// Converts some ID error to a platform-specific handle.
    static OpaqueError GetOpaqueError(usize ID);
    /// Converts GCC Error to an opaque handle.
    static OpaqueError GetOpaqueError(Error E);

    /// Returns an opaque pointer if an error occured,
    /// otherwise returns a `nullptr`.
    static OpaqueError GetLastError();
    /// Unsets the error if set, otherwise does nothing.
    static void ResetLastError();

    /// Gets a description for an ID. `nullptr` if invalid.
    static const char* GetErrorDescription(usize ID);
    /// Gets a description for an error. `nullptr` if invalid.
    static const char* GetErrorDescription(OpaqueError);

    /// Gets the ID of an opaque handle.
    static usize GetErrorID(OpaqueError);
    /// Gets the `ErrorGroup` of an opaque handle.
    static ErrorGroup GetErrorGroup(OpaqueError);

    /// Defines a custom user error, returns `nullptr` if invalid.
    /// Overwrites are only valid when `force` is `true`.
    static OpaqueError RegisterUserError(struct TODO*);
  
  private:
    static void SetOpaqueError(usize ID);
    static void SetOpaqueError(OpaqueError);
  };
} // namespace hc::sys
