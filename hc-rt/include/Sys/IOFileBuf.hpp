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
  struct IIOFileBuf {
    constexpr IIOFileBuf() = default;
  protected:
    template <usize N>
    constexpr IIOFileBuf(u8(&B)[N]) :
     buf_ptr(B), size(N), __true_size(N) { }
  public:
    constexpr usize getTrueSize() const {
      return this->__true_size;
    }
    constexpr void reset() {
      if (__true_size > 0U)
        return;
      buf_ptr = nullptr;
      size = 0U;
    }

  public:
    u8* buf_ptr = nullptr;
    usize size = 0;
    const usize __true_size = 0;
  };

  template <usize BufLen>
  struct IIOFileArray : IIOFileBuf {
    using BufType = u8[BufLen];
    static constexpr usize bufLen = BufLen;
  public:
    constexpr IIOFileArray() 
     : IIOFileBuf(__buf) { }
    
    constexpr IIOFileBuf& asBase() {
      return static_cast<IIOFileBuf&>(*this);
    }
  public:
    BufType __buf {};
  };

  template <>
  struct IIOFileArray<0> : IIOFileBuf {
    using BufType = void;
    static constexpr usize bufLen = 0;
  public:
    constexpr IIOFileArray() : IIOFileBuf() { }
    constexpr IIOFileBuf& asBase() {
      return static_cast<IIOFileBuf&>(*this);
    }
  };
} // namespace hc::sys
