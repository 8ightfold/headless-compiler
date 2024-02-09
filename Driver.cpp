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

#include <Common/Unwrap.hpp>
#include <Common/Refl.hpp>
#include <Bootstrap/Win64KernelDefs.hpp>
#include <Bootstrap/StubParser.hpp>
#include <Bootstrap/Syscalls.hpp>
#include <Parcel/StaticVec.hpp>
#include <Sys/Windows/NtStructs.hpp>
#include <Sys/Windows/NtFilesystem.hpp>

#pragma push_macro("NDEBUG")
#undef NDEBUG
#include <cassert>
#include <cstdio>
#pragma pop_macro("NDEBUG")

namespace B = hc::bootstrap;
namespace C = hc::common;
namespace P = hc::parcel;
namespace S = hc::sys;
namespace W = hc::sys::win;

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
  std::fflush(stdout);
}

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
  std::printf("At:%lli\n", offset.quad);

  if (W::NtStatus S = S::close(handle); $NtFail(S)) {
    std::printf("Closing failed! [0x%.8X]\n", S);
    return S;
  }
}
