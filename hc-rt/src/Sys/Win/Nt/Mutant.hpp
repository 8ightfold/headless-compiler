//===- Sys/Win/Nt/Mutant.hpp ----------------------------------------===//
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

#include "Structs.hpp"

namespace hc::sys::win {

enum class MutantInfoClass {
  Basic, Owner
};

struct BasicMutantInfo {
  i32 current_count = 0;
  Boolean owned_by_caller = true;
  Boolean abandoned_state = false;
};

__global AccessMask MutantQueryState = AccessMask::ReadData;
__global AccessMask MutantAllAccess  = 
  MutantQueryState | AccessMask::StdRightsRequired | AccessMask::Sync;

} // namespace hc::sys::win
