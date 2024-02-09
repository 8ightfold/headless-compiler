//===- Sys/Windows/NtGeneric.hpp ------------------------------------===//
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

#include <Bootstrap/UnicodeString.hpp>
#include <Common/Fundamental.hpp>

// TODO: $Nt[Status|Info|Warning|Error](...)

namespace hc::sys::win {
  using UnicodeString = bootstrap::Win64UnicodeString;
  using NtStatus  = i32;
  using ULong     = u32;
  using DWord     = ULong;
  using Boolean   = bool; // AKA. u8
  
  union LargeInt {
    __ndbg_inline operator i64() const {
      return this->quad;
    }
  public:
    i64 quad;
    struct {
      u32 low;
      i32 high;
    };
  };

  union ULargeInt {
    __ndbg_inline operator u64() const {
      return this->quad;
    }
  public:
    u64 quad;
    struct {
      u32 low;
      u32 high;
    };
  };
} // namespace hc::sys::win
