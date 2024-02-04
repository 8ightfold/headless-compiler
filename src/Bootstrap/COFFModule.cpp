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

using namespace hc::bootstrap;
namespace C = hc::common;
namespace B = hc::bootstrap;

C::AddrRange B::COFFModule::getImageRange() const {
  return __image->getImageRange();
}

ModuleHandle B::COFFModule::operator->() const {
  __hc_invariant(__image != nullptr);
  return this->__image;
}
