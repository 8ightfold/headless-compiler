//===- Std/__type_traits/desugars_to.hpp ----------------------------===//
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
// Adapted from LLVM's libcxx <__type_traits/desugars_to.h>.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Features.hpp>

namespace std {

// syntactically, the operation is equivalent to calling `a == b`
struct __equal_tag {};

// syntactically, the operation is equivalent to calling `a + b`
struct __plus_tag {};

// syntactically, the operation is equivalent to calling `a < b`
struct __less_tag {};

// syntactically, the operation is equivalent to calling `a > b`
struct __greater_tag {};

// syntactically, the operation is equivalent to calling `a < b`, and these expressions
// have to be true for any `a` and `b`:
// - `(a < b) == (b > a)`
// - `(!(a < b) && !(b < a)) == (a == b)`
// For example, this is satisfied for std::less on integral types, but also for ranges::less on all types due to
// additional semantic requirements on that operation.
struct __totally_ordered_less_tag {};

// This class template is used to determine whether an operation "desugars"
// (or boils down) to a given canonical operation.
//
// For example, `std::equal_to<>`, our internal `std::__equal_to` helper and
// `ranges::equal_to` are all just fancy ways of representing a transparent
// equality operation, so they all desugar to `__equal_tag`.
//
// This is useful to optimize some functions in cases where we know e.g. the
// predicate being passed is actually going to call a builtin operator, or has
// some specific semantics.
template <class CanonTag, class Op, typename...TT>
__global bool __desugars_to_v = false;

} // namespace std
