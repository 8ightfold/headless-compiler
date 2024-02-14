//===- Sys/Win/Mutex.cpp --------------------------------------------===//
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
#include <Sys/Mutex.hpp>
#include "Mutant.hpp"
#include "Wait.hpp"

using namespace hc::sys;
namespace C = hc::common;
namespace S = hc::sys;

RawMtxHandle S::RawMtxHandle::New(const wchar_t* name) {
  win::NtStatus S = 0;
  auto H = win_create_mutant(S,
    win::MutantAllAccess, name);
  __hc_invariant($NtSuccess(S));
  return RawMtxHandle {H.__data};
}

[[gnu::flatten]]
RawMtxHandle S::RawMtxHandle::New(const char* name) {
  if (!name)
    return RawMtxHandle::New(
      (const wchar_t*)nullptr);
  auto wname = $to_wstr(name);
  return RawMtxHandle::New(wname.data());
}

void S::RawMtxHandle::Delete(RawMtxHandle H) {
  __hc_invariant(H.isInitialized());
  const auto nt_handle = win::MutexHandle::New(H.__ptr);
  const win::NtStatus S = win_close_mutant(nt_handle);
  __hc_invariant($NtSuccess(S));
}

void S::RawMtxHandle::Lock(RawMtxHandle H) {
  __hc_invariant(H.isInitialized());
  const auto nt_handle = win::MutexHandle::New(H.__ptr);
  const win::NtStatus S = win_wait_single(nt_handle);
  __hc_invariant($NtSuccess(S));
}

[[gnu::flatten]]
void S::RawMtxHandle::LockMs(RawMtxHandle H, usize ms, bool alertable) {
  return RawMtxHandle::Lock(H);
}

i32 S::RawMtxHandle::Unlock(RawMtxHandle H) {
  __hc_invariant(H.isInitialized());
  const auto nt_handle = win::MutexHandle::New(H.__ptr);
  i32 last_count = 0;
  [[maybe_unused]] auto S = 
    win_release_mutant(nt_handle, &last_count);
  __hc_invariant($NtSuccess(S));
  return last_count;
}
