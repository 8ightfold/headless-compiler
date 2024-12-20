//===- Sys/_File.hpp ------------------------------------------------===//
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
//  Public API for file operations. Implementation is in IOFile.hpp.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Features.hpp>

namespace hc::sys {

struct IIOFile;
struct IIOFileBuf;

enum BufferMode {
  None, Line, Full
};

__global usize poutBufSize = 1024;
__global usize pinpBufSize = 512;

} // namespace hc::sys
