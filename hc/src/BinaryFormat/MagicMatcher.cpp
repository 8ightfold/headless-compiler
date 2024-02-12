//===- BinaryFormat/MagicMatcher.cpp --------------------------------===//
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

#include <BinaryFormat/MagicMatcher.hpp>
#include <Common/Strings.hpp>

using namespace hc::binfmt;
namespace C  = hc::common;
namespace F = hc::binfmt;

template <usize N>
static inline bool __starts_with(C::AddrRange data, const char(&str)[N]) {
  __hc_invariant(data.size() >= (N - 1));
  return C::__memcmp(data.intoRange<char>().data(), str, N - 1) == 0;
}

MMagic F::MMagic::Match(C::AddrRange binary) {
  if (binary.size() < 2) return Detail::Unknown;
  const auto binstr = binary.intoRange<char>();
  switch ((u8)binstr[0]) {
   case 0x00: {
    break;
   }
   case 0x07:
    if (__starts_with(binary, "\x07\x01"))
      return Detail::COFFOptPEROM;
    break;
   case 0x0B: {
    if (__starts_with(binary, "\x0B\x01"))
      return Detail::COFFOptPE32;
    else if (__starts_with(binary, "\x0B\x02"))
      return Detail::COFFOptPE64;
    break;
   }
   case 'M':
    if (binary.size() >= 4 && __starts_with(binary, "MZ"))
      return Detail::DosHeader;
    break;
   case 'P': {
    if (__starts_with(binary, "PE\0\0"))
      return Detail::COFFHeader;
    break;
   }
   case 'Z':
    if (binstr[1] == 'M')
      return Detail::DosHeader;
    break;
   default:
    break;
  }
  return Detail::Unknown;
}
