//===- Phase1/Locks.hpp ---------------------------------------------===//
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

#include <Locks.hpp>
#include <Common/EnumArray.hpp>
#include <Common/RawLazy.hpp>

using namespace hc;
using namespace xcrt;

namespace {
#if _XCRT_BASIC_LOCK
constinit EnumArray<LockType, Locks> __lock_tbl_ {};
#else
/// TODO: Convert to critical section?
using LazyMtx = RawLazy<LockType>;
constinit EnumArray<LazyMtx, Locks> __lock_tbl_ {};
#endif

constinit u64 __locks_initialized_ = 0;

LockType& getLockCommon(Locks V) {
  __hc_invariant(u64(V) < u64(Locks::MaxValue));
#if _XCRT_BASIC_LOCK
  return __lock_tbl_[V];
#else
  return __lock_tbl_[V].unwrap();
#endif
}

} // namespace `anonymous`

extern "C" {

u64 __xcrt_locks_setup(void) {
#if !_XCRT_BASIC_LOCK
  for (LazyMtx& mtx : __lock_tbl_) {
    mtx.ctor();
    mtx->initialize();
    ++__locks_initialized_;
  }
#else
  __locks_initialized_ = u64(Locks::MaxValue);
#endif
  return __locks_initialized_;
}

void __xcrt_locks_shutdown(void) {
#if !_XCRT_BASIC_LOCK
  for (u64 I = __locks_initialized_; I > 0; --I) {
    const Locks E = Locks(I - 1);
    __lock_tbl_[E].dtor();
    --__locks_initialized_;
  }
#else
  __locks_initialized_ = 0;
#endif
}

LockType* __xcrt_get_lock(Locks V) {
  return &getLockCommon(V);
}

} // extern "C"

LockType& xcrt::get_lock(Locks V) {
  return getLockCommon(V);
}
