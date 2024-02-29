//===- Common/Fundamental.hpp ---------------------------------------===//
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
//  Does checks to see if fundamental types are the correct sizes.
//
//===----------------------------------------------------------------===//

#include <Common/Fundamental.hpp>

static_assert(sizeof(ubyte) == 1, "???");
static_assert(__bitsizeof(ubyte) == 8);

static_assert(sizeof(void*) == 8,
  "32 bit mode is currently unsupported.");

static_assert(sizeof(i8)   == 1);
static_assert(sizeof(i16)  == 2);
static_assert(sizeof(i32)  == 4);
static_assert(sizeof(i64)  == 8);
static_assert(sizeof(i128) == 16);

static_assert(sizeof(u8)   == 1);
static_assert(sizeof(u16)  == 2);
static_assert(sizeof(u32)  == 4);
static_assert(sizeof(u64)  == 8);
static_assert(sizeof(u128) == 16);

static_assert(sizeof(f16) == 2);
static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);
