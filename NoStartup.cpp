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
#include <Bootstrap/StringMerger.hpp>
#include <Common/Casting.hpp>
#include <Common/InlineMemcpy.hpp>
#include <Common/StrRef.hpp>
#include <Meta/Once.hpp>
#include <Sys/Win/Except.hpp>

#include <xcrt.hpp>
#include <String/Utils.hpp>
#include <Phase1/ConsoleSetup.hpp>

#include <Sys/Win/Console.hpp>
#include <Sys/Win/Process.hpp>

#include "CFTest.hidden.hpp"

using namespace hc;
using namespace hc::bootstrap;
using namespace hc::sys::win;

__always_inline constexpr char wchar_conv(wchar_t WC) {
  return (WC <= 0xFF) ? static_cast<char>(WC) : '?';
}

#define $to_str_sz(S, size) ({ \
  const usize lenU__ = size; \
  auto wstrU__ = $dynalloc(lenU__ + 1, char); \
  for (usize IU__ = 0; IU__ < lenU__; ++IU__) \
    wstrU__[IU__] = wchar_conv(S[IU__]); \
  wstrU__[lenU__] = '\0'; \
  wstrU__; \
})

#define $to_str(S) $to_str_sz(S, xcrt::wstringlen(S))

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

NtStatus TestPrint(const wchar_t* WStr) {
  auto Str = $to_str(WStr);
  return TestPrintI(Str.data(), Str.size() - 1);
}

NtStatus TestPrint(UnicodeString UStr) {
  auto Str = $to_str_sz(UStr.buffer, UStr.getSize());
  return TestPrintI(Str.data(), Str.size() - 1);
}

static Win64Addr GetConsoleHandleRaw() {
  return boot::HcCurrentPEB()
    ->process_params
    ->console_handle;
}

static Win64Addr GetConsoleHandleRaw(usize N) {
  auto* PP = boot::HcCurrentPEB()->process_params;
  switch (N) {
   case 0: return PP->std_in;
   case 1: return PP->std_out;
   case 2: return PP->std_err;
   default:
    return PP->console_handle;
  }
}

static ConsoleHandle GetConsoleHandle() {
  return ConsoleHandle::New(GetConsoleHandleRaw());
}

static ConsoleHandle GetConsoleHandle(usize N) {
  return ConsoleHandle::New(GetConsoleHandleRaw(N));
}

void TestPrintCon(StrRef Str) {
  auto fd = GetConsoleHandle(1);
  usize written = 0;
  auto R = sys::write_console(fd, Str.data(), Str.size(), &written);
  if (written != Str.size()) {
    TestPrint("Failed to write string!");
    TestPrint(Str);
    return;
  }
  sys::write_console(fd, "\n", 1, &written);
}

#define TEST_QUERY(name, bound...) do { \
  auto R = sys::query_process<name, ##bound>(); \
  if (R.isOk()) \
    TestPrint("Got " #name "!"); \
  else \
    TestPrint("Could not get " #name "."); \
} while(0)

void TestQuery() {
  using enum ProcInfo;
  TEST_QUERY(CycleTime);
  TEST_QUERY(GroupInformation, 4);
}

void TestSet() {
  using enum ProcInfo;
  auto R = sys::query_process<ConsoleHostProcess>();
  if (R.isErr()) {
    TestPrint("Could not get ConsoleHostProcess.");
    return;
  }
  if (!sys::set_process<ConsoleHostProcess>(R.ok())) {
    TestPrint("Could not set ConsoleHostProcess.");
    return;
  }
  TestPrint("Set ConsoleHostProcess!");
}

template <usize N>
void TestStrnlenI(const char(&A)[N], usize max) {
  constexpr usize N2 = (N - 1);
  const usize Ex = (N2 > max) ? max : N2;
  if (xcrt::stringnlen(A, max) == Ex)
    return;
  TestPrint(A);
  TestPrint("   [FAILED]");
}

void TestStrnlen() {
  TestStrnlenI("jdasjkjasdkj", 512);
  TestStrnlenI("asdsaasdasd", 3);
  TestStrnlenI("a", 512);
  TestStrnlenI("b", 1);
  TestStrnlenI("c", 0);
  TestStrnlenI("382828282828282", 7);
  TestStrnlenI("3828282828", 77);
}

template <typename Char>
const Char* find_first_str(const Char* S, const Char* Needle, usize MaxRead) {
  const auto Cmp = [](Char L, Char R) -> i32 { return L - R; };
  void* const P = xcrt::xfind_first_str<Char>(S, Needle, MaxRead, Cmp);
  return static_cast<const Char*>(P);
}

template <typename Char, usize N>
void TestStrstrI(const Char(&A)[N], const Char* Needle, bool B) {
  const bool CFRes = TestCF(Needle);
  if (CFRes && (!!find_first_str(A, Needle, (N - 1)) == B))
    return;
  TestPrint(A);
  TestPrint(Needle);
  if (!CFRes)
    TestPrint("   [FAILED] Differing CF");
  else
    TestPrint("   [FAILED]");
}

void TestStrstr() {
  TestStrstrI("Hello world!", "orl", true);

  TestStrstrI("abcdefg", "def", true);
  TestStrstrI(L"abcdefg", L"efg", true);
  TestStrstrI(
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz",
    "zabcdefghijkl", true
  );
  TestStrstrI("abcdfeg", "def", false);
  TestStrstrI(
    "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxy",
    "zabcdefghijklmnopqrstuvwxyz", false
  );
}

//////////////////////////////////////////////////////////////////////////

static constinit int X = 1;
static constinit int Y = 1;
static constinit int Z = 1;

$Once { ::X = 2; };
$Once { ::Y = 4; };
$Once { ::Z = 8; };

int main(int V, char** Args) {
  // TestQuery();
  // TestSet();
  // TestStrnlen();
  // TestStrstr();

  XCRT_NAMESPACE::log_console_state();
  if (!XCRT_NAMESPACE::isConsoleSetUp) {
    TestPrint("Console setup failed.");
    // return 1;
  } else if (!GetConsoleHandle()) {
    TestPrint("No console handle.");
    return 1;
  }

  for (int Ix = 0; Ix < V; ++Ix) {
    StrRef S = Args[Ix];
    TestPrintCon(S);
    // TestPrint(S);
  }

  return X + Y + Z; // Returns 14!!
}
