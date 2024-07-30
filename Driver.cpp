//===- Driver.cpp ---------------------------------------------------===//
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

#include <Common/Checked.hpp>
#include <Common/DefaultFuncPtr.hpp>
#include <Common/Function.hpp>
#include <Common/Limits.hpp>
#include <Common/RawLazy.hpp>

#include <Meta/ID.hpp>
#include <Meta/Refl.hpp>
#include <Meta/Traits.hpp>
#include <Meta/Unwrap.hpp>

#include <Bootstrap/Win64KernelDefs.hpp>
#include <Bootstrap/ModuleParser.hpp>
#include <Bootstrap/Syscalls.hpp>

#include <Parcel/Skiplist.hpp>
#include <Parcel/StaticVec.hpp>
#include <Parcel/StringTable.hpp>

#include <Sys/Win/Nt/Structs.hpp>
#include <Sys/Win/Nt/Except.hpp>
#include <Sys/Win/Volume.hpp>
#include <Sys/Win/Filesystem.hpp>
#include <Sys/Win/Mutant.hpp>
#include <Sys/Win/PathNormalizer.hpp>
#include <Sys/Args.hpp>
#include <Sys/IOFile.hpp>
#include <Sys/OSMutex.hpp>
#include <Sys/BasicNetwork.hpp>
#include <Sys/Atomic.hpp>

#include <Meta/ASM.hpp>
#include "VolumeInfoDumper.hpp"

#pragma push_macro("NDEBUG")
#undef NDEBUG
#include <cassert>
#include <cstdio>
#pragma pop_macro("NDEBUG")

using namespace hc;
namespace B = hc::bootstrap;
namespace C = hc::common;
namespace M = hc::meta;
namespace P = hc::parcel;
namespace S = hc::sys;
namespace W = hc::sys::win;

#define FLAG_MAP(ty) | S::IIOMode::ty
#define IO(ty, tys...) S::IIOMode::ty $PP_expand($PP_mapC(FLAG_MAP, ##tys))
#define IIO(ty, tys...) (IO(ty, ##tys))
#define IO_TEST(str, ex) assert( \
  S::IIOFile::ParseModeFlags(str) == IIO($PP_rm_parens(ex)))

struct alignas(u16) X {
  X()      { std::printf("Ctor: %p\n", this); }
  X(int I) { std::printf("Ctor(%i)\n", I); }
  ~X()     { std::printf("Dtor: %p\n", this); }
  void me() const {
    std::printf("Hi in %p!\n", this);
  }
};

#include <xcrtDefs.hpp>
#include <Sys/IOFile.hpp>
#include <String/Utils.hpp>

void dumpPathData(C::StrRef path,
 S::PathType chk = S::PathType::Unknown) {
  using namespace hc::sys;
  PathNormalizer norm;
  norm(path);
  if (chk == PathType::Unknown)
    chk = norm.getType();
  std::printf("%s: ", getPathType(norm.getType()));
  printPtrRange(path);
  if (PathType ty = norm.getType(); chk != ty) {
    std::printf("\e[0;31m");
    std::printf("ERROR: Expected type %s, got %s.\n", 
      getPathType(chk), getPathType(ty));
    std::printf("\e[0m");
  }
  // Check if valid path.
  if (Error last = norm.getLastError(); last != Error::eNone) {
    std::printf("\e[1;31m");
    const auto E = SysErr::GetOpaqueError(last);
    std::printf(" [%s]: %s\n\n",
      SysErr::GetErrorNameSafe(E),
      SysErr::GetErrorDescriptionSafe(E));
    std::printf("\e[0m");
    return;
  }

  const auto P = norm.getPath();
  if (!P.isEmpty()) {
    if (chk == PathType::NtNamespace)
      std::printf("\e[1;93m");
    else if (chk == PathType::LegacyDevice)
      std::printf("\e[0;96m");
    printPtrRange(norm.getPath(), "Normalized");
    std::printf("\e[0m");
  }
  std::puts("");
}

void stringTableTests() {
  using enum P::IStringTable::Status;
  auto print_break = [] {
    std::printf("\n|============================|\n");
  };
  {
    print_break();
    P::StringTable<64, 8> Tbl;
    Tbl.insert("eeeeeee");
    Tbl.insert("eee");
    Tbl.insert("e");
    Tbl.insert("ee");
    Tbl.insert("ee");
    Tbl.insert("eeeee");
    for (auto S : Tbl)
      std::printf("\"%.*s\"\n",
        int(S.size()), S.data());
  } {
    print_break();
    P::StringTable<64, 8> Tbl;
    Tbl.setNullTerminationPolicy(true);
    Tbl.insert("ab");
    Tbl.insert("abcd");
    Tbl.insert("abc");
    Tbl.insert("abcdef");
    Tbl.setKSortPolicy(true);
    __hc_assertOrIdent(
      Tbl.insert("abcde").u == success);
    __hc_assertOrIdent(
      Tbl.insert("aa").u == success);
    __hc_assertOrIdent(
      Tbl.insert("abc").u == alreadyExists);
    for (auto S : Tbl)
      std::printf("\"%s\"\n", S.data());
  } {
    print_break();
    P::StringTable<64, 16> Tbl;
    Tbl.setNullTerminationPolicy(true);
    Tbl.insert("");
    Tbl.insert("eeeeeee");
    Tbl.insert("eee");
    Tbl.insert("e");
    Tbl.insert("ee");
    Tbl.insert("ee");
    Tbl.insert("eeeee");
    Tbl.setKSortPolicy(true);
    Tbl.setKSortPolicy(false);
    __hc_assertOrIdent(
      Tbl.insert("").u == alreadyExists);
    for (auto S : Tbl)
      std::printf("\"%s\"\n", S.data());
  }
}

void functionTests() {
  auto test = [] (Function<StrRef(const char*)> F) {
    StrRef S = F("Hello!");
    printPtrRange(S);
  };

  auto exact = [] (const char* S) -> StrRef { return StrRef::NewRaw(S); };
  auto deduced = [&] (const char* S) -> const char* { return S; };

  test(+exact);
  test(deduced);
}

#include <Bootstrap/KUserSharedData.hpp>
// #include "UserShared.hpp"

using hc::boot::KUSER_SHARED_DATA;
using hc::boot::KUSER_XState;

template <typename T>
concept _is_array_ptr_impl = meta::is_ptr<T> && meta::is_array<meta::RemovePtr<T>>;

template <typename T>
concept is_array_ptr = _is_array_ptr_impl<meta::RemoveRef<T>>;

template <typename T>
concept not_array_ptr = !is_array_ptr<T>;

template <typename T>
concept is_inline_structure = meta::is_rvalue_ref<T> &&
  meta::is_ptr<meta::RemoveRef<T>> &&
  meta::is_object<meta::RemovePtr<meta::RemoveRef<T>>>;

template <typename T, typename...Args>
requires (sizeof...(Args) == 4)
void _dump_struct(T* data, bool, const char format[], Args&&...args) {
  auto [depth, type, name, arg] = tuple_fwd($fwd(args)...);
  if constexpr (is_inline_structure<decltype(arg)>) {
    const usize offset = (ubyte*)(arg) - (ubyte*)(data);
    std::printf("%s%s: %s = [0x%llx];\n", depth, name, type, offset);
  } else if constexpr (not_array_ptr<decltype(arg)>) {
    const usize offset = (ubyte*)(&arg) - (ubyte*)(data);
    std::printf("%s%s: %s = [0x%llx];\n", depth, name, type, offset);
  } else {
    const usize offset = (ubyte*)(*arg) - (ubyte*)(data);
    std::printf("%s%s: %s = [0x%llx];\n", depth, name, type, offset);
  }
}

template <typename T, typename...Args>
requires (sizeof...(Args) > 1 && sizeof...(Args) != 4)
void _dump_struct(T* data, bool, const char format[], Args...args) {
  std::printf("*");
  std::printf(format, args...);
}

template <typename T>
void _dump_struct(T* data, bool& did_print, const char format[], const char* name) {
  if (!did_print) [[unlikely]] {
    std::printf("%s <0x%llx>", name, sizeof(T));
    did_print = true;
  }
}

void _dump_struct(void*, bool, const char format[]) {
  std::printf(format);
}

template <typename T>
void dump_struct() {
  RawLazy<T> sto;
  T* data = sto.data();
  bool print_extra = false;
  __builtin_dump_struct(data, _dump_struct, data, print_extra);
}

template <typename T>
void dump_struct(T* data) {
  bool print_extra = false;
  __builtin_dump_struct(data, _dump_struct, data, print_extra);
}

// #define X64DBG_HALT() __hc_trap()
#define X64DBG_HALT() (void(0))

#include <Bootstrap/_NtModule.hpp>
using DbgType = boot::ULong(const char* fmt, ...);
using W::ExceptionRecord;
using W::ContextSave;

static constinit DefaultFuncPtr<DbgType> DbgPrint {};
static constinit bool triedLoadDbgPrint = false;

static bool __try_load_dbgprint() {
  if __expect_true(triedLoadDbgPrint)
    return DbgPrint.isSet();
  triedLoadDbgPrint = true;
  auto* M = boot::__NtModule();
  auto res = M->resolveExport<DbgType>("DbgPrint");
  if (res.isNone())
    return false;
  return DbgPrint.setSafe(*res);
}

static void __try_dbgprint(const char* Format, auto&&...Args) {
  static bool load = __try_load_dbgprint();
  X64DBG_HALT();
  DbgPrint(Format, Args...);
}

#define BREAKPOINT_PRINT 1

struct STRING {
  u16   Length;
  u16   MaximumLength;
  char* Buffer;
};

[[gnu::noinline, gnu::naked]]
W::NtStatus __stdcall DebugPrint(
 STRING* DebugString,
 u32 ComponentId,
 u32 Level)
{
  __asm__ volatile (
    "mov  %%r8d, %%r9d;\n"
    "mov  %%edx, %%r8d;\n"
    "movw 0(%%rcx), %%dx;\n"
    "movq 8(%%rcx), %%rcx;\n"
    "movq $1, %%rax;\n"::
  );
  __asm__ volatile (
    "int $0x2D;\n"
    "int $3;\n"::
  );
  __asm__ volatile ("retn;\n"::);
}

__always_inline boot::Win64TEB* LoadTeb() {
  return boot::Win64TEB::LoadTEBFromGS();
}

__always_inline boot::Win64PEB* LoadPeb() {
  return boot::Win64TEB::LoadPEBFromGS();
}

static bool SetInDbgPrint() {
  if (LoadTeb()->in_debug_print) {
    return true;
  }
  LoadTeb()->in_debug_print = true;
  return false;
}

static void UnsetInDbgPrint() {
  LoadTeb()->in_debug_print = false;
}

W::NtStatus DebugPrintI(
 STRING* DebugString,
 u32 ComponentId,
 u32 Level)
{
  W::NtStatus Status = 0;
  if (SetInDbgPrint())
    return Status;
  
  if (ExceptionRecord::CheckDbgStatus()) {
    std::printf("%*s", 
      int(DebugString->Length),
      DebugString->Buffer);
    return Status;
  }

  X64DBG_HALT();
  Status = DebugPrint(DebugString, ComponentId, Level);
  UnsetInDbgPrint();
  return Status;
}

template <usize N>
W::NtStatus TestPrint(const char(&Str)[N]) {
  char Buf[N];
  inline_memcpy(Buf, Str, N);
  STRING S {
    .Length = u16(N - 1),
    .MaximumLength = u16(N),
    .Buffer = Buf,
  };
  return DebugPrint(&S, 0x65, 3);
}

static W::NtStatus TestPrintExIPrologue(const char* Str, usize N);

static W::NtStatus TestPrintExI(const char* Str, usize N) {
  using W::ExceptionRecord;
  static auto* M = boot::__NtModule();
  auto res = M->resolveExport<void(ExceptionRecord*)>("RtlRaiseException");
  void(*Ex)(ExceptionRecord*) = $unwrap(res);

  W::NtStatus Status = 0;
  if (!ExceptionRecord::CheckDbgStatus()) {
    return std::fprintf(
      stderr, "[DBG] %*s", int(N), Str);
  }

  if (SetInDbgPrint())
    return 0;

  ExceptionRecord Record {};
  Record.code    = 0x40010006; // DBG_PRINTEXCEPTION_C
  Record.record  = nullptr;
  Record.address = ptr_cast<void>(&TestPrintExIPrologue);
  Record.nparams = 2;
  Record.info[0] = N;
  Record.info[1] = uptr(Str);
  ExceptionRecord::Raise(Record);
  // sys::raise_exception(&Record, true);
  // Ex(&Record);

  UnsetInDbgPrint();
  return 0;
}

W::NtStatus TestPrintExIPrologue(const char* Str, usize N) {
  return TestPrintExI(Str, N);
}

template <usize N>
W::NtStatus TestPrintEx(const char(&Str)[N]) {
  return TestPrintExIPrologue(Str, N);
}

extern constinit bool OnlyNt;
extern void symdumper_main();

int main(int N, char* A[], char* Env[]) {
  __try_load_dbgprint();
  printPtrRange(sys::Args::ProgramDir(), "Executable");
  printPtrRange(sys::Args::WorkingDir(), "Working in");
  // OnlyNt = false;
  // symdumper_main();

  DbgPrint("[DbgPrint] Hello world!");
  TestPrintEx("H!\n");
  // TestPrintEx("[TstPrint] Hello world!");
  return 0;

  assert(reinterpret_cast<uptr>(&KUSER_SHARED_DATA) == 0x7FFE0000);
  std::printf("CyclesPerYield: %hu\n", KUSER_SHARED_DATA.CyclesPerYield);
  std::printf("UnparkedProcessorCount: %hu\n", KUSER_SHARED_DATA.UnparkedProcessorCount);
  std::printf("XState.CompactionEnabled: %u\n", KUSER_XState.CompactionEnabled);

  dump_struct(&KUSER_SHARED_DATA);
  // dump_struct(&KUSER_SHARED_DATA.Dbg);
  // dump_struct<MEMORY_BASIC_INFORMATION>();
  dump_struct<W::ContextSave>();
  // dump_struct<B::Win64TEB>();
  // functionTests();
  // stringTableTests();

  {
    using W::ContextSave;
    ContextSave Ctx;
    ContextSave::Capture(&Ctx);
    std::printf("BeingDebugged?: %s\n",
      LoadPeb()->being_debugged ? "true" : "false");
    // ContextSave::Capture(nullptr);
  }

  volatile auto* pSystemTime = &KUSER_SHARED_DATA.SystemTime;
  for (sys::Atomic<bool> B = true; B.load();) {
    printLargeInt(*ptr_cast<volatile W::ULargeInt>(pSystemTime));
    break;
  }

  DbgPrint("Hello world!");
  // TestPrint("Hello world!");
  // TestPrintEx("Hello world!");
  return 0;

  {
    sys::Atomic<int> ai {};
    sys::Atomic<float> af {};
  }

  printVolumeInfo("\\??\\C:\\");
  printPtrRange(sys::Args::WorkingDir(), "\nCurrent directory");

  {
    using enum sys::PathType;
    C::StrRef exampleVolume = 
      // "//./Volume{b75e2c83-0000-0000-0000-602f00000000}/";
      "//./Volume{d145a114-03a1-429e-4a49-3bd01e92bd36}/";
    std::puts("Normal:");
    dumpPathData(exampleVolume,           GUIDVolume);
    dumpPathData("//./PhysicalDrive0/",   DosDrive);
    dumpPathData("//./CON/",              LegacyDevice);
    dumpPathData("//./COM1/",             LegacyDevice);
    dumpPathData("//./COM1F/",            DosDrive);
    dumpPathData("//?/X:/",               DosVolume);
    dumpPathData("//.\\UNC/",             DeviceUNC);
    // dumpPathData("//RAHHHH/",             Unknown);
    dumpPathData("\\\\www.id.com\\xyz\\", UNCNamespace);
    dumpPathData("/??/C:",                NtNamespace);
    dumpPathData("CON.txt",               DirRel);
    dumpPathData("./CON.COM1",            DirRel);
    dumpPathData("/GLOBAL??""/C:",        NtNamespace);
    dumpPathData("NUL",                   LegacyDevice);
    dumpPathData("//./COM3",              LegacyDevice);
    dumpPathData("/??/NUL",               LegacyDevice);
    dumpPathData("D:\\ProgramData",       QualDOS);
    dumpPathData("Z:code",                DriveRel);
    dumpPathData("\\build",               CurrDriveRel);
    dumpPathData("contents.txt",          DirRel);

    std::puts("Weird:");
    dumpPathData("\\\\server\\share",     UNCNamespace);
    dumpPathData("//./pipe/P/../N",       DosDrive);
    dumpPathData("//./X:/F/../../C:/",    DosVolume);
    dumpPathData("X:\\ABC\\..\\..\\..",   QualDOS);
    dumpPathData("X/ABC\\../..\\..",      DirRel);
    dumpPathData("\\",                    CurrDriveRel);
    dumpPathData(".",                     DirRel);
    dumpPathData("../ABC",                DirRel);
    dumpPathData("//./C:/abc/xyz",        DosVolume);

    std::puts("Very Weird:");
    dumpPathData("/??/UNC/abc/xyz");
    dumpPathData("//?/GLOBALROOT/??/UNC/abc/xyz");
    dumpPathData("//?/GLOBALROOT/DosDevices/UNC/abc/xyz");
    dumpPathData("\\\\?\\GLOBALROOT\\Device\\Mup\\abc\\xyz");

    std::puts("...\n");
  }

  S::SysErr::ResetLastError();
  return 0;

  W::StaticUnicodeString name(
    // L"\\??\\PhysicalDrive0\\krita-dev\\krita\\README.md"
    L"\\??\\C:\\krita-dev\\krita\\README.md"
    // L"\\??\\C:\\Program Files\\desktop.ini"
    // L"\\??\\C:\\fake-file.txt"
  );

  auto mask = W::GenericReadAccess;
  W::ObjectAttributes obj_attr { .object_name = &name };
  W::IoStatusBlock io {};
  auto file_attr  = W::FileAttribMask::Normal;
  auto share      = W::FileShareMask::Read;
  auto createDis  = W::CreateDisposition::Open;

  W::FileHandle handle = S::open_file(
    mask, obj_attr, io, nullptr, 
    file_attr, share,
    createDis
  );
  if ($NtFail(io.status)) {
    std::printf("With file `%ls`:\n", name.buffer);
    // std::printf("Open failed! [0x%.8X]\n", io.status);
    std::printf("Open failed! [%s] - %s\n", 
      S::SysErr::GetErrorNameSafe(io.status),
      S::SysErr::GetErrorDescriptionSafe(io.status));
    return io.status;
  }
  std::printf("Opened file `%ls`.\n", name.buffer);

  auto buf = $dynalloc(2048, char).zeroMemory();
  if (auto S = S::read_file(handle, io, buf.intoRange()); $NtFail(S)) {
    std::printf("Read failed! [0x%.8X]\n", S);
    return S::close_file(handle);
  }
  std::printf("Buffer contents:\n%.128s\n...\n\n", buf.data());
  
  if (W::NtStatus S = S::close_file(handle); $NtFail(S)) {
    std::printf("Closing failed! [0x%.8X]\n", S);
    return S;
  }

  return 0;
  auto* preEnv = S::Args::Envp().data();
  while (const char* E = *preEnv++) {
    std::printf("%s\n", E);
  }

  IO_TEST("+r", None);
  IO_TEST("W+", None);
  IO_TEST("r", Read);
  IO_TEST("ab", (Append, Binary));
  IO_TEST("r+", (Read, Plus));  
  IO_TEST("wx+", (Write, Exclude, Plus));

  using hc::meta::__get_idname;
  __get_idname<X>();
  __get_idname<S::PathType>();
  assert($typeid(X) != $typeid(S::PathType));

  using namespace hc::sys::win;
  FSInfoClassWrapper<FSVolumeInfo, 16U> ICW;
  assert(ICW->intoRange().size() == 16U);
  assert(ICW.GetInfoClass() == FSInfoClass::Volume);
}
