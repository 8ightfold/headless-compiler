//===- GlobalXtors.hpp ----------------------------------------------===//
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

#pragma once

extern "C" {
  typedef void(*XtorFunc)(void);

  extern XtorFunc __CTOR_LIST__[];
  extern XtorFunc __DTOR_LIST__[];

  extern void __do_global_ctors(void);
  extern void __do_global_dtors(void);
} // extern "C"
