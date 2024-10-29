//===- Phase1/ConsoleSetup.hpp --------------------------------------===//
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

#include <Phase1/ConsoleSetup.hpp>
#include <Memory/Box.hpp>

#include <Bootstrap/_NtModule.hpp>
#include <Bootstrap/Win64KernelDefs.hpp>
#include <Sys/Win/Filesystem.hpp>
#include <Sys/Win/Volume.hpp>

#include <BinaryFormat/COFF.hpp>
#include <Common/InlineMemcpy.hpp>
#include <Parcel/StaticVec.hpp>
#include <Meta/Unwrap.hpp>

using namespace hc;
using namespace hc::bootstrap;
using namespace hc::sys::win;

namespace {

enum ConsoleFlags : u32 {
  CF_HasInp = 0x200,
  CF_HasOut = 0x400,
};

struct Coords {
  u16 X = 0;
  u16 Y = 0;
};

struct CSRConsoleParams {

};

struct CSRConsoleStartInfo {
  int       IconIndex;
  int       HotKey;
  u32       WindowFlags;
  u16       FillAttribute;
  u16       ShowWindow;
  Coords    ScreenBufferSize;
  Coords    WindowSize;
  Coords    WindowOrigin;
  u32       ProcessGroupId;
  bool      IsConsoleApplication;
  bool      CreateNoWindow;
  u16       TitleSize;
  wchar_t   Title[261];
  u16       BaseDllNameSize;
  wchar_t   BaseDllName[128];
  u16       DosPathSize;
  wchar_t   DosPath[261];
};

struct CSRConnectionState {

};

static_assert(sizeof(Coords) == 4);
static_assert(sizeof(CSRConsoleStartInfo) == 0x53c);

//////////////////////////////////////////////////////////////////////////
// Globals

__imut Win64LDRDataTableEntry* exeEntry = nullptr;
__imut pcl::StaticVec<wchar_t, RT_MAX_PATH> exeName {};

} // namespace `anonymous`

constinit bool XCRT_NAMESPACE::isConsoleApp = false;

static bool InitializeConsoleFlag() {
  using namespace binfmt;
  auto O = __NtModule()->getHeader().win;

  if (!O.isEmpty()) {
    using enum COFF::WindowsSubsystemType;
    const auto subsystem = O.$extract_member(subsystem);
    const bool is_console = (subsystem != eSubsystemWindowsCUI);
    return (XCRT_NAMESPACE::isConsoleApp = is_console);
  }

  return (XCRT_NAMESPACE::isConsoleApp = false);
}

static void InitializeExeName() {
  if (!exeName.isEmpty())
    return;
  const auto name = exeEntry->base_dll_name;

  if (name.getSize() < exeName.Capacity())
    exeName.resizeUninit(name.getSize());
  else
    exeName.resizeUninit(RT_MAX_PATH - 1);
  
  com::inline_memcpy(exeName.data(), name.buffer, name.__size);
  exeName.push(L'\0');
}

static bool InitializeServerData(CSRConsoleStartInfo* P) {
  if (P == nullptr)
    return false;
  

  return true;
}

static NtStatus CloseIfConsoleHandle(IOFile& handle) {
  auto tmp_handle = ConsoleHandle::New(handle);
  IoStatusBlock IO {};
  auto info = sys::query_volume_info<FSDeviceInfo>(
    tmp_handle, IO
  );

  if ($NtFail(IO.status))
    return IO.status;
  if (info->device_type != DeviceType::Console)
    return IO.status;
  
  const NtStatus ret
    = sys::close_file(tmp_handle);
  handle = nullptr;
  return ret;
}

//////////////////////////////////////////////////////////////////////////
// Top-level

static bool SetupCUIApp() {
  CSRConnectionState state {};
  auto* PP = HcCurrentPEB()->process_params;

  if ((PP->console_flags & 0x4) != 0) {
    CloseIfConsoleHandle(PP->std_in);
    CloseIfConsoleHandle(PP->std_in);
    CloseIfConsoleHandle(PP->std_err);
  }

  auto con = PP->console_handle;
  if (con == nullptr) {
    // TODO: ConsoleAllocate(state);
  }

  return false;
}

static bool SetupGUIApp() {
  auto* PP = HcCurrentPEB()->process_params;
  PP->console_handle = nullptr;

  if ((PP->flags & CF_HasInp) == 0)
    CloseIfConsoleHandle(PP->std_in);
  if ((PP->flags & CF_HasOut) == 0)
    CloseIfConsoleHandle(PP->std_out);
  NtStatus R = CloseIfConsoleHandle(PP->std_err);
  
  return $NtSuccess(R);
}

bool XCRT_NAMESPACE::setup_console() {
  exeEntry = Win64LoadOrderList::GetExecutableEntry();
  InitializeConsoleFlag();
  InitializeExeName();
  if (!isConsoleApp)
    return SetupGUIApp();
  return SetupCUIApp();
}
