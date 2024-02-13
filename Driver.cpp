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

#include <Sys/Core/Nt/Structs.hpp>
#include <Sys/IO/Win/Filesystem.hpp>
#include <Sys/Sync/Win/Mutant.hpp>
#include <Sys/IOFile.hpp>
#include <Sys/Mutex.hpp>

#pragma push_macro("NDEBUG")
#undef NDEBUG
#include <cassert>
#include <cstdio>
#pragma pop_macro("NDEBUG")

using namespace hc;
namespace B = hc::bootstrap;
namespace C = hc::common;
namespace P = hc::parcel;
namespace S = hc::sys;
namespace W = hc::sys::win;

int main() {
  wchar_t raw_name[] = L"\\??\\C:\\krita-dev\\krita\\README.md";
  auto name = W::UnicodeString::New(raw_name);
  auto mask = W::GenericReadAccess;
  W::ObjectAttributes obj_attr { .object_name = &name };
  W::IoStatusBlock io {};
  auto file_attr  = W::FileAttribMask::Normal;
  auto share      = W::FileShareMask::Read;
  auto createDis  = W::CreateDisposition::Open;
  auto createOpt  = W::CreateOptsMask::IsFile;

  W::FileHandle handle = S::win_open_file(
    mask, obj_attr, io, nullptr, 
    file_attr, share,
    createDis, createOpt
  );
  if ($NtFail(io.status)) {
    std::printf("Open failed! [0x%.8X]\n", io.status);
    return io.status;
  }
  std::printf("Opened file `%ls`.\n", name.buffer);

  auto buf = $dynalloc(2048, char).zeroMemory();
  if (auto S = S::win_read_file(handle, io, buf.toPtrRange()); $NtFail(S)) {
    std::printf("Read failed! [0x%.8X]\n", S);
    return S::win_close(handle);
  }
  std::printf("Buffer contents:\n%.128s\n...\n", buf.data());

  if (W::NtStatus S = S::win_close(handle); $NtFail(S)) {
    std::printf("Closing failed! [0x%.8X]\n", S);
    return S;
  }
}
