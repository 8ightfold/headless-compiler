//===- Sys/Shutdown.hpp ---------------------------------------------===//
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
//  Low-level shutdown functions for a platform.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Features.hpp>

namespace hc::sys {

[[noreturn]] void terminate(int status);
[[noreturn]] void exit(int status);

} // namespace hc::sys
