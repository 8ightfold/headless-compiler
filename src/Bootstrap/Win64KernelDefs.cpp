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
#include <Common/Memory.hpp>
#include <Common/Strings.hpp>

static_assert(sizeof(void*) == 8, 
  "Do not use these definitions with 32-bit executables.");

using namespace hc;
namespace C = ::hc::common;
namespace B = ::hc::bootstrap;

/// Load from segment register at `offset`.
[[gnu::always_inline, gnu::nodebug]]
static inline void* __load_seg_offset(uptr offset) {
  void* raw_addr = nullptr;
  __asm__(
   "mov %%gs:%[off], %[ret]"
   : [ret] "=r"(raw_addr)
   : [off]  "m"(*reinterpret_cast<u64*>(offset))
  );
  return raw_addr;
}

// UnicodeString

B::Win64UnicodeString B::Win64UnicodeString::New(wchar_t* str) {
  __hc_assert(str != nullptr);
  B::Win64UnicodeString new_ustr;
  usize str_len = C::__wstrlen(str);
  new_ustr.buffer = str;
  new_ustr.size = str_len * 2;
  new_ustr.size_max = (str_len + 1) * 2;
  return new_ustr;
}

B::Win64UnicodeString B::Win64UnicodeString::New(wchar_t* str, usize max) {
  __hc_assert(str != nullptr);
  B::Win64UnicodeString new_ustr;
  new_ustr.buffer = str;
  new_ustr.size = C::__wstrlen(str) * 2;
  new_ustr.size_max = max * 2;
  __hc_assert(new_ustr.size <= new_ustr.size_max);
  return new_ustr;
}

bool B::Win64UnicodeString::isEqual(const B::Win64UnicodeString& rhs) const {
  if __expect_false(!this->buffer || !rhs.buffer) return false;
  if(this->size != rhs.size) return false;
  int ret = C::__wstrncmp(this->buffer, rhs.buffer, this->size);
  return (ret == 0);
}

// LDR

B::Win64ListEntryNode* B::Win64ListEntryNode::GetBaseNode() {
  return B::Win64TEB::LoadTEBFromGS()->getPEB()->LDR_data->getBaseEntryNode();
}

template struct B::TWin64ListEntry<0U>;
template struct B::TWin64ListEntry<1U>;
template struct B::TWin64ListEntry<2U>;

// PEB

B::Win64InitOrderList* B::Win64PEB::getLDRModulesInInitOrder() {
  __hc_assert(LDR_data->is_initialized);
  return this->LDR_data->getEntryNodeAt<2U>();
}

B::Win64MemOrderList*  B::Win64PEB::getLDRModulesInMemOrder() {
  __hc_assert(LDR_data->is_initialized);
  return this->LDR_data->getEntryNodeAt<1U>();
}

B::Win64LoadOrderList* B::Win64PEB::getLDRModulesInLoadOrder() {
  __hc_assert(LDR_data->is_initialized);
  return this->LDR_data->getEntryNodeAt<0U>();
}

// TEB

B::Win64TEB* B::Win64TEB::LoadTEBFromGS() {
  static constexpr uptr offset = $offsetof(TEB_addr, B::Win64TIB);
  void* raw_addr = __load_seg_offset(offset);
  __hc_assert(raw_addr != nullptr);
  return static_cast<B::Win64TEB*>(raw_addr);
}

B::Win64AddrRange B::Win64TEB::getStackRange() const {
  return { tib.stack_end, tib.stack_begin };
}

uptr B::Win64TEB::getProcessId() const {
  return reinterpret_cast<uptr>(this->process_id);
}

uptr B::Win64TEB::getThreadId() const {
  return reinterpret_cast<uptr>(this->thread_id);
}

B::Win64PEB* B::Win64TEB::getPEB() const {
  return static_cast<B::Win64PEB*>(this->PEB_addr);
}
