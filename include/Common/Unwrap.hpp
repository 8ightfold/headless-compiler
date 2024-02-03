//===- Common/Unwrap.hpp --------------------------------------------===//
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
//  A utility for generically returning on error.
//
//===----------------------------------------------------------------===//

#pragma once

#include "Tuple.hpp"

#define $unwrap(obj, on_err...) ({ if(!obj) return ::hc::_Wrapper{on_err}; *obj; })

namespace hc {
  template <typename...TT>
  struct _Wrapper {
    template <typename U>
    constexpr operator U() {
      using Ids = common::make_idxseq<sizeof...(TT)>;
      return [this] <usize...II> 
       (common::IdxSeq<II...>) -> U {
        return U(__hc_fwd(__tup[__i<II>])...);
      }(Ids{});
    }

  public:
    common::Tuple<TT...> __tup;
  };

  template <typename...TT>
  _Wrapper(TT&&...) -> _Wrapper<TT...>;
} // namespace hc
