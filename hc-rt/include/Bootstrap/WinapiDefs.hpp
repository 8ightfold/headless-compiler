//===- Bootstrap/WinapiDefs.hpp -------------------------------------===//
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
//  Defines structures used by the public Windows API (in this project).
//  Other parts may be defined in Sys/Win/Nt/*.hpp
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Features.hpp>
#include <Common/Fundamental.hpp>
#include <Bootstrap/LargeInteger.hpp>
#include <Bootstrap/UnicodeString.hpp>

//======================================================================//
// Aliases
//======================================================================//

namespace hc {
namespace sys {
  struct IIOFile;
  struct RawOSSemaphore;
} // namespace sys

using RawIOFile = sys::IIOFile;
using IOFile    = RawIOFile*;

namespace bootstrap {
  using Win64Addr       = void*;
  using Win64AddrRange  = common::AddrRange;
  using Win64Handle     = hc::__void*;
  using Win64Lock       = sys::RawOSSemaphore;
  using Win64Bool       = ubyte;

  using Win64LargeInt       = LargeInt;
  using Win64ULargeInt      = ULargeInt;
  using Win64UnicodeString  = UnicodeString;
} // namespace bootstrap
} // namespace hc

//======================================================================//
// New Definitions
//======================================================================//

namespace hc::bootstrap {
  struct KSystemTime {
    u32 low;
    i32 high;
    i32 high_alt;
  };

  struct ClientID {
    Win64Handle unique_process;
    Win64Handle unique_thread;
  };

  struct ListEntry {
    ListEntry* prev;
    ListEntry* next;
  };

  struct alignas(u32) ProcessorNumber {
    u16   group;
    ubyte number;
    ubyte __reserved;
  };
} // namespace hc::bootstrap
