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
#include <Sys/Win/Console.hpp>
#include <Sys/Win/Filesystem.hpp>
#include <Sys/Win/Process.hpp>
#include <Sys/Win/Volume.hpp>

#include <BinaryFormat/COFF.hpp>
#include <Common/Flags.hpp>
#include <Common/InlineMemcpy.hpp>
#include <Common/MMatch.hpp>
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
  union {
    u32 Flags = 0;
    struct {
      __prefer_type(bool) u32 HasInp: 1;	
      __prefer_type(bool) u32 HasOut: 1;	
      __prefer_type(bool) u32 HasErr: 1;	
      u32 _BitPadding: 29;	
    };
  };
  u32 SomeData;	
  GenericHandle ConnectionHandle;	
  ConsoleHandle ConsoleHandle;	
  IOFile  InpHandle;	
  IOFile  OutHandle;	
  IOFile  ErrHandle;	
  bool    IsValidHandle;	
  u8      _Padding[7];	
};

static_assert(sizeof(Coords) == 4);
static_assert(sizeof(CSRConsoleStartInfo) == 0x53c);
static_assert(sizeof(CSRConnectionState) == 0x38);

//////////////////////////////////////////////////////////////////////////
// Globals

__imut Win64LDRDataTableEntry* exeEntry = nullptr;
__imut pcl::StaticVec<wchar_t, RT_MAX_PATH> exeName {};
__imut CSRConnectionState connectionState {};

} // namespace `anonymous`

constinit bool XCRT_NAMESPACE::isConsoleApp = false;

static bool InitConsoleFlag() {
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

static void InitExeName() {
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

/// Proxy: `ConsoleIsCallerInLowbox`.
/// Signature: `NtStatus(BOOLEAN*)`
static NtStatus IsCallerInLowbox(bool& out) {
  TokenHandle handle = ThreadEffectiveToken;
  ULong token_class  = /*TokenIsAppContainer*/ 0x1D;
  ULong return_buf  = 0;
  ULong return_size = 0;
  
  NtStatus status = sys::isyscall<
   Syscall::QueryInformationToken>(
    handle, token_class,
    &return_buf, sizeof(return_buf),
    &return_size
  );

  if ($NtSuccess(status)) {
    out = !!return_buf;
    status = 0;
  }

  return status;
}

/// Proxy: `ConsoleCloseIfConsoleHandle`.
/// Signature: `NtStatus(HANDLE*)`
static NtStatus CloseIfConsoleHandle(IOFile& handle) {
  auto tmp_handle = ConsoleHandle::New(handle);
  IoStatusBlock IO {};
  auto info = sys::query_volume_info<FSDeviceInfo>(tmp_handle, IO);

  if ($NtFail(IO.status))
    return IO.status;
  if (info->device_type != DeviceType::Console)
    return IO.status;
  
  const NtStatus ret
    = sys::close_file(tmp_handle);
  handle = nullptr;
  return ret;
}

/// Proxy: `ConsoleCommitState`.
/// Signature: `NtStatus(CONSOLE_STATE*)`
static NtStatus CommitState(CSRConnectionState& state) {
  auto* PP = HcCurrentPEB()->process_params;
  IoStatusBlock io {};
  // Unknown masked flags
  com::bitmask<9>(PP->flags);
  com::bitmask<10>(PP->flags);

  uptr out_handle = 0;
  const auto con_handle
    = ConsoleHandle(state.ConnectionHandle);
  PP->console_handle = ptr_cast<__void>(con_handle.get());

  if (con_handle) {
    constexpr auto code = CtlCode::New(
      DeviceType::Console, 0x8,
      CtlMethod::Neither, AccessMask::Any
    );
    (void) sys::control_device_file(
      con_handle, io, code,
      AddrRange::New(),
      AddrRange::New(ptr_cast<>(&out_handle), 8)
    );
  }

  connectionState = state;
  if (state.Flags & 0b111) {
    if (state.HasInp)
      PP->std_in = state.InpHandle;
    if (state.HasOut)
      PP->std_out = state.OutHandle;
    if (state.HasErr)
      PP->std_err = state.ErrHandle;
  }

  out_handle |= 1U; // No idea why we do this...
  sys::set_process<ProcInfo::ConsoleHostProcess>(out_handle);
  // TODO:
  // InitializeCtrlHandling();
  // NVar1 = SetTEBLangID();
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// Top-level

static bool SetupCUIApp() {
  auto* PP = HcCurrentPEB()->process_params;
  auto console = ConsoleHandle::New(PP->console_handle);
  const MMatch M(console);

  if (PP->console_flags & 0x4) {
    CloseIfConsoleHandle(PP->std_in);
    CloseIfConsoleHandle(PP->std_in);
    CloseIfConsoleHandle(PP->std_err);
  }

  CSRConnectionState state {};
  if (!console || M.is(CreateNewConsole, CreateNoWindow)) {
    // TODO: ConsoleAllocate(state);
  } else {
    // TODO: CreateConnectionObject
    NtStatus status = 0;
    $scope {
      bool in_lowbox = false
      if (status != /*STATUS_ACCESS_DENIED*/ 0xC0000022)
        break;
      if ($NtFail(IsCallerInLowbox(in_lowbox)))
        break;
      if (!in_lowbox)
        break;
      // sys::close_file(state.ConsoleHandle);
      // TODO: ALLOC_CONSOLE
    }
  }

  // CommitState(state);

  return false;
}

static bool SetupGUIApp() {
  auto* PP = HcCurrentPEB()->process_params;
  PP->console_handle = nullptr;

  if (!com::has_flagval<CF_HasInp>(PP->flags))
    CloseIfConsoleHandle(PP->std_in);
  if (!com::has_flagval<CF_HasOut>(PP->flags))
    CloseIfConsoleHandle(PP->std_out);
  NtStatus R = CloseIfConsoleHandle(PP->std_err);
  
  return $NtSuccess(R);
}

bool XCRT_NAMESPACE::setup_console() {
  exeEntry = Win64LoadOrderList::GetExecutableEntry();
  InitConsoleFlag();
  InitExeName();
  if (!isConsoleApp)
    return SetupGUIApp();
  return SetupCUIApp();
}
