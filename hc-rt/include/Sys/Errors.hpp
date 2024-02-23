//===- Sys/Errors.hpp -----------------------------------------------===//
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

namespace hc::sys {
  enum class Error : int {
    eNone  = 0,   // The default, no error
    ePerms = 1,   // Operation requires special priveleges
    eNoEntry,     // File/dir expected to exist, but doesn't
    ePIO,         // Physical read/write error
    eBadFD,       // Closed FD or insufficient perms (eg. read on write only)
    eNoMem,       // No virtual memory, rare in static mode
    eAccDenied,   // File perms do not allow the operation
    eFault,       // Access violation, not guaranteed to be returned
    eInval,       // Invalid argument for library function.
    eNFiles,      // Maximum files allotted for the process
    eMaxFiles,    // Maximum files allotted for the system
  };
} // namespace hc::sys
