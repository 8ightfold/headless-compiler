//===- Sys/Win/Nt/IOControl.hpp -------------------------------------===//
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

#include "Filesystem.hpp"
#include "Volume.hpp"
#include <Common/Memory.hpp>

// See https://github.com/wine-mirror/wine/blob/master/include/winioctl.h

#define CTL_TYPE u32

namespace hc::sys::win {

enum class CtlMethod : CTL_TYPE {
  Buffered  = 0,
  InDirect  = 1,
  OutDirect = 2,
  Neither   = 3,
};

struct __packed_alignas(CTL_TYPE) CtlCode {
  constexpr static CtlCode New(
   DeviceType type, CTL_TYPE function,
   CtlMethod method, AccessMask access
  ) {
    return CtlCode {
      .Method   = method,
      .Function = function,
      .Access   = access,
      .Type     = type
    };
  }

  static CtlCode FromCode(CTL_TYPE code) {
    return com::__bit_cast<CtlCode>(code);
  }

  static CTL_TYPE ToCode(CtlCode code) {
    return com::__bit_cast<CTL_TYPE>(code);
  }

  CTL_TYPE toCode() const {
    return CtlCode::ToCode(*this); 
  }

public:
  CtlMethod  Method   : 2;
  CTL_TYPE   Function : 12; // Some function ID...
  AccessMask Access   : 2;
  DeviceType Type     : 16;
};

static_assert(sizeof(CtlCode) == sizeof(CTL_TYPE));
static_assert(alignof(CtlCode) == alignof(CTL_TYPE));

__global auto consoleWrite = CtlCode::New(
  DeviceType::Console, 0x5,
  CtlMethod::OutDirect, AccessMask::Any
);

} // namespace hc::sys::win

#undef CTL_TYPE
