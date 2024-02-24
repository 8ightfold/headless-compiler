//===- Bootstrap/UnicodeString.hpp ----------------------------------===//
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
//  Implementation can be found in Win64KernelDefs.cpp
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Fundamental.hpp>
#include <Common/PtrRange.hpp>

namespace hc::bootstrap {
  struct Win64UnicodeString {
    u16 size = 0, size_max = 0; // In bytes
    wchar_t* buffer = nullptr;
  public:
    static Win64UnicodeString New(wchar_t* str);
    static Win64UnicodeString New(wchar_t* str, usize max);
    static Win64UnicodeString New(common::PtrRange<wchar_t> R);
    bool isEqual(const Win64UnicodeString& rhs) const;
  };
} // namespace hc::bootstrap
