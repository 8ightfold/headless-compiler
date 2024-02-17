//===- Sys/IOFileBuf.hpp --------------------------------------------===//
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
//  Statically sized binary buffer & handle.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/Fundamental.hpp>
#include <Common/Limits.hpp>

namespace hc::sys {
  struct IIOFileBufBase {
    constexpr IIOFileBufBase() = default;
  protected:
    template <usize N>
    constexpr IIOFileBufBase(u8(&B)[N]) 
     : size(N), buf_ptr(B) { }
  public:
    usize size = 0;
    u8* buf_ptr = nullptr;
    usize written = Max<usize>;
  };

  template <usize BufLen>
  struct IIOFileBuf : IIOFileBufBase {
    using BufType = u8[BufLen];
    static constexpr usize bufLen = BufLen;
  public:
    constexpr IIOFileBuf() 
     : IIOFileBufBase(__buf) { }
    
    constexpr IIOFileBufBase& asBase() {
      return static_cast<IIOFileBufBase&>(*this);
    }
  public:
    BufType __buf {};
  };

  template <>
  struct IIOFileBuf<0> : IIOFileBufBase {
    using BufType = void;
    static constexpr usize bufLen = 0;
  public:
    constexpr IIOFileBuf() 
     : IIOFileBufBase() { }
    
    constexpr IIOFileBufBase& asBase() {
      return static_cast<IIOFileBufBase&>(*this);
    }
  };
} // namespace hc::sys
