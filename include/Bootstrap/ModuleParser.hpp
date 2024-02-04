//===- Bootstrap/ModuleParser.hpp -----------------------------------===//
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
//  This file defines a utility to parse modules that are loaded
//  into the current process. Windows only.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Option.hpp>
#include <BinaryFormat/COFF.hpp>

namespace hc::binfmt {
  struct Consumer;
} // namespace hc::binfmt

namespace hc::bootstrap {
  struct Win64LDRDataTableEntry;
  using ModuleHandle = Win64LDRDataTableEntry*;
  using DualString = common::PtrUnion<const char, const wchar_t>;
  using ImageConsumer = binfmt::Consumer;
  namespace COFF = hc::binfmt::COFF;

  struct COFFHeader {
    COFF::DosHeader*      dos  = nullptr;
    COFF::FileHeader*     file = nullptr;
    COFF::OptPEHeader     opt;
    COFF::PEWindowsHeader win;
  };

  struct COFFTables {
    COFF::DataDirectoryTable data_dirs;
    COFF::SectionTable sections;
  };

  class COFFModule {
    friend class ModuleParser;
    COFFModule() = default;
    COFFModule(ModuleHandle H) : __image(H) { }
  public:
    COFFModule(const COFFModule&) = default;
    COFFModule(COFFModule&&) = default;
    COFFModule& operator=(const COFFModule&) = default;
    COFFModule& operator=(COFFModule&&) = default;
  public:
    const COFFHeader& getHeader() const& { return __header; }
    const COFFTables& getTables() const& { return __tables; }
    common::AddrRange getImageRange() const;
    ModuleHandle operator->() const;
  private:
    COFFHeader   __header;
    COFFTables   __tables;
    ModuleHandle __image = nullptr;
  };

  using OptCOFFModule = common::Option<COFFModule>;

  class ModuleParser {
    ModuleParser(COFFModule& M, ImageConsumer& C) : mod(M), IC(C) { }
  public:
    /// Don't assume these will always be valid.
    /// If a module is unloaded, it'll no longer work.
    /// This is ok for stuff like ntdll, but not for user dlls.
    static ModuleHandle  GetModuleHandle(DualString name);
    static OptCOFFModule Parse(ModuleHandle handle); 
    static OptCOFFModule GetParsedModule(DualString name);
  private:
    [[nodiscard]] bool runParser();
    COFF::FileHeader* parseHeader();
    COFF::PEWindowsHeader& parseOpt(u32& size);
    void parseTables(u32 RVAs, COFF::FileHeader* fh);
  private:
    COFFModule&    mod;
    ImageConsumer& IC;
  };
} // namespace hc::bootstrap
