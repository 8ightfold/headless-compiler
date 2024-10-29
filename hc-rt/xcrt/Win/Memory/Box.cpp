//===- Memory/Box.hpp -----------------------------------------------===//
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

#include <Memory/Box.hpp>
#include <Common/DefaultFuncPtr.hpp>
#include <Bootstrap/_NtModule.hpp>
#include <Bootstrap/Win64KernelDefs.hpp>
#include <Meta/Unwrap.hpp>

#define $load_symbol(name) \
  succeeded &= load_nt_symbol(name, #name)

using namespace hc;
using namespace hc::bootstrap;

namespace {

using LOGICAL = u32;
using AllocType = void*(void* heap_ptr, u32 flags, u32 size);
using FreeType = LOGICAL(void* heap_ptr, u32 flags, void* alloc);

__imut DefaultFuncPtr<AllocType> RtlAllocateHeap {};
__imut DefaultFuncPtr<FreeType>  RtlFreeHeap {};
__imut Win64Handle processHeap = nullptr;

template <typename F>
static bool load_nt_symbol(
 DefaultFuncPtr<F>& func, StrRef symbol) {
  if __expect_true(func.isSet())
    return true;
  auto exp = __NtModule()->resolveExport<F>(symbol);
  return func.setSafe($unwrap(exp));
}

static bool load_heap() {
  if (processHeap)
    return true;
  processHeap = HcCurrentPEB()->process_heap;
  return !!processHeap;
}

} // namespace `anonymous`

void* XCRT_NAMESPACE::box_heap_alloc(
 usize size, XCRT_NAMESPACE::HeapAllocFlags flags) {
  auto hflags = static_cast<u32>(flags);
#if !_HC_MULTITHREADED
  hflags |= XCRT_NAMESPACE::HA_NoSerialize;
#endif
  return RtlAllocateHeap(processHeap, hflags, size);
}

bool XCRT_NAMESPACE::box_heap_free(
 void* ptr, bool no_serialize) {
  const auto hflags
#if _HC_MULTITHREADED
    = no_serialize ? HA_NoSerialize : HA_None;
#else
    = HA_NoSerialize;
#endif
  return !!RtlFreeHeap(processHeap, hflags, ptr);
}

bool XCRT_NAMESPACE::setup_heap_funcs() {
  static bool has_succeeded = false;
  if __expect_false(!has_succeeded) {
    bool succeeded = true;
    $load_symbol(RtlAllocateHeap);
    $load_symbol(RtlFreeHeap);
    succeeded &= load_heap();
    has_succeeded = succeeded;
  }
  return has_succeeded;
}
