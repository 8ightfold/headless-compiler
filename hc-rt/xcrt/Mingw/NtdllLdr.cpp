//===- NtdllLdr.cpp -------------------------------------------------===//
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

#include "NtdllLdr.hpp"
#include <Bootstrap/ModuleParser.hpp>
#include <Bootstrap/Win64KernelDefs.hpp>
#include <Meta/Unwrap.hpp>

using namespace hc::bootstrap;

extern "C" {
  static COFFModule* __xcrt_get_pmodule(void) {
    static auto M = [] -> OptCOFFModule {
      auto* H = Win64TEB::LoadPEBFromGS()
        ->getLDRModulesInMemOrder()
        ->findModule(L"ntdll.dll");
      if (!H) return $None();
      return ModuleParser::Parse(H);
    }();
    return &$unwrap(M, nullptr);
  }

  void* __xcrt_load_ntdll_func(const char* sym) {
    return $unwrap(__xcrt_get_pmodule()).resolveExportRaw(sym);
  }
}
