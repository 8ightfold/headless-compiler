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
#include <Sys/Win/Except.hpp>
#include <xcrt.hpp>

#include <Sys/Win/Console.hpp>

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
  static constexpr usize PrintException = 0x40010006; // DBG_PRINTEXCEPTION_C
  static constexpr usize LoopSize = 512;

  if (!ExceptionRecord::CheckDbgStatus())
    return -1;
  if (SetInDbgPrint())
    return 0;
  
  char OutBuf[LoopSize + 2];
  OutBuf[LoopSize + 0] = '\n';
  OutBuf[LoopSize + 1] = '\0';

  auto Record = ExceptionRecord::New(
    PrintException, &TestPrintI,
    LoopSize + 2, OutBuf
  );

  while (N >= LoopSize) {
    inline_memcpy(OutBuf, Str, LoopSize);
    ExceptionRecord::Raise(Record);
    Str += LoopSize;
    N -= LoopSize;
  }

  Record.setArgs(N + 2, OutBuf);
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

void TestPrintCon(StrRef Str) {
  auto fd = FileHandle::New(GetConsoleHandle());
  usize written = 0;
  auto R = sys::write_console(fd, Str.data(), Str.size(), &written);
  if (written != Str.size()) {
    TestPrint("Failed to write string!");
    TestPrint(Str);
  }
}

//////////////////////////////////////////////////////////////////////////

struct Global {
  ~Global() { TestPrint("Global"); }
};

struct Static {
  ~Static() { TestPrint("Static"); }
};

static constinit int X = 1;
static constinit int Y = 1;
static constinit int Z = 1;

$Once { ::X = 2; };
$Once { ::Y = 4; };
$Once { ::Z = 8; };

void static_() {
  static Static S {};
  xcrt::exit(X + Y);
}
Global global {};

int main(int V, char** Args) {
  if (V == 0)
    return 55;
  if (!GetConsoleHandle()) {
    TestPrint("No console handle.");
    return 1;
  }
  for (int Ix = 0; Ix < V; ++Ix) {
    StrRef S = Args[Ix];
    TestPrintCon(S);
    // TestPrint(S);
  }
  static_();
  return X + Y + Z; // Returns 14!!
}
