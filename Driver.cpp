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

#include <Common/DynAlloc.hpp>
#include <Common/Fundamental.hpp>
#include <Common/Location.hpp>
#include <Common/Memory.hpp>
#include <Common/PtrUnion.hpp>
#include <Common/Strings.hpp>
#include <Common/StrRef.hpp>

#include <Bootstrap/Win64KernelDefs.hpp>
#include <BinaryFormat/COFF.hpp>
#include <BinaryFormat/Consumer.hpp>
#include <BinaryFormat/MagicMatcher.hpp>

#include <cstddef>
#include <cassert>
#include <cstdio>
#include <iostream>

#include <Processthreadsapi.h>
#include <winnt.h>
#include <winternl.h>
#include <Winnls.h>

#define __assert_offset(ty, mem, offset) assert($offsetof(mem, ty) == offset)

#define $unwrap(obj, ...) ({ if(!obj) return { __VA_ARGS__ }; *obj; })
#define $extract(name, ty...) visitR<ty>([](auto* p) { return p->name; }) 

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
  if(fmt[hc::common::__strlen(fmt) - 1] != '\n') std::puts("\\n");
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
  for(usize I = 0, E = fmt_len; I < E; ++I) {
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
    if constexpr(__is_class(P)) {
      dump_data(p);
    }
  });
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

int dump_image(B::Win64LDRDataTableEntry* dll) {
  namespace COFF = BF::COFF;
  using COFF::OptPEWindowsHeader;
  dump_data(dll);

  auto IC = BF::Consumer::New(dll->getImageRange());
  auto dos_header = IC.intoIfMatches<COFF::DosHeader>(BF::MMagic::DosHeader);
  if(dos_header) IC.dropRaw(dos_header->coff_header_offset);
  auto file_header = IC.consumeIfMatches<COFF::FileHeader>(BF::MMagic::COFFHeader);
  u16 opt_size = $unwrap(file_header).optional_header_size;

  COFF::OptPEHeader opt_header;
  COFF::PEWindowsHeader win_header;
  if(IC.matches(BF::MMagic::COFFOptPE64)) {
    opt_header = IC.consumeAndSub<COFF::OptPE64Header>(opt_size);
    win_header = IC.consumeAndSub<OptPEWindowsHeader<8>>(opt_size);
  } else if(IC.matches(BF::MMagic::COFFOptPE32)) {
    opt_header = IC.consumeAndSub<COFF::OptPE32Header>(opt_size);
    win_header = IC.consumeAndSub<OptPEWindowsHeader<4>>(opt_size);
  }

  const u32 RVA_count = win_header.$extract(RVA_and_sizes_count);
  auto dir_headers = IC.consumeRange<COFF::DataDirectoryHeader>(RVA_count);
  auto section_headers = IC.consumeRange<COFF::SectionHeader>(file_header->section_count);

  // Object only
  C::PtrRange<COFF::Symbol> symbols;
  C::PtrRange<char> string_table;
  if(file_header->sym_tbl_addr && (volatile bool)(false)) {
    auto sym_offset_span = dll->getRangeFromRVA(file_header->sym_tbl_addr);
    auto sIC = BF::Consumer::New(sym_offset_span);
    symbols = sIC.consumeRange<COFF::Symbol>(file_header->symbol_count);
    const auto string_table_len = *sIC.consume<u32>();
    string_table = sIC.consumeRange<char>(string_table_len);
  }

  auto get_sym = [&] (C::StrRef S) {
    S = S.dropNull();
    if(S[0] == '\\' && !string_table.isEmpty()) {
      u32 offset = 0;
      if(S.dropFront().consumeUnsigned(offset))
        return S;
      S = C::StrRef::NewRaw(&string_table[offset]);
    }
    return S;
  };

  dump_data(dos_header);
  dump_data(file_header);
  dump_data(opt_header);
  dump_data(win_header);

  std::cout << "RVA Count: " << RVA_count << "\n\n";
  for(int I = 0; I < RVA_count; ++I) {
    std::cout << "|===================================|\n\n";
    std::cout << RVA_names[I] << ":\n";
    dump_data(dir_headers[I]);
  }
  std::cout << "Section Count: " << section_headers.size() << "\n\n";
  for(const auto& S : section_headers) {
    std::cout << "|===================================|\n\n";
    auto str = C::StrRef::New(S.name);
    std::cout << get_sym(str) << ":\n";
    dump_data(S);
  }

  return 1;
}

static void dump_modules() {
  const auto modules = B::Win64TEB::LoadTEBFromGS()->getPEB()->getLDRModulesInMemOrder();
  for(auto* P = modules->prev(); !P->isSentinel(); P = P->prev())
    dump_data(P->asLDRDataTableEntry());
}

int main() {
  B::Win64PEB* ppeb = B::Win64TEB::LoadTEBFromGS()->getPEB();
  const auto ldr_modules = ppeb->getLDRModulesInMemOrder();
  auto* self = ldr_modules->findModule(L"driver.exe");
  auto* ntdll = ldr_modules->findModule(L"ntdll.dll");
  auto* kernel32 = ldr_modules->findModule(L"KERNEL32.DLL");

  // dump_modules();
  dump_image(self);
}
