//===- Bootstrap/ModuleParser.cpp -----------------------------------===//
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
#include <BinaryFormat/Consumer.hpp>
#include <Common/Unwrap.hpp>

using namespace hc::bootstrap;
using hc::binfmt::MMagic;
namespace C = hc::common;
namespace B = hc::bootstrap;

static inline const Win64PEB* __get_PEB() {
  static thread_local const Win64PEB* ppeb =
    Win64TEB::LoadTEBFromGS()->getPEB();
  return ppeb;
}

C::AddrRange B::COFFModule::getImageRange() const {
  return __image->getImageRange();
}

ModuleHandle B::COFFModule::operator->() const {
  __hc_invariant(__image != nullptr);
  return this->__image;
}

// Parser API

ModuleHandle B::ModuleParser::GetModuleHandle(DualString name) {
  auto* modules = __get_PEB()->getLDRModulesInMemOrder();
  return name.visitR([modules] (auto* str) {
    return modules->findModule(str);
  });
}

COFFModule B::ModuleParser::Parse(ModuleHandle handle) {
  COFFModule M(handle);
  ImageConsumer IC;
  ModuleParser P(M, IC);
  if (!P.runParser()) {
    // TODO: Handle failure
  }
  return M;
}

COFFModule B::ModuleParser::GetParsedModule(DualString name) {
  const auto handle = ModuleParser::GetModuleHandle(name);
  // TODO: Handle null case
  return ModuleParser::Parse(handle);
}

// Parser Impl

COFF::FileHeader* B::ModuleParser::parseHeader() {
  COFFHeader& H = mod.__header;
  H.dos = IC.intoIfMatches<COFF::DosHeader>(MMagic::DosHeader);
  if (H.dos != nullptr)
    IC.dropRaw(H.dos->coff_header_offset);
  H.file = IC.consumeIfMatches<COFF::FileHeader>(MMagic::COFFHeader);
  return H.file;
}

COFF::PEWindowsHeader& B::ModuleParser::parseOpt(u32& size) {
  using COFF::OptPEWindowsHeader;
  COFFHeader& H = mod.__header;
  if (IC.matches(BF::MMagic::COFFOptPE64)) {
    H.opt = IC.consumeAndSub<COFF::OptPE64Header>(size);
    H.win = IC.consumeAndSub<OptPEWindowsHeader<8>>(size);
  } else if (IC.matches(BF::MMagic::COFFOptPE32)) {
    H.opt = IC.consumeAndSub<COFF::OptPE32Header>(size);
    H.win = IC.consumeAndSub<OptPEWindowsHeader<4>>(size);
  }
  return H.win;
}

void B::ModuleParser::parseTables(u32 RVAs, COFF::FileHeader* fh) {
  COFFTables& tbls = mod.__tables;
  tbls.data_directories = IC.consumeRange<COFF::DataDirectoryHeader>(RVAs);
  tbls.sections = IC.consumeRange<COFF::SectionHeader>(fh->section_count);
}

bool B::ModuleParser::runParser() {
  IC.reInit(mod->getImageRange());
  COFF::FileHeader* file_header = this->parseHeader();
  u32 opt_size = $unwrap(file_header).optional_header_size;
  const u32 RVA_count = this->parseOpt()
    .$extract_member(RVA_and_sizes_count);
  this->parseTables(RVA_count, file_header);
  return true;
}
