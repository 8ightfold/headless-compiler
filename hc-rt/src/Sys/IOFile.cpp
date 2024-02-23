//===- Sys/IOFile.cpp -----------------------------------------------===//
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

#include <Common/Casting.hpp>
#include <Common/InlineMemcpy.hpp>
#include <Sys/IOFile.hpp>

#define $FileErr(e) S::FileResult::Err(e)

using namespace hc;
using namespace hc::sys;
namespace C = hc::common;
namespace S = hc::sys;

namespace {
  template <typename T, typename U>
  inline void copy_range(C::PtrRange<T> to, C::PtrRange<U> from) {
    static_assert(meta::is_same_size<T, U>);
    __hc_invariant(to.size() >= from.size());
    C::inline_memcpy(to.data(), from.data(), from.sizeInBytes());
  }
} // namespace `anonymous`

IIOMode IIOFile::ParseModeFlags(C::StrRef S) {
  S = S.dropNull();
  if __expect_false(!S.beginsWithAny("rwa"))
    return IIOMode::Err;
  auto flags = IIOMode::None;
  /// Checks if each main mode has only been used once.
  int mmode_count = 0;
  for (char C : S) {
    switch (C) {
     case 'a':
      flags |= IIOMode::Append;
      ++mmode_count;
      break;
     case 'r':
      flags |= IIOMode::Read;
      ++mmode_count;
      break;
     case 'w':
      flags |= IIOMode::Write;
      ++mmode_count;
      break;
     case '+':
      flags |= IIOMode::Plus;
      break;
     case 'b':
      flags |= IIOMode::Binary;
      break;
     case 'x':
      flags |= IIOMode::Exclude;
      break;
     default:
      return IIOMode::Err;
    }
  }
  if __expect_false(mmode_count != 1)
    return IIOMode::Err;
  return flags;
}

FileResult IIOFile::readUnlocked(C::AddrRange data) {
  if __expect_false(!canRead()) {
    err = true;
    return $FileErr(eBadFD);
  }
  last_op = IIOOp::Read;

  auto self_buf = getSelfRange();
  const usize len = data.size();

  __hc_invariant(read_limit > pos);
  usize available_data = read_limit - pos;
  if (len <= available_data) {
    C::inline_memcpy(data.data(),
      self_buf.dropFront(pos).data(), len);
    pos += len;
    return len;
  }

  // Copy all available data to the buffer.
  C::inline_memcpy(
    data.data(),
    self_buf.dropFront(pos).data(),
    available_data
  );
  // Reset position.
  pos = read_limit = 0;

  // Update the output buffer.
  data = data.dropFront(available_data);
  usize to_fetch = len - available_data;
  // Check if output can be buffered
  if (to_fetch > bufSize()) {
    // Unbuffered read into the output buffer.
    auto R = read_fn(this, data);
    usize fetched = R.value;
    if (R.isErr() || fetched < to_fetch) {
      if (R.isOk())
        eof = true;
      else
        err = true;
      return {available_data + fetched, R.err};
    }
    return len;
  }

  auto R = read_fn(this,
    self_buf.intoRange<void>());
  usize fetched = R.value;
  read_limit += fetched;
  usize transfer_size = (fetched >= to_fetch) ? to_fetch : fetched;
  C::inline_memcpy(data.data(), self_buf.data(), transfer_size);
  pos += transfer_size;
  if (R.isErr() || fetched < to_fetch) {
    if (R.isOk())
      eof = true;
    else
      err = true;
  }
  return {available_data + transfer_size, R.err};
}

FileResult IIOFile::writeUnlocked(C::ImmAddrRange data) {
  if __expect_false(!canWrite()) {
    err = true;
    return $FileErr(eBadFD);
  }
  last_op = IIOOp::Write;

  const auto u8data = data.intoImmRange<u8>();
  if (buf_mode == BufferMode::None) {
    auto R = writeUnlockedNone(u8data);
    flushUnlocked();
    return R;
  } else if (buf_mode == BufferMode::Full) {
    return writeUnlockedFull(u8data);
  } else /* BufferMode::Line */ {
    return writeUnlockedLine(u8data);
  }
}

int IIOFile::flushUnlocked() {
  if (last_op == IIOOp::Write && pos > 0) {
    auto R = write_fn(this, 
      getSelfRange().intoRange<void>());
    // Ensure all data was flushed.
    if (R.isErr() || R.value < pos) {
      err = true;
      return R.err;
    }
    pos = 0;
  } else if (last_op == IIOOp::Read && pos > 0) {
    // Discard any pending reads to the file buffer.
    // TODO: Test this...
    pos = read_limit = 0;
  }
  return 0;
}

// impl

FileResult IIOFile::writeUnlockedNone(C::ImmPtrRange<u8> data) {
  if (pos > 0) {
    const usize write_size = pos;
    auto R = write_fn(this, 
      getSelfRange()
        .takeFront(write_size).intoRange<void>());
    pos = 0;
    // Error, not enough bytes were written.
    if (R.value < write_size) {
      err = true;
      return $FileErr(R.err);
    }
  }
  auto R = write_fn(this, 
    data.intoImmRange<void>());
  if (R.value < data.size())
    err = true;
  return R;
}

FileResult IIOFile::writeUnlockedLine(C::ImmPtrRange<u8> data) {
  __hc_unreachable("`writeUnlockedLine` unimplemented.");
  return $FileErr(0);
}

FileResult IIOFile::writeUnlockedFull(C::ImmPtrRange<u8> data) {
  const usize init_pos = pos;
  const usize buf_space = bufSize() - pos;
  const usize len = data.size();

  // The idea is we should be able to write in >2 sections.
  // First, writing to the current buffer, then flushing
  // and writing to the clean buffer. If we do not have
  // sufficient space for this, just write unbuffered.
  if (len > (buf_space + bufSize()))
    $tail_return writeUnlockedNone(data);
  
  // Find the section middle.
  const usize split_pos = 
    (len < buf_space) ? len : buf_space;
  
  // This section is written to the current buffer.
  auto first = data.takeFront(split_pos);
  // This section is written to the flushed buffer (if required).
  // Invariant: `split_pos <= data.size()` 
  auto remainder = data.dropFront(split_pos);

  // Do the primary write.
  copy_range(getSelfPosRange(), first);
  pos += first.size();

  if (remainder.size() == 0)
    return len;
  
  const usize flush_size = pos;
  auto flush_res = write_fn(this, 
    getSelfRange().intoImmRange<void>());
  usize bytes_flushed = flush_res.value;

  pos = 0;
  // If not all data was flushed, an error occured.
  if (flush_res.isErr() || bytes_flushed < flush_size) {
    err = true;
    return {
      bytes_flushed <= init_pos ? 
        0 : bytes_flushed - init_pos,
      flush_res.err
    };
  }

  // If there is space in the buffer to write, then do that.
  // Otherwise, just write unbuffered and leave `pos` at 0.
  if (remainder.size() < bufSize()) {
    copy_range(getSelfRange(), remainder);
    pos = remainder.size();
  } else {
    // Write directly to output.
    auto R = write_fn(this,
      remainder.intoImmRange<void>());
    const usize bytes_written = R.value;

    if (R.isErr() || bytes_written < remainder.size()) {
      err = true;
      return {first.size() + bytes_written, R.err};
    }
  }

  return len;
}
