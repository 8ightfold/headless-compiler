//===- SyscallDumper.cpp --------------------------------------------===//
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

// TODO: Finish implementation

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
#include <Common/TaggedEnum.hpp>

#include <Bootstrap/Win64KernelDefs.hpp>
#include <Bootstrap/ModuleParser.hpp>
#include <Bootstrap/StubParser.hpp>
#include <Bootstrap/Syscalls.hpp>

#include <BinaryFormat/COFF.hpp>
#include <BinaryFormat/Consumer.hpp>
#include <BinaryFormat/MagicMatcher.hpp>

#include <Meta/Refl.hpp>
#include <Meta/Preproc.hpp>
#include <Meta/Unwrap.hpp>
#include <Parcel/StaticVec.hpp>

#include <Sys/Errors.hpp>
#include <Sys/Win/Filesystem.hpp>

#if _HC_DEBUG
# undef NDEBUG
#endif

#include <cassert>
#include <cstdio>

#include <windows.h>

#undef GetModuleHandle

using namespace hc;
namespace C = hc::common;
namespace F = hc::binfmt;
namespace B = hc::bootstrap;
namespace P = hc::parcel;

namespace S = hc::sys;
namespace W = hc::sys::win;

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
  std::puts("");
  std::fflush(stdout);
  count = 0;
  __builtin_dump_struct(p, &__dump_args, count);
  std::puts("");
  std::fflush(stdout);
  return count;
}

u32 dump_introspect(const auto& v) {
  return dump_introspect(&v);
}

void dump_data(auto* p) {
  if __expect_false(!p) return;
  __builtin_dump_struct(p, &std::printf);
  std::puts("");
  std::fflush(stdout);
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

//======================================================================//
// Loader stuff
//======================================================================//

#define $Load_(str) $unwrap(load_module(str))
#define $Load_void(str) $unwrap_void(load_module(str))
#define $Load_Module(str, ret...) &$Load_##ret(str)

static B::Win64LDRDataTableEntry* self_module() {
  return B::Win64InitOrderList::GetExecutableEntry();
}

static B::Win64LDRDataTableEntry* load_module(
 const char* S, bool ignore_ext = false) {
  auto* Entry = B::Win64MemOrderList::GetListSentinel();
  return Entry->findModule(S, ignore_ext);
}

static W::FileHandle openfile(
 W::IoStatusBlock& io, W::UnicodeString S) {
  assert(S.buffer[1] == L':');
  auto fname = $zdynalloc(S.getSize() + 4 + 1, wchar_t);
  com::Mem::Copy(fname.data(), L"\\??\\", 4);
  com::Mem::Copy(fname.data() + 4, S.buffer, S.getSize());

  auto name = W::UnicodeString::New(fname);
  W::ObjectAttributes obj_attr { .object_name = &name };

  auto mask       = W::GenericReadAccess;
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
    std::exit(io.status);
  }

  return handle;
}

template <typename T>
static bool readfile(
 W::FileHandle handle, W::IoStatusBlock& io,
 PtrRange<T> R, i64 off = 0
) {
  auto buf = R.template intoRange<char>();
  W::LargeInt offset { .quad = off };
  if (auto S = S::read_file(handle, io, buf, &offset); $NtFail(S)) {
    std::printf("Read failed! [0x%.8X]\n", S);
    return false;
  }
  return true;
}

static void closefile(W::FileHandle handle) {
  if (W::NtStatus S = S::close_file(handle); $NtFail(S)) {
    std::printf("Closing failed! [0x%.8X]\n", S);
    std::exit(S);
  }
}

static u32 get_stringtbl_off(B::COFFModule& M) {
  using B::COFF::SymbolRecord;
  auto* FH = M.getHeader().file;
  const u32 record_size = FH->symbol_count * sizeof(SymbolRecord);
  return (FH->sym_tbl_addr + record_size);
}

void dumpLDRModule(
 B::Win64LDRDataTableEntry* tbl, 
 bool show_format = false
) {
  dump_data(tbl);
  if (show_format) {
    u32 lc = 0;
    __builtin_dump_struct(tbl, &__dump_args, lc);
  }
  std::puts("");
  std::fflush(stdout);
}

template <B::__is_win64_list_entry EntryType>
void dumpLDRModule(EntryType* entry, bool show_format = false) {
  std::printf("\n|=============================================|\n\n");
  if __expect_false(entry->isSentinel()) {
    std::printf("<sentinel>\n");
    return;
  }
  dumpLDRModule(entry->asLDRDataTableEntry(), show_format);
}

static void dump_module(B::COFFModule& M) {
  namespace COFF = B::COFF;
  auto& H = M.getHeader();
  auto& T = M.getTables();

  multidump(H.dos, H.file, H.opt, H.win);
  const auto RVA_count = T.data_dirs.size();
  std::printf("RVA Count: %llu\n\n", RVA_count);
  for (int I = 0; I < RVA_count; ++I) {
    std::printf("|===================================|\n\n");
    static constexpr auto R = $reflexpr(F::COFF::DataDirectories);
    std::printf("%s:\n", R.Fields().NameAt(I));
    dump_data(T.data_dirs[I]);
  }
  std::printf("Section Count: %llu\n\n", T.sections.size());
  for (const auto& S : T.sections) {
    std::printf("|===================================|\n\n");
    std::printf("%s:\n", S.name);
    dump_data(S);
  }
  if (u32 extbl_RVA = T.data_dirs[COFF::eDirectoryExportTable].RVA) {
    std::printf("|===================================|\n\n");
    auto* EDT = M->getRVA<COFF::ExportDirectoryTable>(extbl_RVA);
    dump_data(EDT);
    auto NPT = M->getRangeFromRVA(EDT->name_pointer_table_RVA)
      .intoRange<COFF::NamePointerType>()
      .takeFront(EDT->name_pointer_table_count);
    std::printf("Exported names:\n");
    for (auto off : NPT) {
      auto S = C::StrRef::NewRaw(M->getRVA<char>(off));
      std::printf("%.*s\n", int(S.size()), S.data());
    }
    std::puts("");
    std::fflush(stdout);
  }
}

static void dump_module(B::DualString name) {
  using hc::dyn_cast;
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
  dump_module(O.some());
}

static void dump_exports(B::COFFModule& M, bool dump_body = false) {
  using hc::dyn_cast;
  namespace COFF = B::COFF;
  const auto name = M.getName();
  auto& T = M.getTables();

  W::IoStatusBlock io {};
  W::FileHandle handle = openfile(io, M->fullName());
  auto readrange = [&handle, &io] (auto R, i64 off = 0) {
    if __expect_false(!readfile(handle, io, R, off)) {
      std::printf("Failed to read file [+%lli]\n", off);
      closefile(handle);
      std::exit(1);
    }
  };

  auto stbl = DynAllocation<char>::New(nullptr, 0);  
  $scope {
    const u32 off = get_stringtbl_off(M);
    u32 tbl_size = 0;
    auto block = PtrRange<u32>::New(&tbl_size, 1u);
    readrange(block, off);
    if (tbl_size <= 4)
      break;
    stbl = $dynalloc(tbl_size, char);
    readrange(stbl.intoRange(), off);
  }

  if (u32 extbl_RVA = T.data_dirs[COFF::eDirectoryExportTable].RVA) {
    std::printf("|===================================|\n\n");
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
        std::printf("%.*s\n", int(S.size()), S.data());
      }
    }
  } else if (auto* FH = M.getHeader().file; M.hasSymbols()) {
    auto syms = $dynalloc(FH->symbol_count, COFF::SymbolRecord);
    readrange(syms.intoRange(), FH->sym_tbl_addr);

    std::printf("|===================================|\n\n");
    if (auto* WS = dyn_cast<const wchar_t>(name))
      std::printf("Exported symbols for `%ls`:\n", WS);
    if (auto* S  = dyn_cast<const char>(name))
      std::printf("Exported symbols for `%s`:\n", S);
    std::printf("Symbol count: %i\n", int(FH->symbol_count));

    usize to_skip = 0;
    for (const auto& rec : syms) {
      if (to_skip) {
        --to_skip;
        continue;
      }

      if (rec.name.zeroes == 0) {
        const u32 off = rec.name.table_offset;
        std::printf("%s", &stbl[off]);
      } else {
        auto& arr = rec.name.short_name;
        int sz = 0;
        if (arr[7] != '\0') sz = 8;
        else sz = __strlen(arr);
        std::printf("%.*s", sz, arr);
      }

      if (auto N = usize(rec.aux_count)) {
        std::printf(": ");
        if __expect_false(N > 1)
          std::printf("\e[1;35m");
        to_skip = N;
        std::printf("%i aux record[s].", int(N));
        std::printf("\e[0m");
      }
      std::printf("\n");
      // TODO: Add zero/aux checks
    }
  } else {
    std::printf("\e[1;91m");
    std::printf("Couldn't locate exported names.\n");
    std::printf("\e[0m");
  }
  
  std::puts("");
  std::fflush(stdout);
  closefile(handle);
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

static void dump_exports(B::Win64LDRDataTableEntry* mod) {
  if (!mod) {
    std::printf("ERROR: Module cannot be NULL.\n");
    return;
  }
  auto O = B::ModuleParser::Parse(mod);
  if (O.isNone()) {
    std::printf("Could not parse module.");
    return;
  }
  dump_exports(O.some());
}

static void list_modules() {
  for (auto* P : B::Win64MemOrderList::GetIterable())
    dump_data(P->asLDRDataTableEntry());
}

void check_module(B::Win64LDRDataTableEntry* mod) {
  std::printf("\e[1;93m");
  {
    auto name      = mod->name();
    std::printf("Dumping  `%.*ls`\n", 
      int(name.getSize()), name.buffer);
    
    auto full_name = mod->fullName();
    std::printf("Filename \"%.*ls\"\n", 
      int(full_name.getSize()), full_name.buffer);
  }
  std::printf("|======================================================|\n\n");
  std::printf("\e[0m");

  auto O = B::ModuleParser::Parse(mod);
  B::COFFModule& M = $unwrap_void(O);
  dump_module(M);
  dump_exports(M);
}

void check_module(com::StrRef S) {
  auto mod = $Load_Module(S.data(), void);
  check_module(mod);
}

void symdumper_main() {
  auto self = self_module();
  auto name = self->name();
  std::printf("Executable name: %.*ls\n\n", int(name.getSize()), name.buffer);
  // list_modules();

  check_module(self);
  // check_module("ntdll.dll");
}
