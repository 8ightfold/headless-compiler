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
namespace C = hc::common;

namespace {
  /// TODO: Convert to critical section?
  using LazyMtx = C::RawLazy<sys::OSMtx>;
  constinit C::EnumArray<LazyMtx, xcrt::Locks> __lock_tbl_;
  constinit u64 __locks_initialized_ = 0;
} // namespace `anonymous`

extern "C" {
  u64 __xcrt_locks_setup(void) {
    for (LazyMtx& mtx : __lock_tbl_) {
      mtx.ctor();
      mtx->initialize();
      ++__locks_initialized_;
    }
    return __locks_initialized_;
  }

  void __xcrt_locks_shutdown(void) {
    for (u64 I = __locks_initialized_; I > 0; --I) {
      const xcrt::Locks E = xcrt::Locks(I - 1);
      __lock_tbl_[E].dtor();
      --__locks_initialized_;
    }
  }

  sys::OSMtx* __xcrt_get_lock(xcrt::Locks V) {
    using xcrt::Locks;
    __hc_invariant(u64(V) < u64(Locks::MaxValue));
    return __lock_tbl_[V].data();
  }
} // extern "C"
