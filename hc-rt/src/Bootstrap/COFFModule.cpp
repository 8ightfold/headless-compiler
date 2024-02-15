//===- Bootstrap/COFFModule.cpp -------------------------------------===//
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

#include <Bootstrap/ModuleParser.hpp>
#include <Bootstrap/Win64KernelDefs.hpp>
#include <BinaryFormat/COFF.hpp>
#include <Common/Pair.hpp>
#include <Meta/Unwrap.hpp>
#include <Parcel/StaticVec.hpp>

using namespace hc::bootstrap;
namespace C = hc::common;
namespace B = hc::bootstrap;

namespace {
  // TODO: Add hash/cache?

  struct ExportBSearcher {
    ExportBSearcher(ModuleHandle M,
     COFF::ExportDirectoryTable* EDT) : M(M), EDT(EDT) {
      __hc_invariant(M && EDT);
      this->NPT = M->getRangeFromRVA(EDT->name_pointer_table_RVA)
        .intoRange<COFF::NamePointerType>()
        .takeFront(EDT->name_pointer_table_count);
    }
  public:
    void* operator()(C::StrRef S) const {
      if __expect_false(NPT.isEmpty())
        return nullptr;
      if (i64 pos = bsearch(S); pos >= 0) {
        auto* ords = M->getRVA<COFF::OrdinalType>(EDT->ordinal_table_RVA);
        auto* exports = M->getRVA<u32>(EDT->export_address_table_RVA);
        const COFF::OrdinalType ord = ords[pos];
        __hc_invariant(ord < EDT->export_address_table_count);
        if (!exports[ord]) return nullptr;
        return M->getRVA(exports[ord]);
      }
      return nullptr;
    }

    i64 bsearch(C::StrRef S) const { 
      i64 F = -1, B = i64(NPT.size()) - 1;
      while (F <= B) {
        const i64 middle = (B + F) / 2;
        const auto sym = resolveOffset(middle);
        const auto R = C::__strcmp(sym.data(), S.data());
        if __expect_false(R == 0)
          return middle;
        if (R > 0)
          B = middle - 1;
        else
          F = middle + 1;
      }
      return -1;
    }

    C::StrRef resolveOffset(i64 off) const {
      const auto tbl_off = this->NPT[off];
      return C::StrRef(M->getRVA<char>(tbl_off));
    }

  private:
    ModuleHandle M = nullptr;
    COFF::ExportDirectoryTable* EDT = nullptr;
    C::PtrRange<COFF::NamePointerType> NPT { };
  };
} // namespace `anonymous`

// Resolvers

void* B::COFFModule::resolveExportRaw(C::StrRef S) const {
  static constexpr auto I = u32(COFF::eDirectoryExportTable);
  if (S.isEmpty())
    return nullptr;
  auto& D = getTables().data_dirs;
  if (D.size() < I)
    return nullptr;
  auto* EDT = self()->getRVA<COFF::ExportDirectoryTable>(D[I].RVA);
  ExportBSearcher B {__image, EDT};
  return B(S);
}

void* B::COFFModule::resolveImportRaw(C::StrRef S) const {
  return nullptr;
}

// Getters

C::AddrRange B::COFFModule::getImageRange() const {
  return self()->getImageRange();
}

DualString B::COFFModule::getName() const {
  if __expect_false(!__image)
    return DualString::New();
  const auto name = self()->base_dll_name.buffer;
  return DualString::New(name);
}

ModuleHandle B::COFFModule::operator->() const {
  __hc_invariant(__image != nullptr);
  return this->__image;
}
