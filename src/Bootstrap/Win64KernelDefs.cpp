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

B::Win64LDRDataTableEntry* B::Win64ListEntry::asLDRDataTableEntry() const {
  // Stupid fucking windows bullshit
  B::Win64ListEntry* table_base = this->asMutable() - 1;
  auto* pLDR_dte = reinterpret_cast<B::Win64LDRDataTableEntry*>(table_base);
  // This is fine because we know it IS a data table entry.
  return C::__launder(pLDR_dte);
}

B::Win64LDRDataTableEntry* B::Win64ListEntry::findDLL(const wchar_t* str) const {
  if __expect_false(!str) 
    return nullptr;
  B::Win64ListEntry* curr = this->asMutable();
  if __expect_false(curr->isFirst())
    curr = curr->next;
  // This is evil... buuut we never modify the buffer so it's ok ;)
  const auto ustr = B::Win64UnicodeString::New(const_cast<wchar_t*>(str));
  while(!curr->isFirst()) {
    const auto tbl = curr->asLDRDataTableEntry();
    const B::Win64UnicodeString& dll_ustr = tbl->base_dll_name;
    if(dll_ustr.isEqual(ustr)) return tbl;
    curr = curr->next;
  }
  return nullptr;
}

B::Win64LDRDataTableEntry* B::Win64ListEntry::findDLL(const char* str) const {
  // TODO: Implement
  return nullptr;
}

B::Win64ListEntry* B::Win64ListEntry::getFirst() const {
  B::Win64ListEntry* curr = this->asMutable();
  if __expect_false(curr->next->isFirst()) 
    return curr->next;
  while(!curr->isFirst()) {
    curr = curr->prev;
    __hc_assert(curr != nullptr);
  }
  return curr;
}

B::Win64ListEntry* B::Win64ListEntry::getLast() const {
  B::Win64ListEntry* curr = this->asMutable();
  if __expect_false(curr->isFirst()) 
    return curr->prev;
  while(!curr->next->isFirst()) {
    curr = curr->next;
    __hc_assert(curr != nullptr);
  }
  return curr;
}

bool B::Win64ListEntry::isFirst() const {
  auto* tbl = this->asLDRDataTableEntry();
  // RAHHHHHH
  __hc_assert(tbl != nullptr);
  return !tbl->entry_point && !tbl->dll_base;
}

// PEB

[[gnu::always_inline]]
static inline B::Win64ListEntry* 
 __verify_mlist_integrity(B::Win64ListEntry& e) {
  // Make sure our assumption that the first table entry
  // always has a null `.entry_point` is true.
  __hc_assert(e.asLDRDataTableEntry()->entry_point == nullptr);
  return &e;
}

B::Win64ListEntry* B::Win64PEB::getLDRModulesInLoadOrder() {
  const auto data = this->LDR_data;
  __hc_assert(data->is_initialized);
  return __verify_mlist_integrity(data->__mlist_in_load_order);
}

B::Win64ListEntry* B::Win64PEB::getLDRModulesInMemOrder() {
  const auto data = this->LDR_data;
  __hc_assert(data->is_initialized);
  return __verify_mlist_integrity(data->__mlist_in_mem_order)->next;
}

B::Win64ListEntry* B::Win64PEB::getLDRModulesInInitOrder() {
  const auto data = this->LDR_data;
  __hc_assert(data->is_initialized);
  return __verify_mlist_integrity(data->__mlist_in_init_order);
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
