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

#include <Common/Checked.hpp>
#include <Common/Limits.hpp>
#include <Common/RawLazy.hpp>
#include <Meta/ID.hpp>
#include <Meta/Refl.hpp>
#include <Meta/Traits.hpp>
#include <Meta/Unwrap.hpp>
#include <Bootstrap/Win64KernelDefs.hpp>
#include <Bootstrap/Syscalls.hpp>
#include <Parcel/Skiplist.hpp>
#include <Parcel/StaticVec.hpp>

#include <Sys/Core/Nt/Structs.hpp>
#include <Sys/Win/Volume.hpp>
#include <Sys/Win/Filesystem.hpp>
#include <Sys/Win/Mutant.hpp>
#include <Sys/Win/PathNormalizer.hpp>
#include <Sys/Args.hpp>
#include <Sys/IOFile.hpp>
#include <Sys/Mutex.hpp>

#include "VolumeInfoDumper.hpp"

#pragma push_macro("NDEBUG")
#undef NDEBUG
#include <cassert>
#include <cstdio>
#pragma pop_macro("NDEBUG")

using namespace hc;
namespace B = hc::bootstrap;
namespace C = hc::common;
namespace M = hc::meta;
namespace P = hc::parcel;
namespace S = hc::sys;
namespace W = hc::sys::win;

// TODO: Make types trivial

#define FLAG_MAP(ty) | S::IIOMode::ty
#define IO(ty, tys...) S::IIOMode::ty $PP_expand($PP_mapC(FLAG_MAP, ##tys))
#define IIO(ty, tys...) (IO(ty, ##tys))
#define IO_TEST(str, ex) assert( \
  S::IIOFile::ParseModeFlags(str) == IIO($PP_rm_parens(ex)))

struct X {
  X()  { std::printf("Ctor: %p\n", this); }
  ~X() { std::printf("Dtor: %p\n", this); }
  void me() const {
    std::printf("Hi in %p!\n", this);
  }
};

#include <String/Utils.hpp>

template <typename Char, usize N>
void test_xstrlen(const Char(&A)[N]) {
  const usize len = xcrt::xstringlen<Char>(A);
  assert(len == (N - 1));
  std::printf("Got: %zu\n", len);
}

int main(int N, char* A[], char* Env[]) {
  test_xstrlen(L"Hello!");
  test_xstrlen(L"We got a pretty damn long string here, hope this is faster!!");
  test_xstrlen("And now we do the exact same tests with regular strings, which should work.");
  return (1 / 2);

  printPathType("//?/PhysicalDrive0/"); // DosDrive
  printPathType("//?/X:/");             // DosVolume
  printPathType("//.\\UNC/");           // DeviceUNC
  printPathType("//RAHHHH/");           // UNCNamespace
  printPathType("\\??\\C:");            // NtNamespace  
  printPathType("/GLOBAL??""/C:");      // NtNamespace
  printPathType("NUL");                 // LegacyDevice
  printPathType("//./CON3");            // LegacyDevice
  printPathType("D:\\ProgramData");     // QualDOS
  printPathType("Z:code");              // DriveRel
  printPathType("\\build");             // CurrDriveRel
  printPathType("contents.txt");        // DirRel
  std::puts("");
  
  W::StaticUnicodeString name(
    // L"\\??\\PhysicalDrive0\\krita-dev\\krita\\README.md"
    // L"\\??\\C:\\krita-dev\\krita\\README.md"
    L"\\??\\C:\\Program Files\\desktop.ini"
  );

  auto mask = W::GenericReadAccess;
  W::ObjectAttributes obj_attr { .object_name = &name };
  W::IoStatusBlock io {};
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
    std::printf("Open failed! [0x%.8X]\n", io.status);
    return io.status;
  }
  std::printf("Opened file `%ls`.\n", name.buffer);

  auto buf = $dynalloc(2048, char).zeroMemory();
  if (auto S = S::read_file(handle, io, buf.intoRange()); $NtFail(S)) {
    std::printf("Read failed! [0x%.8X]\n", S);
    return S::close_file(handle);
  }
  std::printf("Buffer contents:\n%.128s\n...\n", buf.data());
  
  if (W::NtStatus S = S::close_file(handle); $NtFail(S)) {
    std::printf("Closing failed! [0x%.8X]\n", S);
    return S;
  }

  auto* preEnv = S::Args::Envp().data();
  while (const char* E = *preEnv++) {
    std::printf("%s\n", E);
  }

  IO_TEST("+r", None);
  IO_TEST("W+", None);
  IO_TEST("r", Read);
  IO_TEST("ab", (Append, Binary));
  IO_TEST("r+", (Read, Plus));  
  IO_TEST("wx+", (Write, Exclude, Plus));

  using hc::meta::__get_idname;
  __get_idname<X>();
  __get_idname<S::PathType>();
  assert($typeid(X) != $typeid(S::PathType));

  using namespace hc::sys::win;
  FSInfoClassWrapper<FSVolumeInfo, 16U> ICW;
  assert(ICW->intoRange().size() == 16U);
  assert(ICW.GetInfoClass() == FSInfoClass::Volume);

  // S::query_create_volume_info()
}
