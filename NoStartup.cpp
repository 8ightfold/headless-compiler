//===- NoStartup.cpp ------------------------------------------------===//
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

#include <Bootstrap/_NtModule.hpp>
#include <Common/Casting.hpp>
#include <Common/InlineMemcpy.hpp>
#include <Common/StrRef.hpp>
#include <Meta/Once.hpp>
#include <Sys/Win/Nt/Except.hpp>

using namespace hc;
using namespace hc::bootstrap;
using namespace hc::sys::win;

static bool SetInDbgPrint() {
  if (HcCurrentTEB()->in_debug_print) {
    return true;
  }
  HcCurrentTEB()->in_debug_print = true;
  return false;
}

static void UnsetInDbgPrint() {
  HcCurrentTEB()->in_debug_print = false;
}

static NtStatus TestPrintI(const char* Str, usize N) {
  if (!ExceptionRecord::CheckDbgStatus())
    return -1;
  if (SetInDbgPrint())
    return 0;
  
  static constexpr usize LoopSize = 512;
  char OutBuf[LoopSize + 2];
  OutBuf[LoopSize + 0] = '\n';
  OutBuf[LoopSize + 1] = '\0';

  ExceptionRecord Record {};
  Record.code    = 0x40010006; // DBG_PRINTEXCEPTION_C
  Record.record  = nullptr;
  Record.address = ptr_cast<void>(&TestPrintI);
  Record.nparams = 2;
  Record.info[0] = LoopSize + 2;
  Record.info[1] = reinterpret_cast<uptr>(OutBuf);

  while (N >= LoopSize) {
    inline_memcpy(OutBuf, Str, LoopSize);
    ExceptionRecord::Raise(Record);
    Str += LoopSize;
    N -= LoopSize;
  }

  Record.info[0] = N + 2;
  inline_memcpy(OutBuf, Str, N);
  OutBuf[N + 0] = '\n';
  OutBuf[N + 1] = '\0';
  ExceptionRecord::Raise(Record);

  UnsetInDbgPrint();
  return 0;
}

NtStatus TestPrint(StrRef Str) {
  return TestPrintI(Str.data(), Str.size());
}

static Win64Addr GetConsoleHandle() {
  return boot::HcCurrentPEB()
    ->process_params
    ->console_handle;
}

//////////////////////////////////////////////////////////////////////////

struct Global {
  ~Global() { TestPrint("Global"); }
};

struct Static {
  ~Static() { TestPrint("Static"); }
};

void static_() { static Static S {}; }
Global global {};

static constinit int X = 1;
static constinit int Y = 1;
static constinit int Z = 1;

$Once { ::X = 2; };
$Once { ::Y = 4; };
$Once { ::Z = 8; };

int main(int V, char** Args) {
  if (V == 0)
    return 55;
  if (!GetConsoleHandle()) {
    TestPrint("No console handle.");
  }
  for (int Ix = 0; Ix < V; ++Ix)
    TestPrint(Args[Ix]);
  static_();
  return X + Y + Z; // Returns 14!!
}
