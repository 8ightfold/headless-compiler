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
#include <Bootstrap/Syscalls.hpp>
#include <Common/RawLazy.hpp>
#include <Meta/Once.hpp>

#ifndef __XCRT__
# include <cstdio>
#endif // __XCRT__?

using namespace hc::sys;
namespace C = hc::common;
namespace S = hc::sys;

using LazyIIOFile = C::RawLazy<WinIOFile>;

namespace {
  constinit IIOFileArray<1024> pOut_buf {};
  constinit IIOFileArray<0>    pErr_buf {};
  constinit IIOFileArray<512>  pInp_buf {};

  constinit LazyIIOFile pOut {};
  constinit LazyIIOFile pErr {};
  constinit LazyIIOFile pInp {};
} // namespace `anonymous`

namespace hc::sys {
  [[gnu::used, gnu::noinline]]
  void __init_pfiles() {
    static bool __init = false;
    if __expect_true(__init)
      return;
    __init = true;

    pOut.ctor(1, pOut_buf, BufferMode::Full, IIOMode::Write);
    pErr.ctor(2, pErr_buf, BufferMode::None, IIOMode::Write);
    pInp.ctor(0, pInp_buf, BufferMode::Full, IIOMode::Read);

    pOut->initialize();
    pErr->initialize();
    pInp->initialize();

   #ifndef __XCRT__
    std::printf("DONE.\n");
   #endif // __XCRT__?

    // TODO: Init file handles!!
  }

  [[gnu::used, gnu::noinline]] 
  void __fini_pfiles() {
    static bool __fini = false;
    if __expect_false(__fini)
      return;
    __fini = true;

    // TODO: Fini file handles!!

    const auto close = [&](LazyIIOFile& pFile) {
      IIOFile::FileLock L(pFile.data());
      pFile->flushUnlocked();
      pFile->buf->reset();
    };

    close(pInp);
    close(pErr);
    close(pOut);

    pInp.dtor();
    pErr.dtor();
    pOut.dtor();
  }

 #ifndef __XCRT__
  _PFileInit::_PFileInit() {
    // __init_pfiles();
  }

  // At the moment, without the xcrt there's no way to
  // force these to initialize early... without bullshit.
  // I tried using an iostream-like initialization
  // sentinel (same style used in Syscall.hpp), and
  // no dice, I got `AccessViolation`. So that means
  // you'll just have to hope they get initialized :P
  $Once { __init_pfiles(); };
 #endif // __XCRT__?

  constinit IIOFile* pout = pOut.data();
  constinit IIOFile* perr = pErr.data();
  constinit IIOFile* pin  = pInp.data();
} // namespace hc::sys
