//===- Common/SSEVec.hpp --------------------------------------------===//
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

#include <Common/Fundamental.hpp>
#include <Common/Array.hpp>

namespace hc::rt {
  using Gv128 = u8 __attribute__((__vector_size__(16)));
  using Gv256 = u8 __attribute__((__vector_size__(32)));
  using Gv512 = u8 __attribute__((__vector_size__(64)));

  template <typename> struct _IsVector { static constexpr bool value = false; };
  template <> struct _IsVector<Gv128>  { static constexpr bool value = true; };
  template <> struct _IsVector<Gv256>  { static constexpr bool value = true; };
  template <> struct _IsVector<Gv512>  { static constexpr bool value = true; };

  template <typename T>
  concept __is_vector = _IsVector<T>::value;

#if defined(__AVX512F__)
  inline constexpr usize __vector_size = 64;
  using v128 = Gv128;
  using v256 = Gv256;
  using v512 = Gv512;
#elif defined(__AVX__)
  inline constexpr usize __vector_size = 32;
  using v128 = Gv128;
  using v256 = Gv256;
  using v512 = hc::common::Array<v256, 2>;
#elif defined(__SSE2__)
  inline constexpr usize __vector_size = 16;
  using v128 = Gv128;
  using v256 = hc::common::Array<v128, 2>;
  using v512 = hc::common::Array<v128, 4>;
#else
  inline constexpr usize __vector_size = 8;
  using v128 = hc::common::Array<u64, 2>;
  using v256 = hc::common::Array<u64, 4>;
  using v512 = hc::common::Array<u64, 8>;
#endif
} // namespace hc::rt
