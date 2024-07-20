//===- Sys/Win/_PathUtils.hpp ---------------------------------------===//
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

#include <Common/MMatch.hpp>

namespace {
  inline bool __is_control(const char C) {
    return (C < ' ' || C == '\x7F');
  }

  __always_inline bool __is_upper(const char C) {
    return (C >= 'A' && C <= 'Z');
  }

  __always_inline bool __is_lower(const char C) {
    return (C >= 'a' && C <= 'z');
  }

  __always_inline bool __is_alpha(const char C) {
    return __is_upper(C) || __is_lower(C);
  }

  __always_inline bool __is_numeric(const char C) {
    return (C >= '0' && C <= '9');
  }

  inline bool __is_alnum(const char C) {
    return __is_alpha(C) || __is_numeric(C);
  }

  inline bool __is_hex_upper(const char C) {
    return __is_numeric(C) || (C >= 'A' && C <= 'F');
  }

  inline bool __is_hex_lower(const char C) {
    return __is_numeric(C) || (C >= 'a' && C <= 'f');
  }

  __always_inline bool __is_hex(const char C) {
    return __is_numeric(C)   || 
      (C >= 'A' && C <= 'F') ||
      (C >= 'a' && C <= 'f');
  }

  inline bool __is_special_pchar(const char C) {
    return __is_control(C) || hc::MMatch(C).is(
     '"', '*', '/', ':', '<', '>', '?', '|');
  }

  inline bool __is_valid_pchar(const char C) {
    if __expect_true(__is_alnum(C))
      return true;
    return (C != '\\' && !__is_special_pchar(C));
  }
} // namespace `anonymous`
