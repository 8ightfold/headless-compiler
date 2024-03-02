//===- Sys/OpaqueError.hpp ------------------------------------------===//
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
#pragma clang system_header

#include <Sys/Errors.hpp>

#define $NewOpqErr(val, msg, extra...) \
 { val, msg __VA_OPT__(,) extra }

namespace hc::sys {
  using OSErr = struct _OSErr;
  struct _OSErr {
    static void SetLastError(OpqErrorID ID) {
      SysErr::SetLastError(ID);
    }
    static void SetLastError(OpaqueError E) {
      SysErr::SetLastError(E);
    }
  };

  //====================================================================//
  // Implementation
  //====================================================================//

  struct [[gnu::packed]] OpqErrorExtra {
    ErrorSeverity severity;
    u8  __reserved8;
    u16 __reserved16;
    u32 __reserved32;
  };

  /// The underlying representation of an error.
  /// Useful for wrapping objects nicely.
  struct IOpaqueError {
    using StrType = const char*;
  public:
    constexpr IOpaqueError(StrType V, StrType M, ErrorSeverity S) :
     error_val(V), message(M), extra(S) { }
    constexpr IOpaqueError(StrType V, StrType M) :
     IOpaqueError(V, M, ErrorSeverity::Unset) { }
    
  public:
    virtual ErrorGroup getErrorGroup() const {
      return ErrorGroup::Unknown;
    }
    
  public:
    StrType error_val;
    StrType message;
    OpqErrorExtra extra;
  };
} // namespace hc::sys
