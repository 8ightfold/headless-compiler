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

#include <Common/Result.hpp>
#include <Sys/Mutex.hpp>
#include "IOFileBuf.hpp"

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
    static constexpr FileResult Ok(usize V) { return {V}; }
    static constexpr FileResult Err(int E) { return {0UL, E}; }
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
    using FReadType   = FileResult(IIOFile*, void*, usize);
    using FWriteType  = FileResult(IIOFile*, const void*, usize);
    using FSeekType   = IOResult<long>(IIOFile*, long, int);
    using FCloseType  = int(IIOFile*);
    using RawFlags    = u32;

    struct FileLock {
      FileLock(IIOFile* f) : file(f) { file->lock(); }
      FileLock(IIOFile& f) : FileLock(&f) { }
      FileLock(const FileLock&) = delete;
      FileLock& operator=(const FileLock&) = delete;
      ~FileLock() { file->unlock(); }
    private:
      IIOFile* file;
    };

  public:
    constexpr IIOFile(
      FReadType* read, FWriteType* write,
      FSeekType* seek, FCloseType* close,
      IIOFileBufBase& buf) 
     : read_fn(read), write_fn(write), seek_fn(seek), close_fn(close),
      mtx(), buf_size(buf.size), buf_ptr(buf.buf_ptr), pos(0)
    {

    }

  public:
    void initialize() {
      mtx.initialize();
    }

    void lock() { mtx.lock(); }
    void unlock() { mtx.unlock(); }

  private:
    FReadType*  read_fn;
    FWriteType* write_fn;
    FSeekType*  seek_fn;
    FCloseType* close_fn;
    Mtx mtx;
    usize buf_size = 0;
    u8* buf_ptr = nullptr;
    usize pos = 0;
    bool eof = false;
    bool err = false;
  };
} // namespace hc::sys
