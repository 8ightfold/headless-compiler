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
#include <Sys/File.hpp>
#include "Initialization.hpp"
#include "ConsoleSetup.hpp"

using namespace hc;

extern "C" {

void __xcrt_sysio_setup(void) {
  XCRT_NAMESPACE::setup_console();
  // Initializes at startup.
  // We need these BEFORE calling ctors.
  sys::__init_pfiles();
}

void __xcrt_sysio_shutdown(void) {
  // Destroys at shutdown.
  // We need these after calling dtors.
  sys::__fini_pfiles();
}

} // extern "C"
