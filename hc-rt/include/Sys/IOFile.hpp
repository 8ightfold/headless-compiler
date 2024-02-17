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

namespace hc {
  namespace sys { 
    struct IIOFile;
    // extern constinit IIOFile* pout;
    // extern constinit IIOFile* perr;
    // extern constinit IIOFile* pin;
  } // namespace sys
  using RawIOFile = sys::IIOFile;
  using IOFile    = RawIOFile*;
  // using sys::pout;
  // using sys::perr;
  // using sys::pin;
} // namespace hc

#ifdef  __HC_INTERNAL__
#ifndef _HC_IOFILE_STNL
// honestly wanted to fuck with include_next
# define _HC_IOFILE_STNL
# include_next <Sys/IOFile.hpp>
#endif // !_HC_IOFILE_STNL
#endif // __HC_INTERNAL__
