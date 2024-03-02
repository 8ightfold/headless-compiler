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
#include <Common/RawLazy.hpp>
#include <Meta/Once.hpp>

using namespace hc::sys;
namespace C = hc::common;
namespace S = hc::sys;

using LazyIIOFile = C::RawLazy<WinIOFile>;

namespace {
  constinit IIOFileArray<1024> pOut_buf {};
  constinit IIOFileArray<0>    pErr_buf {};
  constinit IIOFileArray<512>  pInp_buf {};

  constinit LazyIIOFile pOut;
  constinit LazyIIOFile pErr;
  constinit LazyIIOFile pInp;
} // namespace `anonymous`

namespace hc::sys {
  void __init_pfiles() {
    static bool __init = false;
    if __expect_true(__init)
      return;
    __init = true;
    pOut.ctor(pOut_buf, BufferMode::Full, IIOMode::Write);
    pErr.ctor(pErr_buf, BufferMode::None, IIOMode::Write);
    pInp.ctor(pInp_buf, BufferMode::Full, IIOMode::Read);
  }

  void __fini_pfiles() {
    static bool __fini = false;
    if __expect_false(__fini)
      return;
    __fini = true;
    pOut.dtor();
    pErr.dtor();
    pInp.dtor();
  }

  $Once { __init_pfiles(); };
  $OnExit { __fini_pfiles(); };

  constinit IIOFile* pout = pOut.data();
  constinit IIOFile* perr = pErr.data();
  constinit IIOFile* pin  = pInp.data();
} // namespace hc::sys
