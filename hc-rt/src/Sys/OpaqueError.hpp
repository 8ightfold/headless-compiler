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

#define $NewOpqErr(cls, val, msg, extra...) \
 { OpqErrorClass::New(cls), val, msg __VA_OPT__(,) extra }

namespace hc::sys {
  union OpqErrorClass {
    static constexpr OpqErrorClass New(ErrorGroup G) {
      return { .group = G };
    }
    static constexpr OpqErrorClass New(OpqErrorTy V) {
      return { .id = V };
    }
    bool isUserDefined() const {
      const auto V = uptr(this->id);
      return (V > 3U);
    }
  public:
    ErrorGroup group;
    OpqErrorTy id;
    u64 __pad;
  };

  struct [[gnu::packed]] OpqErrorExtra {
    ErrorSeverity severity 
      = ErrorSeverity::Unset;
    u8  __reserved8;
    u16 __reserved16;
    u32 __reserved32;
  };

  /// The underlying representation of an error.
  /// Useful for wrapping objects nicely.
  struct IOpaqueError {
    OpqErrorClass error_class;
    const char*   error_val;
    const char*   message;
    OpqErrorExtra extra;
  };

  using OSErr = struct _OSErr;
  struct _OSErr {
    static void SetLastError(OpqErrorID ID) {
      SysErr::SetLastError(ID);
    }
    static void SetLastError(OpaqueError E) {
      SysErr::SetLastError(E);
    }
  };
} // namespace hc::sys
