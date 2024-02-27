//===- Sys/Win/PFiles.hpp -------------------------------------------===//
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

#include <Sys/Win/IOFile.hpp>
#include <Meta/Once.hpp>

using namespace hc::sys;
namespace S = hc::sys;

namespace {
  constinit IIOFileArray<1024> pOut_buf {};
  constinit IIOFileArray<0>    pErr_buf {};
  constinit IIOFileArray<512>  pIn_buf {};

  // constinit IIOFile pOut;
  // constinit IIOFile pErr;
  // constinit IIOFile pIn;
} // namespace `anonymous`

namespace hc::sys {
  void __init_pfiles() {
    static bool __init = false;
    if __expect_true(__init)
      return;
    (void) pOut_buf;
    (void) pErr_buf;
    (void) pIn_buf;
  }
} // namespace hc::sys

$Once { 
  __init_pfiles(); 
};
