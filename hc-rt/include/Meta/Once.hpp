//===- Meta/Once.hpp ------------------------------------------------===//
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

#include <Common/Features.hpp>

#define $OnceEx(extra...) static \
  ::hc::meta::_Once $var(once) = [extra] ()
#define $Once $OnceEx()

#define $OnExitEx(extra...) static \
  ::hc::meta::_OnceExit $var(onex) = [extra] ()
#define $OnExit $OnExitEx()

namespace hc::meta {
  struct _Once {
    _Once(auto __onentry) {
      (void) __onentry();
    }
  };

  template <typename F>
  struct _OnceExit {
    _OnceExit(const F& f) : __onexit(f) {}
    _OnceExit(F&& f) : __onexit(__hc_move(f)) {}
    ~_OnceExit() { (void) __onexit(); }
  private:
    F __onexit;
  };

  template <typename F>
  _OnceExit(F&&) -> _OnceExit<__remove_cvref(F)>;
} // namespace hc::meta
