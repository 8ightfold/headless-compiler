//===- Sys/Mutex.hpp ------------------------------------------------===//
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
//
//  The default mutex on a specific platform/implementation.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Features.hpp>
#include "AtomicMutex.hpp"
#include "_EmptyMutex.hpp"

namespace hc::sys {

#if _HC_MULTITHREADED
using _MtxBase = AtomicMtx;
#else
using _MtxBase = _EmptyMtx;
#endif

struct Mtx : public _MtxBase {
  using _MtxBase::_MtxBase;
  using _MtxBase::tryLock;
  using _MtxBase::lock;
  using _MtxBase::unlock;
};

} // namespace hc::sys
