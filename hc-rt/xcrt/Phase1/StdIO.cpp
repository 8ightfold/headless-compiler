//===- Phase1/StdIO.cpp ---------------------------------------------===//
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
//  This file ensures the standard io files (pout, perr, pin) have
//  been initialized. On Windows, this initializes the mutexes.
//
//===----------------------------------------------------------------===//

#include <xcrtDefs.hpp>
#include <Bootstrap/Win64KernelDefs.hpp>
#include <Std/utility>
#include <Sys/File.hpp>
#include "Initialization.hpp"

using namespace hc;
using namespace hc::bootstrap;

namespace {
  constinit IOFile __old_pout = nullptr;
  constinit IOFile __old_perr = nullptr;
  constinit IOFile __old_pinp = nullptr;
} // namespace `anonymous`

extern "C" {
  void __xcrt_sysio_setup(void) {
    // TODO: Check if Win64ProcParams::console_handle == nullptr.
    Win64PEB* PEB = Win64TEB::LoadPEBFromGS();
    Win64ProcParams* PP = PEB->process_params;
    if (!PP->hasConsole())
      __hc_todo("Add AllocConsole stuff.");
    // Initializes at startup.
    // We need these BEFORE calling ctors.
    sys::__init_pfiles();
    {
      $XCRTLock(ProcessInfoBlock);
      std::swap(__old_pout, PP->std_out);
      std::swap(__old_perr, PP->std_err);
      std::swap(__old_pinp, PP->std_in);
    }
  }

  void __xcrt_sysio_shutdown(void) {
    {
      // TODO: Check if necessary...
      Win64ProcParams* PP = Win64TEB::LoadPEBFromGS()->process_params;
      $XCRTLock(ProcessInfoBlock);
      PP->std_out = __old_pout;
      PP->std_err = __old_perr;
      PP->std_in  = __old_pinp;
    }
    // Destroys at shutdown.
    // We need these after calling dtors.
    sys::__fini_pfiles();
  }
} // extern "C"
