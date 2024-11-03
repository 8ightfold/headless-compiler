//===- Common/Location.hpp ------------------------------------------===//
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

#include "Features.hpp"

HC_HAS_BUILTIN(FILE);
HC_HAS_BUILTIN(FUNCTION);
HC_HAS_BUILTIN(LINE);
HC_HAS_BUILTIN(COLUMN);

namespace hc {

struct SourceLocation {
  constexpr static SourceLocation Current(
   const char* file = __builtin_FILE(),
   const char* func = __builtin_FUNCTION(),
   int line         = __builtin_LINE(),
   int column       = __builtin_COLUMN()
  ) __noexcept {
    return { file, func, line, column };
  }

public:
  const char* __file = __builtin_FILE();
  const char* __func = __builtin_FUNCTION();
  int __line         = __builtin_LINE();
  int __column       = __builtin_COLUMN();
};

} // namespace hc
