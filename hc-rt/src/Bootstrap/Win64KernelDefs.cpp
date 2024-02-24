//===- Bootstrap/Win64KernelDefs.cpp --------------------------------===//
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

#include <Bootstrap/Win64KernelDefs.hpp>
#include <Common/Strings.hpp>

static_assert(sizeof(void*) == 8, 
  "Do not use these definitions with 32-bit executables.");

using namespace hc;
using namespace hc::bootstrap;
namespace B = hc::bootstrap;
namespace C = hc::common;

namespace {
  /// Load from segment register at `offset`.
  __always_inline void* __load_seg_offset(uptr offset) {
    void* raw_addr = nullptr;
    __asm__(
     "mov %%gs:%[off], %[ret]"
     : [ret] "=r"(raw_addr)
     : [off]  "m"(*reinterpret_cast<u64*>(offset))
    );
    return raw_addr;
  }
} // namespace `anonymous`

// UnicodeString

Win64UnicodeString Win64UnicodeString::New(wchar_t* S) {
  __hc_invariant(S != nullptr);
  Win64UnicodeString new_ustr;
  usize str_len = C::__wstrlen(S);
  new_ustr.buffer = S;
  new_ustr.size = str_len * 2;
  new_ustr.size_max = (str_len + 1) * 2;
  return new_ustr;
}

Win64UnicodeString Win64UnicodeString::New(wchar_t* S, usize max) {
  __hc_invariant(S != nullptr);
  Win64UnicodeString new_ustr;
  new_ustr.buffer = S;
  new_ustr.size = C::__wstrlen(S) * 2;
  new_ustr.size_max = max * 2;
  __hc_invariant(new_ustr.size <= new_ustr.size_max);
  return new_ustr;
}

[[gnu::flatten]]
Win64UnicodeString Win64UnicodeString::New(C::PtrRange<wchar_t> R) {
  __hc_invariant(!R.isEmpty());
  return Win64UnicodeString::New(R.data(), R.size());
}

bool Win64UnicodeString::isEqual(const Win64UnicodeString& rhs) const {
  if __expect_false(!this->buffer || !rhs.buffer) return false;
  if (this->size != rhs.size) return false;
  int ret = C::__wstrncmp(this->buffer, rhs.buffer, this->size);
  return (ret == 0);
}

// LDR

Win64ListEntryNode* Win64ListEntryNode::GetBaseNode() {
  return Win64TEB::LoadPEBFromGS()->LDR_data->getBaseEntryNode();
}

template struct B::TWin64ListEntry<Win64ModuleType::loadOrder>;
template struct B::TWin64ListEntry<Win64ModuleType::memOrder>;
template struct B::TWin64ListEntry<Win64ModuleType::initOrder>;

// PEB

Win64InitOrderList* Win64PEB::getLDRModulesInInitOrder() const {
  __hc_invariant(LDR_data->is_initialized);
  return this->LDR_data->getEntryNodeAt<B::Win64ModuleType::initOrder>();
}

Win64MemOrderList*  Win64PEB::getLDRModulesInMemOrder() const {
  __hc_invariant(LDR_data->is_initialized);
  return this->LDR_data->getEntryNodeAt<Win64ModuleType::memOrder>();
}

Win64LoadOrderList* Win64PEB::getLDRModulesInLoadOrder() const {
  __hc_invariant(LDR_data->is_initialized);
  return this->LDR_data->getEntryNodeAt<Win64ModuleType::loadOrder>();
}

// TEB

Win64TEB* Win64TEB::LoadTEBFromGS() {
  static constexpr uptr offset = $offsetof(TEB_addr, Win64TIB);
  void* raw_addr = __load_seg_offset(offset);
  __hc_invariant(raw_addr != nullptr);
  return static_cast<Win64TEB*>(raw_addr);
}

Win64PEB* Win64TEB::LoadPEBFromGS() {
  return LoadTEBFromGS()->getPEB();
}

Win64AddrRange Win64TEB::getStackRange() const {
  return C::AddrRange::New(tib.stack_end, tib.stack_begin);
}

Win64ProcParams* Win64TEB::GetProcessParams() {
  return LoadPEBFromGS()->process_params;
}

uptr Win64TEB::getProcessId() const {
  return reinterpret_cast<uptr>(this->process_id);
}

uptr Win64TEB::getThreadId() const {
  return reinterpret_cast<uptr>(this->thread_id);
}

Win64PEB* Win64TEB::getPEB() const {
  return static_cast<Win64PEB*>(this->PEB_addr);
}
