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
#include <Common/Casting.hpp>
#include <Common/DynAlloc.hpp>
#include <Common/Fundamental.hpp>
#include <Common/Location.hpp>
#include <Common/Memory.hpp>
#include <Common/PtrUnion.hpp>
#include <Common/StaticVec.hpp>
#include <Common/Strings.hpp>
#include <Common/StrRef.hpp>

#include <Common/Option.hpp>
#include <Common/Result.hpp>
#include <Common/Unwrap.hpp>

#include <Bootstrap/Win64KernelDefs.hpp>
#include <Bootstrap/ModuleParser.hpp>
#include <BinaryFormat/COFF.hpp>
#include <BinaryFormat/Consumer.hpp>
#include <BinaryFormat/MagicMatcher.hpp>

#include <cstddef>
#include <cassert>
#include <cstdio>
#include <iostream>

#undef GetModuleHandle

namespace C  = hc::common;
namespace BF = hc::binfmt;
namespace B  = hc::bootstrap;

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
    std::cout << RVA_names[I] << ":\n";
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

static void dump_exports(B::DualString name) {
  using hc::dyn_cast;
  namespace COFF = B::COFF;
  if (name.isEmpty()) {
    std::printf("ERROR: Name cannot be NULL.\n");
    return;
  }
  auto O = B::ModuleParser::GetParsedModule(name);
  if(O.isNone()) return;
  B::COFFModule& M = O.some();
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
      // if (S.beginsWith("Ldr") || S.beginsWith("Nt") || S.beginsWith("Zw"))
      std::cout << S << '\n';
    }
    std::cout << std::endl;
  }
}

static void list_modules() {
  B::Win64PEB* ppeb = B::Win64TEB::LoadTEBFromGS()->getPEB();
  const auto modules = ppeb->getLDRModulesInMemOrder();
  for (auto* P = modules->prev(); !P->isSentinel(); P = P->prev())
    dump_data(P->asLDRDataTableEntry());
}



template <typename T>
struct SizedHandle {
  template <typename Obj>
  SizedHandle(Obj& obj) : __data(obj.data()),
   __cap(obj.data() + obj.__get_capacity()),
   __size(obj.__get_sizeref()) {
  }

  bool checkedEmplace(auto&&...args) {
    if __expect_false(end() == __cap)
      return false;
    (void) C::construct_at(end(), __hc_fwd(args)...);
    ++__size;
    return true;
  }

  bool checkedPush(const T& elem) {
    if __expect_false(end() == __cap)
      return false;
    (void) C::construct_at(end(), elem);
    ++__size;
    return true;
  }

  T* end() __noexcept {
    return __data + __size;
  }

private:
  T* __data = nullptr;
  T* __cap  = nullptr;
  usize& __size;
};

template <typename Obj>
SizedHandle(Obj&) -> SizedHandle<typename Obj::Type>;

template <typename T, usize N>
void print_vec(C::StaticVec<T, N>& V) {
  for(auto&& E : V)
    std::cout << E << ' ';
  std::cout << std::endl;
}

int main() {
  dump_exports("ntdll.dll");
  // dump_exports(L"KERNEL32.DLL");
  // dump_exports("msvcrt.dll");
  auto V = $vec("a", "b", "c", "d", "e");
  auto S = $vec("g", "h", "f");
  while(SizedHandle(V).checkedPush(
   S.popBack().valueOr("."))) {
    print_vec(V);
  }
}
