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
#include <Common/Handle.hpp>
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
#include <BinaryFormat/COFF.hpp>

#include <Bootstrap/Syscalls.hpp>
#include <Sys/Windows/NtStructs.hpp>
#include <Sys/Windows/NtFilesystem.hpp>

#if _HC_DEBUG
# undef NDEBUG
#endif

#include <cstddef>
#include <cassert>
#include <cstdio>
#include <iostream>

namespace C = hc::common;
namespace F = hc::binfmt;
namespace B = hc::bootstrap;
namespace P = hc::parcel;

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

static void dump_exports(B::COFFModule& M, [[maybe_unused]] bool dump_body = false) {
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
      if (S.beginsWith("Nt"))
        std::cout << S.dropFront(2) << '\n';
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

template <B::Syscall C, typename Ret = B::NtReturn, typename...Args>
void dump_syscall(Args...) {
  auto F = &B::__syscall<C, Ret, Args...>;
  auto P = reinterpret_cast<const u8*>(F);
  std::printf("__syscall<%s, ...> [%p]:\n",
    $reflexpr(B::Syscall).Fields().Name(C), P);
  std::printf("value: 0x%.3x\n", B::__syscalls_[C]);
  do {
    std::printf("%.2X ", u32(*P));
  } while (check_ret(P));
  std::cout << '\n' << std::endl;
}

void check_syscalls() {
  static constexpr auto R = $reflexpr(B::Syscall);
  const auto& F = R.Fields();
  bool none_unset = true;
  std::printf("Unset syscalls:\n");
  for (auto I = 0; I < F.Count(); ++I) {
    const auto C = B::Syscall(I);
    if (B::__syscalls_[C] == ~0UL) {
      std::printf("Nt%s\n", F.NameAt(I));
      none_unset = false;
    }
  }
  if (none_unset)
    std::printf("All loaded!\n");
  std::cout << std::endl;
}

namespace hc::sys {
  using NtSyscall  = B::Syscall;
  using ReadBuffer = C::PtrRange<char>;

  inline win::FileObjHandle __stdcall open_file(
   NtAccessMask mask, win::ObjectAttributes& attr,
   win::IoStatusBlock& io, win::LargeInt* alloc_size,
   NtFileAttribMask file_attr, win::ULong share_access,
   win::ULong create_disposition, win::ULong create_opts,
   void* ea_buffer = nullptr, win::ULong ea_len = 0UL)
  {
    win::FileObjHandle hout;
    win::NtStatus S = B::__syscall<NtSyscall::CreateFile>(
      &hout, mask, &attr, &io, alloc_size, 
      file_attr, share_access, 
      create_disposition, create_opts,
      ea_buffer, ea_len
    );
    if (S != 0x0)
      std::printf("Opening failed!\n");
    return hout;
  }

  inline win::NtStatus __stdcall read_file(
   win::FileHandle handle, win::EventHandle event,
   win::IOAPCRoutinePtr apc, void* apc_ctx,
   win::IoStatusBlock& io, ReadBuffer buf, 
   win::LargeInt* offset = nullptr, 
   win::ULong* key = nullptr)
  {
    if (!handle) return -1;
    const usize buf_size = buf.size();
    return B::__syscall<NtSyscall::ReadFile>(
      handle.__data, event, apc, apc_ctx,
      &io, buf.data(), win::ULong(!buf_size ? 0 : (buf_size - 1)),
      offset, key
    );
  }

  __always_inline win::NtStatus __stdcall read_file(
   win::FileHandle handle, 
   win::IoStatusBlock& io, ReadBuffer buf, 
   win::LargeInt* offset = nullptr,
   win::ULong* key = nullptr) 
  {
    return read_file(
      handle, win::EventHandle::New(nullptr),
      win::IOAPCRoutinePtr(nullptr), nullptr, 
      io, buf, 
      offset, key
    );
  }

  __always_inline win::NtStatus __stdcall
   close(win::FileObjHandle handle) {
    if (!handle) return -1;
    return B::__syscall<NtSyscall::Close>(handle.__data);
  }
} // namespace hc::sys

namespace S = hc::sys;
namespace W = hc::sys::win;

int main() {
  wchar_t raw_name[] = L"\\??\\C:\\krita-dev\\krita\\README.md";
  auto name = W::UnicodeString::New(raw_name);
  auto mask = 
     W::AccessMask::StdRightsRead
   | W::AccessMask::ReadData
   | W::AccessMask::ReadAttributes
   | W::AccessMask::ReadEA
   | W::AccessMask::Sync;
  W::ObjectAttributes obj_attr { .object_name = &name };
  W::IoStatusBlock io {};
  auto file_attr = W::FileAttribMask::Normal;
  W::ULong share      = 0x00; // FILE_SHARE_READ
  W::ULong createDis  = 0x01; // FILE_OPEN
  W::ULong createOpt  = 0x40; // FILE_NON_DIRECTORY_FILE

  W::FileHandle handle = S::open_file(
    mask, obj_attr, io, nullptr, 
    file_attr, share, createDis, createOpt
  );
  std::printf("Opened file `%ls`.\n", name.buffer);

  auto buf = $dynalloc(2048, char).zeroMemory();
  W::LargeInt offset {};
  if (auto S = S::read_file(handle, io, buf.toPtrRange(), &offset); $NtFail(S)) {
    std::printf("Read failed! [0x%.8X]\n", S);
    return S::close(handle);
  }
  std::printf("Buffer contents:\n%.128s...\n", buf.data());

  if (W::NtStatus S = S::close(handle); $NtFail(S)) {
    std::printf("Closing failed! [0x%.8X]\n", S);
    return S;
  }
}
