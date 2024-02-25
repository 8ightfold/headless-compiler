//===- Sys/Win/PathNormalizer.hpp -----------------------------------===//
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
//  This file defines an object which can be used to normalize paths.
//  VERY useful for interacting with the filesystem.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Common/StrRef.hpp>
#include <Common/PtrRange.hpp>
#include <Meta/ExTraits.hpp>
#include <Parcel/StaticVec.hpp>
#include <Sys/Errors.hpp>

namespace hc::sys {
  static constexpr usize path_max = RT_MAX_PATH;
  using UPathType = parcel::StaticVec<wchar_t, path_max>;
  using PathRef = common::PtrRange<wchar_t>;
  using ImmPathRef = common::ImmPtrRange<wchar_t>;

  /// Swapped "\" with "/" because comment syntax is
  /// fucking stupid and it can't decide what to escape.
  /// Just swap out any of those in your head, they get converted 
  /// to backslashes either way. Also used `~` in place of `...`,
  /// as the latter is technically a valid path under Nt.
  ///
  /// If a path begins with `//?/`, it is not normalized,
  /// but still subject to path length limitations (buffered).
  enum class PathType : u32 {
    Unknown = 0,
    DosDrive,       // `//./[drive]/~`
    DosVolume,      // `//?/[volume]:/~`
    DeviceUNC,      // `//[.|?]/UNC/~`*
    UNCNamespace,   // `//[name]/~`*
    NtNamespace,    // `//??/~` or `//GLOBAL??/~`
    LegacyDevice,   // `[CON|COM[1-9]|...]/~`
    QualDOS,        // `[drive]:/`
    DriveRel,       // `[drive]:~`
    CurrDriveRel,   // `/~`
    DirRel,         // `~`
  };

  struct [[gsl::Owner]] PathNormalizer {
    using enum PathType;
    static PathType GetPathType(common::StrRef);
  public:
    constexpr PathNormalizer() = default;
    bool operator()(common::StrRef path);
    bool operator()(ImmPathRef wpath);
    UPathType&& take() { return __hc_move(path); }
    PathRef  getPath() { return path.intoRange(); }
    PathType getType() const { return type; }
    Error getLastError() const { return err; }
    bool didError() const {
      return err != Error::eNone;
    }
  private:
    __always_inline void push(char C) {
      path.emplace(static_cast<wchar_t>(C));
    }
    __always_inline void push(wchar_t C) {
      path.emplace(C);
    }

    template <typename CType, usize N>
    [[gnu::flatten]]
    constexpr void push(const CType(&A)[N]) {
      if __expect_false(isNameTooLong(N))
        return;
      /// Force compiler to unroll loop
      if constexpr (N < 16) {
        [&, this] <usize...II> (common::IdxSeq<II...>) {
          ((this->push(A[II])), ...);
        } (common::make_idxseq<N>());
      } else {
        for (usize I = 0U; I < N; ++I)
          this->push(A[I]);
      }
    }

    void push(common::PtrRange<wchar_t> P);
    void push(common::PtrRange<char> P);

    bool isNameTooLong(const usize N) {
      if __expect_true(N <= path.remainingCapacity())
        return false;
      this->err = Error::eNameTooLong;
      return true;
    }

  private:
    alignas(8) UPathType path {};
    PathType type = PathType::Unknown;
    Error err = Error::eNone;
  };
} // namespace hc::sys
