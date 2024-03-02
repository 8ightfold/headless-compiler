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
//
//  Internal implementation of IIOFile. 
//  IIOFile::*: IOFile.cpp
//  open_file/close_file: {PLATFORM}/IOFile.Xpp
//  pout/perr/pin: {PLATFORM}/PFiles.cpp
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/EnumBitwise.hpp>
#include <Meta/Traits.hpp>
#include <Sys/Errors.hpp>
#include <Sys/Mutex.hpp>
#include <Sys/IOFileBuf.hpp>
#include <Sys/File.hpp>

// For more info:
// https://github.com/llvm/llvm-project/blob/main/libc/src
// https://doxygen.reactos.org/d2/d1b/sdk_2lib_2crt_2stdio_2file_8c_source.html
// https://github.com/huangqinjin/ucrt/blob/master/stdio

namespace hc::sys {
  struct FileResult {
    constexpr FileResult(usize V) : value(V) { }
    constexpr FileResult(usize V, Error E) : value(V), err(E) { }
    constexpr FileResult(Error E) : value(0UL), err(E) { }
    static constexpr FileResult Ok(usize V) { return {V}; }
    static constexpr FileResult Err(Error E) { return {E}; }
    static constexpr FileResult Err(int E) {
      __hc_invariant(E < int(Error::MaxValue));
      return {0UL, Error(E)};
    }
  public:
    __always_inline constexpr bool isOk() const {
      return err == Error::eNone; 
    }
    __always_inline constexpr bool isErr() const { 
      return err != Error::eNone;
    }
    constexpr explicit operator usize() const {
      return this->value;
    }
  
  public:
    usize value;
    Error err = Error::eNone;
  };

  enum class IIOMode : u32 {
    None        = 0x00,
    Err         = None,
    Read        = 0x01,
    Write       = 0x02,
    Append      = 0x04,
    Plus        = 0x08,
    Binary      = 0x10,
    Exclude     = 0x20,
  };

  $MarkBitwise(IIOMode);

  struct IIOFile {
    using FLockType   = void(IIOFile*);
    using FUnlockType = void(IIOFile*);
    using FReadType   = FileResult(IIOFile*, common::AddrRange);
    using FWriteType  = FileResult(IIOFile*, common::ImmAddrRange);
    using FSeekType   = IOResult<long>(IIOFile*, long, int);
    using FCloseType  = IOResult<>(IIOFile*);
    using RawFlags    = meta::UnderlyingType<IIOMode>;
    using enum Error;
  private:
    enum class IIOOp : u8 {
      None, Read, Write, Seek
    };

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
      IIOFileBuf& buf, BufferMode buf_mode,
      IIOMode mode, bool is_owned = false) : 
     read_fn(read), write_fn(write), seek_fn(seek), close_fn(close),
     mtx(), buf(&buf), pos(0), read_limit(0),
     buf_mode(buf_mode), mode(RawFlags(mode)), last_op(IIOOp::None),
     owning(is_owned), eof(false), err(false) {
      adjustBuf();
      __hc_invariant(bufPtr() || bufSize() == 0);
      // TODO: Fixme
      __hc_assert(buf_mode != BufferMode::Line);
      __hc_assert(!is_owned);
    }

  protected:
    constexpr bool canRead() const {
      return mode & RawFlags(
        IIOMode::Read   |
        IIOMode::Append |
        IIOMode::Plus
      );
    }
    constexpr bool canWrite() const {
      return mode & RawFlags(
        IIOMode::Write  |
        IIOMode::Plus
      );
    }

    common::ImmPtrRange<u8> getSelfRange() const __noexcept {
      const u8* const P = bufPtr();
      return common::PtrRange<>::New(P, bufSize());
    }
    common::PtrRange<u8> getSelfRange() __noexcept {
      return common::PtrRange<>::New(bufPtr(), bufSize());
    }
    common::PtrRange<u8> getSelfPosRange() __noexcept {
      return common::PtrRange<>::New(
        bufPtr() + pos, bufSize() - pos);
    }

  public:
    /// r: Read, w: Write, a: Append, +: Plus, b: Binary, x: Exclude.
    static IIOMode ParseModeFlags(common::StrRef flags);

    void initialize(common::DualString S) { mtx.initialize(S); }
    void initialize() { mtx.initialize(); }
    void lock() { mtx.lock(); }
    void unlock() { mtx.unlock(); }

    u8* bufPtr() const { return buf->buf_ptr; }
    usize bufSize() const { return buf->size; }
    IIOFileBuf& getFileBuf() const { return *buf; }

    //==================================================================//
    // IO
    //==================================================================//

    FileResult readUnlocked(common::AddrRange data);
    FileResult read(common::AddrRange data) {
      FileLock L(this);
      return readUnlocked(data);
    }

    FileResult writeUnlocked(common::ImmAddrRange data);
    FileResult write(common::ImmAddrRange data) {
      FileLock L(this);
      return writeUnlocked(data);
    }

    Error flushUnlocked();
    Error flush() {
      FileLock L(this);
      return flushUnlocked();
    }

    IOResult<long> seek(long offset, int /* whence */);
    IOResult<long> tell();

    IOResult<> close() {
      {
        FileLock L(this);
        if (Error E = flushUnlocked(); E != eNone) {
          // Something fucked happened...
          return $Err(E);
        }
        // Resets the buffer if we set up unget operations.
        buf->reset();
      }

      if (owning) {
        // This shouldn't ever be true...
        // ...for now.
        __hc_unreachable("What...");
      }

      return close_fn(this);
    }

    //==================================================================//
    // Meta
    //==================================================================//

    bool errorUnlocked() const { return err; }
    bool error() {
      FileLock L(this);
      return errorUnlocked();
    }

    bool isEOFUnlocked() const { return eof; }
    bool isEOF() {
      FileLock L(this);
      return isEOFUnlocked();
    }

    void clearerrUnlocked() { this->err = false; }
    void clearerr() {
      FileLock L(this);
      return clearerrUnlocked();
    }
  
  private:
    friend void __init_pfiles();
    friend void __fini_pfiles();
    
    FileResult writeUnlockedNone(common::ImmPtrRange<u8> data);
    FileResult writeUnlockedLine(common::ImmPtrRange<u8> data);
    FileResult writeUnlockedFull(common::ImmPtrRange<u8> data);

    constexpr void adjustBuf() {
      if (canRead() && (bufPtr() == nullptr || bufSize() == 0U)) {
        buf->buf_ptr = &ungetc_buf;
        buf->size = 1U;
        owning = false;
      }
    }

  private:
    FReadType*  read_fn;
    FWriteType* write_fn;
    FSeekType*  seek_fn;
    FCloseType* close_fn;
    Mtx mtx;

    u8 ungetc_buf = 0;
    IIOFileBuf* buf;
    usize pos = 0;

    /// Upper limit of where a read buffer can be read.
    usize read_limit;

    /// If unbuffered, line buffered, or fully buffered.
    BufferMode buf_mode;
    RawFlags mode;

    /// Last operation done by the file.
    IIOOp last_op = IIOOp::None;

    bool owning;
    bool eof = false;
    bool err = false;
  };
} // namespace hc::sys
