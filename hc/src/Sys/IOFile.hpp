//===- Sys/IOFile.hpp -----------------------------------------------===//
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

#ifndef _HC_IOFILE_STNL
# define _HC_IOFILE_STNL
# include_next <Sys/IOFile.hpp>
#endif // _HC_IOFILE_STNL

#include <Common/Fundamental.hpp>
#include <Common/Result.hpp>
#include <Sys/Mutex.hpp>

// For more info:
// https://github.com/llvm/llvm-project/blob/main/libc/src
// https://doxygen.reactos.org/d2/d1b/sdk_2lib_2crt_2stdio_2file_8c_source.html
// https://github.com/huangqinjin/ucrt/blob/master/stdio

namespace hc::sys {
  template <typename T>
  using IOResult = common::Result<T, int>;

  struct FileResult {
    constexpr FileResult(usize V) : value(V) { }
    constexpr FileResult(usize V, int E) : value(V), err(E) { }
  public:
    __always_inline constexpr bool isOk()  const { return err == 0; }
    __always_inline constexpr bool isErr() const { return err != 0; }

    constexpr explicit operator usize() const {
      __hc_invariant(isOk());
      return this->value;
    }
  
  public:
    usize value;
    int err = 0;
  };

  struct IIOFile {
    using FLockType   = void(IIOFile*);
    using FUnlockType = void(IIOFile*);
    using FWriteType  = FileResult(IIOFile*, const void*, usize);
    using FReadType   = FileResult(IIOFile*, void*, usize);
    using FSeekType   = IOResult<long>(IIOFile*, long, int);
    using FCloseType  = int(IIOFile*);
    using RawFlags    = u32;
  public:
    
  };
} // namespace hc::sys
