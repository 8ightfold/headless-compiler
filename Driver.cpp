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

#include <Common/Align.hpp>
#include <Common/Array.hpp>
#include <Common/Casting.hpp>
#include <Common/DynAlloc.hpp>
#include <Common/EnumArray.hpp>
#include <Common/Fundamental.hpp>
#include <Common/Location.hpp>
#include <Common/Memory.hpp>
#include <Common/PtrUnion.hpp>
#include <Common/Strings.hpp>
#include <Common/StrRef.hpp>

#include <Common/Option.hpp>
#include <Common/Result.hpp>
#include <Common/Unwrap.hpp>

#include <Common/Intrusive.hpp>
#include <Parcel/StaticVec.hpp>

#include <Common/Preproc.hpp>
#include <Common/Refl.hpp>
#include <Common/TaggedEnum.hpp>

#include <Bootstrap/Win64KernelDefs.hpp>
#include <Bootstrap/ModuleParser.hpp>
#include <Bootstrap/StubParser.hpp>
// #include <Bootstrap/Syscalls.hpp>

#include <BinaryFormat/COFF.hpp>
#include <BinaryFormat/Consumer.hpp>
#include <BinaryFormat/MagicMatcher.hpp>

#if _HC_DEBUG
# undef NDEBUG
#endif

#include <cstddef>
#include <cassert>
#include <cstdio>
#include <iostream>

#include <windows.h>

#undef GetModuleHandle

namespace C = hc::common;
namespace F = hc::binfmt;
namespace B = hc::bootstrap;
namespace P = hc::parcel;

static constexpr const char* RVA_names[] = {
  "Export Table",
  "Import Table",
  "Resource Table",
  "Exception Table",
  "Certificate Table",
  "BaseRelocation Table",
  "Debug",
  "Architecture",
  "Global Ptr",
  "TLS Table",
  "Load Config Table",
  "Bound Import",
  "Import Address Table",
  "Delay Import Descriptor",
  "CLR Runtime Header",
  "Max Value"
};

std::ostream& operator<<(std::ostream& os, C::StrRef S) {
  return os.write(S.data(), S.size());
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const C::Option<T>& O) {
  if constexpr (C::__has_binary_shl<std::ostream&, const T&>) {
    if (O.isSome()) 
      return os << O.some();
  }
  return os << "<null>";
}

void __dump_introspect(
 u32& count, const char* fmt, auto&&...args) {
  ++count;
  std::printf("%u: ", count);
  std::printf(fmt, args...);
  if (fmt[hc::common::__strlen(fmt) - 1] != '\n') std::puts("\\n");
  std::fflush(stdout);
}

void __dump_args(
 u32& count, const char* fmt, auto&&...args) {
  ++count;
  const auto arg_count = unsigned(sizeof...(args));
  const usize fmt_len = hc::common::__strlen(fmt) + 1;
  void* saddr_prev = nullptr, *saddr_post = nullptr;
  saddr_prev = __builtin_stack_address();
  auto local_fmt = $dynalloc(fmt_len, char).zeroMemory();
  saddr_post = __builtin_stack_address();
  assert(saddr_prev != saddr_post);
  if __expect_false(!local_fmt) {
    std::printf("%u: ALLOC_ERROR\n", count);
    std::fflush(stdout);
    return;
  }
  for (usize I = 0, E = fmt_len; I < E; ++I) {
    const char c = fmt[I];
    if __expect_false(c == '\0') break;
    else if(c < ' ' || c > '~') local_fmt[I] = '\\';
    else local_fmt[I] = c;
  }
  std::printf("%u: `%s` [%u]\n", count, local_fmt.data(), arg_count);
  std::fflush(stdout);
}

u32 dump_introspect(auto* p) {
  u32 count = 0;
  __builtin_dump_struct(p, &__dump_introspect, count);
  std::cout << std::endl;
  count = 0;
  __builtin_dump_struct(p, &__dump_args, count);
  std::cout << std::endl;
  return count;
}

u32 dump_introspect(const auto& v) {
  return dump_introspect(&v);
}

void dump_data(auto* p) {
  if __expect_false(!p) return;
  __builtin_dump_struct(p, &std::printf);
  std::cout << std::endl;
}

void dump_data(const auto& v) { 
  dump_data(&v);
}

template <typename...TT>
void dump_data(C::PtrUnion<TT...> data) {
  data.visit([] <typename P> (P* p) {
    if constexpr (__is_class(P)) {
      dump_data(p);
    }
  });
}

void multidump(auto&&...args) {
  ((dump_data(args)), ...);
}

template <B::__is_win64_list_entry EntryType>
void dumpLDRModule(EntryType* entry) {
  std::cout << "\n|=============================================|\n" << std::endl;
  if __expect_false(entry->isSentinel()) {
    std::cout << "<sentinel>" << std::endl;
    return;
  }
  auto* tbl = entry->asLDRDataTableEntry();
  dump_data(tbl);
  // ...
  u32 lc = 0;
  __builtin_dump_struct(tbl, &__dump_args, lc);
  std::cout << std::endl;
}

static void dump_nt_function(C::StrRef S);

static void dump_module(B::DualString name) {
  using hc::dyn_cast;
  namespace COFF = B::COFF;
  if (name.isEmpty()) {
    std::printf("ERROR: Name cannot be NULL.\n");
    return;
  }
  auto O = B::ModuleParser::GetParsedModule(name);
  if(O.isNone()) {
    if (auto* WS = dyn_cast<const wchar_t>(name))
      std::printf("ERROR: Unable to find module `%ls`.\n", WS);
    if (auto* S  = dyn_cast<const char>(name))
      std::printf("ERROR: Unable to find module `%s`.\n", S);
    return;
  }
  B::COFFModule M = O.some();
  auto& H = M.getHeader();
  auto& T = M.getTables();

  multidump(H.dos, H.file, H.opt, H.win);
  const auto RVA_count = T.data_dirs.size();
  std::cout << "RVA Count: " << RVA_count << "\n\n";
  for (int I = 0; I < RVA_count; ++I) {
    std::cout << "|===================================|\n\n";
    static constexpr auto R = $reflexpr(F::COFF::DataDirectories);
    std::cout << R.Fields().NameAt(I) << ":\n";
    dump_data(T.data_dirs[I]);
  }
  std::cout << "Section Count: " << T.sections.size() << "\n\n";
  for (const auto& S : T.sections) {
    std::cout << "|===================================|\n\n";
    std::cout << C::StrRef(S.name) << ":\n";
    dump_data(S);
  }
  if (u32 extbl_RVA = T.data_dirs[COFF::eDirectoryExportTable].RVA) {
    std::cout << "|===================================|\n\n";
    auto* EDT = M->getRVA<COFF::ExportDirectoryTable>(extbl_RVA);
    dump_data(EDT);
    auto NPT = M->getRangeFromRVA(EDT->name_pointer_table_RVA)
      .intoRange<COFF::NamePointerType>()
      .takeFront(EDT->name_pointer_table_count);
    std::cout << "Exported names:\n";
    for (auto off : NPT) {
      auto S = C::StrRef::NewRaw(M->getRVA<char>(off));
      std::cout << S << '\n';
    }
    std::cout << std::endl;
  }
}

static void dump_exports(B::COFFModule& M, bool dump_body = false) {
  using hc::dyn_cast;
  namespace COFF = B::COFF;
  const auto name = M.getName();
  auto& T = M.getTables();
  if (u32 extbl_RVA = T.data_dirs[COFF::eDirectoryExportTable].RVA) {
    std::cout << "|===================================|\n\n";
    auto* EDT = M->getRVA<COFF::ExportDirectoryTable>(extbl_RVA);
    auto NPT = M->getRangeFromRVA(EDT->name_pointer_table_RVA)
      .intoRange<COFF::NamePointerType>()
      .takeFront(EDT->name_pointer_table_count);
    if (auto* WS = dyn_cast<const wchar_t>(name))
      std::printf("Exported names for `%ls`:\n", WS);
    if (auto* S  = dyn_cast<const char>(name))
      std::printf("Exported names for `%s`:\n", S);
    for (auto off : NPT) {
      auto S = C::StrRef::NewRaw(M->getRVA<char>(off));
      if (S.beginsWith("Nt")) {
        if (dump_body) 
          dump_nt_function(S);
        else
          std::cout << S << '\n';
      }
    }
    std::cout << std::endl;
  }
}

static void dump_exports(B::DualString name, bool dump_body = false) {
  if (name.isEmpty()) {
    std::printf("ERROR: Name cannot be NULL.\n");
    return;
  }
  auto O = B::ModuleParser::GetParsedModule(name);
  if(O.isNone()) return;
  dump_exports(O.some(), dump_body);
}

static void list_modules() {
  B::Win64PEB* ppeb = B::Win64TEB::LoadTEBFromGS()->getPEB();
  const auto modules = ppeb->getLDRModulesInMemOrder();
  for (auto* P = modules->prev(); !P->isSentinel(); P = P->prev())
    dump_data(P->asLDRDataTableEntry());
}

static bool check_ret(const u8*& P) {
  const u8 I = *P++;
  if (I == 0xC2) {
    std::printf("%.2X ", u32(*++P));
    std::printf("%.2X ", u32(*++P));
    return false;
  }
  return (I != 0xC2) && (I != 0xC3);
}

void dump_nt_function(C::StrRef S) {
  static thread_local auto O = 
    B::ModuleParser::GetParsedModule("ntdll.dll");
  B::COFFModule& M = O.some();
  if __expect_false(!S.beginsWith("Nt")) {
    std::cout << "Non NT function `" << S << "`" << std::endl;
    return;
  }
  auto* P = hc::ptr_cast<const u8>(M.resolveExportRaw(S));
  if __expect_false(!P) {
    std::cout << "Invalid Function `" << S << "`" << std::endl;
    return;
  }
  
  std::printf("%s [%p]:\n", S.data(), P);
  do {
    std::printf("%.2X ", u32(*P));
  } while (check_ret(P));
  std::cout << '\n';

  auto R = B::parse_stub(S);
  if (R.isOk()) {
    std::printf("syscall: 0x%.3X\n", R.ok());
  } else {
    static constexpr auto E = $reflexpr(B::StubError);
    std::printf("syscall-err: %s\n", E.Fields().Name(R.err()));
  }
  std::cout << std::endl;
}

template <typename Ret, typename...Args>
using StdCall = Ret(&__stdcall)(Args...);

using NtReturn = long;

template <typename...Args>
using NtCall = StdCall<NtReturn, Args...>;

$StrongEnum((Syscall, u32),
  (GetCurrentProcessorNumber, 0),
  (Close),
  (TestAlert),
  (MaxValue)
);

inline C::EnumArray<B::SyscallValue, Syscall> __syscalls_ {};

template <Syscall C, typename Ret = NtReturn, typename...Args>
[[gnu::noinline, gnu::naked]]
inline Ret __stdcall invoke_syscall(Args...args) {
  __asm__ volatile ("movq %%rcx, %%r10;\n"::);
  __asm__ volatile (
    "mov %[val], %%eax;\n"
    "syscall;\n"
    "retn;\n"
    :: [val] "r"(__syscalls_.__data[u32(C)])
  );
}

template <Syscall C>
void print_syscall(const char* name) {
  std::printf("%s: %x\n", name, __syscalls_.__data[u32(C)]);
}

#define $AssignSyscall(name) __syscalls_[Syscall::name] = $unwrap(B::parse_stub($stringify(Nt##name)), -1)
#define $PrintSyscall(name) print_syscall<Syscall::name>(#name)

#include <winternl.h>

template <Syscall C, typename Ret = NtReturn, typename...Args>
void dump_syscall(Args...) {
  auto F = &invoke_syscall<Syscall::TestAlert, Ret, Args...>;
  auto P = reinterpret_cast<const u8*>(F);
  std::printf("invoke_syscall<%s, ...> [%p]:\n",
    $reflexpr(Syscall).Fields().Name(C), P);
  std::printf("value: 0x%.3x\n", __syscalls_[C]);
  do {
    std::printf("%.2X ", u32(*P));
  } while (check_ret(P));
  std::cout << '\n' << std::endl;
}

int main() {
  auto O = B::ModuleParser::GetParsedModule("ntdll.dll");
  B::COFFModule& M = $unwrap(O, 1);
  NtCall<> NtTestAlert = M.resolveExport<long(void)>("NtTestAlert").some();
  StdCall<ULONG> NtGetCurrentProcessorNumber 
    = M.resolveExport<ULONG(void)>("NtGetCurrentProcessorNumber").some();

  $AssignSyscall(TestAlert);
  $AssignSyscall(Close);
  $AssignSyscall(GetCurrentProcessorNumber);

  dump_syscall<Syscall::TestAlert>();
  dump_syscall<Syscall::Close>();
  dump_syscall<Syscall::GetCurrentProcessorNumber>();

  auto TA = invoke_syscall<Syscall::TestAlert>();
  if (auto nTA = NtTestAlert(); TA != nTA)
    return std::fprintf(stderr, "Failed TestAlert: 0x%x [0x%x]\n", TA, nTA);

  auto CPN = invoke_syscall<Syscall::GetCurrentProcessorNumber, ULONG>();
  if (auto nCPN = NtGetCurrentProcessorNumber(); CPN != nCPN)
    return std::fprintf(stderr, "Failed GetCurrentProcessorNumber: %u [%u]\n", CPN, nCPN);
  std::printf("Processor Number: %u\n", CPN);

  HANDLE file = CreateFileA("contents.txt", GENERIC_READ, 0,
    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == nullptr)
    return std::fprintf(stderr, "Failed to open `contents.txt`\n");
  std::printf("Opened `contents.txt`\n");

  auto C = invoke_syscall<Syscall::Close>(file);
  if (C != 0x00000000)
    return std::fprintf(stderr, "Failed Close: %i\n", C);
  std::printf("Closed `contents.txt`\n");
}
