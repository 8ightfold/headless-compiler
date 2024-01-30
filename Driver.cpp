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

namespace C  = hc::common;
namespace BF = hc::binfmt;
namespace B  = hc::bootstrap;

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

u32 dump_introspect(auto&& v) {
  u32 count = 0;
  __builtin_dump_struct(&v, &__dump_introspect, count);
  std::cout << std::endl;
  count = 0;
  __builtin_dump_struct(&v, &__dump_args, count);
  std::cout << std::endl;
  return count;
}

void dump_data(auto* p) {
  if(!p) return;
  __builtin_dump_struct(p, &std::printf);
  std::cout << std::endl;
}

void dump_data(const auto& v) {
  __builtin_dump_struct(&v, &std::printf);
  std::cout << std::endl;
}

template <typename...TT>
void dump_data(C::PtrUnion<TT...> data) {
  data.visit([] <typename P> (P* p) {
    if constexpr(__is_class(P)) {
      dump_data(p);
    }
  });
}

void dynalloc_test(usize len) {
  auto local = $zdynalloc(len, std::string).init("Hello!");
  for(const auto& str : local) {
    std::cout << str << std::endl;
  }
  std::cout << "Bye bye!\n" << std::endl;
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
  u32 lc = 0;
  __builtin_dump_struct(tbl, &__dump_args, lc);
  std::cout << std::endl;
}

#define $unwrap(obj, ...) ({ if(!obj) return { __VA_ARGS__ }; *obj; })

int main() {
  i8  dst[64];
  u32 src[16] { };
  u32 cpy[16];

  C::__zero_memory(dst);
  C::__array_memset(src, -1);
  C::__vmemcpy<64>(dst, src);
  C::__memcpy(cpy, src);
  __hc_assert(C::__memcmp(src, cpy, 16) == 0);

  int data[8] { };
  C::__array_memset(data, 2);
  __clpragma(loop unroll(enable))
  for(int idx = 0; idx < 8; ++idx) {
    data[idx] *= idx;
  }

  auto loc = hc::SourceLocation::Current();
  auto* addr = C::__addressof(loc);
  std::string str { "Hello world!" };

  // dump_introspect(loc);
  // dump_introspect(str);

  B::Win64PEB* ppeb = B::Win64TEB::LoadTEBFromGS()->getPEB();
  const auto ldr_modules = ppeb->getLDRModulesInMemOrder();
  auto* ntdll = ldr_modules->findLoadedDLL(L"ntdll.dll");
  auto* kernel32 = ldr_modules->findLoadedDLL(L"KERNEL32.DLL");
  // dump_image(ldr_modules->GetExecutableEntry());
  // dump_image(ntdll);
  // dump_image(kernel32);

  namespace COFF = BF::COFF;
  using COFF::OptPEWindowsHeader;
  auto IC = BF::Consumer::New(ntdll->getImageRange());
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
  auto dir_headers = IC.intoRangeRaw<COFF::DataDirectoryHeader>(opt_size);

  dump_data(dos_header);
  dump_data(file_header);
  dump_data(opt_header);
  dump_data(win_header);
  for(int I = 0, E = BF::COFF::eDirectoryMaxValue; I != E; ++I) {
    if(dir_headers[I].size == 0) continue;
    dump_data(dir_headers[I]);
  }

  auto& export_table = dir_headers[COFF::eDirectoryExportTable];
  dump_data(export_table);
  void* raw_offset = ntdll->getImageRange().dropFront(export_table.virtual_address).data();
  auto* section_header = static_cast<COFF::SectionHeader*>(raw_offset);
  std::cout << std::string(section_header->name, COFF::eCOFFNameSize) << ":\n";
  dump_data(section_header);
}
