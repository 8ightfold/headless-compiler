//===- Sys/Win/PathNormalizer.cpp -----------------------------------===//
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

#include <Common/DynAlloc.hpp>
#include <Common/InlineMemcpy.hpp>
#include <Common/Option.hpp>
#include <Common/TaggedUnion.hpp>
#include <Bootstrap/Win64KernelDefs.hpp>
#include <Parcel/StringTable.hpp>
#include <Sys/Args.hpp>
#include <Sys/Win/Volume.hpp>
#include "PathNormalizer.hpp"
#include "_PathUtils.hpp"


#ifndef __XCRT__
# include <cstdio>
# define $printf(...) std::printf(__VA_ARGS__)
#else
# define $printf(...) (void(0))
#endif

using namespace hc;
using namespace hc::sys;
namespace B = hc::bootstrap;
namespace P = hc::parcel;
namespace S = hc::sys;

static constexpr usize maxPathSlices = 64;
using PathSliceType = P::StringTable<RT_MAX_PATH, maxPathSlices>;

struct PathNormalizer::PathDeductionCtx {

};

//======================================================================//
// Identification
//======================================================================//

// ...

//======================================================================//
// Core
//======================================================================//

static char current_drive_letter() {
  const wchar_t prefix = Args::WorkingDir()[0];
  return static_cast<char>(prefix);
}

bool PathNormalizer::operator()(StrRef path) {
  return false;
}

/*
bool PathNormalizer::doNormalization(StrRef S) {
  this->type = deduce_path_type(S);
  if (type == Unknown) {
    err = Error::eInvalName;
    return false;
  }

  PathSliceType path_slices {};
  if (MMatch(type).is(DriveRel, CurrDriveRel, DirRel))
    this->applyRelativePath(path_slices);

  return false;
}

bool PathNormalizer::operator()(StrRef S) {
  path.clear();
  this->err = Error::eNone;
  bool R = doNormalization(S.dropNull());
  this->push(L'\0');
  return R;
}

bool PathNormalizer::operator()(ImmPathRef wpath) {
  path.clear();
  this->err = Error::eNone;
  __hc_todo("operator()(ImmPathRef)", false);
}
*/

//======================================================================//
// Reformatting
//======================================================================//

void PathNormalizer::push(ImmPtrRange<wchar_t> P) {
  if (P.isEmpty())
    return;
  const usize N = P.size();
  if (isNameTooLong(N)) {
    // TODO: Output a warning.
    return;
  }
  // Get the old end(). We will copy from here.
  wchar_t* const old_end = path.growUninit(N);
  inline_memcpy(old_end, P.data(), N * sizeof(wchar_t));
}

void PathNormalizer::push(ImmPtrRange<char> P) {
  if (P.isEmpty())
    return;
  auto S = StrRef(P).dropNull();
  const usize N = S.size();
  if (isNameTooLong(N)) {
    // TODO: Output a warning.
    return;
  }

  // Similar to the wide version, except here we need to widen.
  wchar_t* const old_end = path.growUninit(N);
  for (usize Ix = 0; Ix < N; ++Ix)
    old_end[Ix] = static_cast<wchar_t>(S[Ix]);
}
