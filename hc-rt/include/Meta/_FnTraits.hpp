//===- Meta/_FnTraits.hpp -------------------------------------------===//
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
//  Basic standalone traits related to functions
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Fundamental.hpp>
#include "Traits.hpp"

namespace hc::meta {
  template <typename T> 
  __add_rvalue_reference(T) Decl() noexcept {
    $compile_failure(T, 
      "Decl<...> must be called in an unevaluated context.")
  }

  template <typename F, typename...TT>
  using __return_t = decltype(Decl<F>()(Decl<TT>()...));

  template <typename From, typename To>
  concept __convertible = __is_convertible(From, To);

  template <typename To, typename...From>
  concept __all_convertible = (true && ... && __convertible<From, To>);
} // namespace hc::meta
