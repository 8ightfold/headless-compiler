//===- Bootstrap/LargeInteger.hpp -----------------------------------===//
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
//  Implementation of Window's [U]LARGE_INTEGER, but type-safe.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Fundamental.hpp>

namespace hc::bootstrap {
  union LargeInt {
    __ndbg_inline operator i64() const {
      return this->quad;
    }
    __ndbg_inline operator i64() const volatile {
      return this->quad;
    }
    __ndbg_inline LargeInt& operator=(i64 I) {
      this->quad = I;
      return *this;
    }
  public:
    i64 quad = 0L;
    struct {
      u32 low;
      i32 high;
    };
  };

  union ULargeInt {
    __ndbg_inline operator u64() const {
      return this->quad;
    }
    __ndbg_inline operator u64() const volatile {
      return this->quad;
    }
    __ndbg_inline ULargeInt& operator=(u64 I) {
      this->quad = I;
      return *this;
    }
  public:
    u64 quad = 0UL;
    struct {
      u32 low;
      u32 high;
    };
  };

  using LargeInteger  = LargeInt;
  using ULargeInteger = ULargeInt;
} // namespace hc::bootstrap
