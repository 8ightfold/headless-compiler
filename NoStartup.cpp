#include <Bootstrap/_NtModule.hpp>
#include <Common/Casting.hpp>
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

  ExceptionRecord Record {};
  Record.code    = 0x40010006; // DBG_PRINTEXCEPTION_C
  Record.record  = nullptr;
  Record.address = ptr_cast<void>(&TestPrintI);
  Record.nparams = 2;
  Record.info[0] = N;
  Record.info[1] = uptr(Str);
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
    TestPrint("No console handle.\n");
  }
  TestPrint(Args[0]);
  return X + Y + Z; // Returns 14!!
}
