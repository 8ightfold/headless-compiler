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

#include <Common/StaticVec.hpp>
#include <BinaryFormat/COFF.hpp>

namespace hc::bootstrap {
  struct Win64LDRDataTableEntry;
  using ModuleHandle = Win64LDRDataTableEntry*;
  namespace COFF = ::hc::binfmt::COFF;

  struct COFFHeader {
    COFF::DosHeader*      dos = nullptr;
    COFF::FileHeader*     file = nullptr;
    COFF::OptPEHeader     opt;
    COFF::PEWindowsHeader win;
  };

  struct COFFTables {
    COFF::DataDirectoryTable data_directories;
    COFF::SectionTable sections;
  };

  struct COFFModule {
    COFFHeader        __header;
    COFFTables        __tables;
    common::AddrRange __image;
  };

  struct ModuleParser {
    // Don't assume these will always be valid.
    static ModuleHandle GetModuleHandle(const char* name);
    static ModuleHandle GetModuleHandle(const wchar_t* name);
  };
} // namespace hc::bootstrap
