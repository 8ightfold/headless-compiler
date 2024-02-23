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

namespace {
  inline win::LargeInt* __mtx_time_fmt(win::LargeInt& I, usize ms) {
    if __expect_false(ms == hc::Max<usize>)
      return nullptr;
    const i64 nt_ms = i64(ms) * -10000LL;
    return &(I = nt_ms);
  }

  template <bool Alertable = false>
  inline void __mtx_lock_ms(RawMtxHandle H, usize ms, bool) {
    static constexpr win::NtStatus alerted = 0x101;
    const auto nt_handle = win::MutexHandle::New(H.__ptr);
    win::NtStatus S = 0;
    win::LargeInt I;
    if constexpr (Alertable)
      __hc_unreachable("Fuck!");
    win::LargeInt* const P = 
      __mtx_time_fmt(I, ms);
    do {
      S = wait_single(nt_handle);
      if __expect_false($NtFail(S)) {
        // TODO: Set last error
        S = hc::Max<win::NtStatus>; // WAIT_FAILED
      }
    } while((S == alerted) && Alertable);
    __hc_invariant($NtSuccess(S));
  }
} // namespace `anonymous`

RawMtxHandle S::RawMtxHandle::New(const wchar_t* name) {
  win::NtStatus S = 0;
  auto H = create_mutant(S,
    win::MutantAllAccess, name);
  __hc_invariant($NtSuccess(S));
  return RawMtxHandle {H.__data};
}

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
  const win::NtStatus S = close_mutant(nt_handle);
  __hc_invariant($NtSuccess(S));
}

[[gnu::flatten]]
void S::RawMtxHandle::Lock(RawMtxHandle H) {
  __hc_invariant(H.isInitialized());
  const auto nt_handle = win::MutexHandle::New(H.__ptr);
  const win::NtStatus S = wait_single(nt_handle);
  __hc_invariant($NtSuccess(S));
}

void S::RawMtxHandle::LockMs(RawMtxHandle H, usize ms, bool alertable) {
  /// Max<usize> is the same as winapi's INFINITE.
  /// This means we do not need to time the wait period.
  if __expect_false(ms == hc::Max<usize>)
    return RawMtxHandle::Lock(H);
  if __expect_false(alertable)
    $tail_return __mtx_lock_ms<true>(H, ms, alertable);
  // else:
  $tail_return __mtx_lock_ms<>(H, ms, alertable);
}

[[gnu::flatten]]
i32 S::RawMtxHandle::Unlock(RawMtxHandle H) {
  __hc_invariant(H.isInitialized());
  const auto nt_handle = win::MutexHandle::New(H.__ptr);
  i32 last_count = 0;
  [[maybe_unused]] auto S = 
    release_mutant(nt_handle, &last_count);
  __hc_invariant($NtSuccess(S));
  return last_count;
}
