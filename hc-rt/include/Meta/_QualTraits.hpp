//===- Meta/_QualTraits.hpp -----------------------------------------===//
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
//  Basic standalone traits related to qualifiers.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Fundamental.hpp>
#include "Traits.hpp"

namespace hc::meta {

template <typename To, typename From>
struct _TCopyQuals {
  using Type = To;
};

template <typename To, typename From>
struct _TCopyQuals<To, const From> {
  using Type = const To;
};

template <typename To, typename From>
struct _TCopyQuals<To, volatile From> {
  using Type = volatile To;
};

template <typename To, typename From>
struct _TCopyQuals<To, const volatile From> {
  using Type = const volatile To;
};

template <typename To, typename From>
using __copy_quals = typename
  _TCopyQuals<RemoveCV<To>, From>::Type;

//////////////////////////////////////////////////////////////////////////
// Matching

template <typename T, typename U>
concept __matching_quals =
  is_same<__copy_quals<U, T>, U>;

} // namespace hc::meta
