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
#include <Sys/File.hpp>

#include <xcrt.hpp>
#include <String/Utils.hpp>
#include <Phase1/ConsoleSetup.hpp>

#include <Sys/Win/Console.hpp>
#include <Sys/Win/Process.hpp>
#include <Sys/Win/Volume.hpp>

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

//////////////////////////////////////////////////////////////////////////

static constinit int X = 1;
static constinit int Y = 1;
static constinit int Z = 1;

$Once { ::X = 2; };
$Once { ::Y = 4; };
$Once { ::Z = 8; };

int main(int V, char** Args) {
  hc::write_file(pout, "Hello world!\n");
  hc::write_file(perr, "Hello error!\n");
  return X + Y + Z; // Returns 14!!
}
