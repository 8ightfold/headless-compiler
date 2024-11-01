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
#include <String/Utils.hpp>

#include <Bootstrap/_NtModule.hpp>
#include <Bootstrap/Win64KernelDefs.hpp>
#include <Bootstrap/KUserSharedData.hpp>
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

#define PACKING_TEST(obj, offset, member) \
 static_assert($offsetof(member, obj) == offset, \
  #obj "::" #member " is offset incorrectly. Please report this.")

#define STRT_TEST(offset, member) \
  PACKING_TEST(CSRConsoleStartInfo, offset, member)
#define CONN_TEST(offset, member) \
  PACKING_TEST(CSRConnectionObject, offset, member)

using namespace hc;
using namespace hc::bootstrap;
using namespace hc::sys::win;

namespace {

enum ConsoleFlags : u32 {
  CF_HasInp = 0x200,
  CF_HasOut = 0x400,
};

enum StartFlags : u32 {
  SF_UseShowWindow    = 0x1,
  SF_UseSize          = 0x2,
  SF_UsePosition      = 0x4,
  SF_UseCountChars    = 0x8,
  SF_UseFillAttribute = 0x10,
  SF_RunFullscreen    = 0x20,
  SF_ForceOnFeedback  = 0x40,
  SF_ForceOffFeedback = 0x80,
  SF_UseStdHandles    = 0x100,
  // Winver 0x400
  SF_UseHotkey        = 0x200,
  SF_ShellPrivate     = 0x400,
  SF_TitleIsLinkName  = 0x800,
  SF_TitleIsAppID     = 0x1000,
  SF_PreventPinning   = 0x2000,
};

//////////////////////////////////////////////////////////////////////////
// Objects

struct Coords /*TODO*/ {
  u16 X = 0;
  u16 Y = 0;
};

struct CSRConsoleParams {

};

struct __packed_align(4) CSRConsoleStartInfo {
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
      __prefer_type(bool) u32 HasInp : 1;
      __prefer_type(bool) u32 HasOut : 1;
      __prefer_type(bool) u32 HasErr : 1;
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

struct __packed_align(4) CSRConnectionObject {
  static constexpr usize ExtraSize(usize N) {
    return N * sizeof(char);
  }
public:
  ULong               Size;
  ULong               ID;
  char                ServerName[7];
  CSRConsoleStartInfo ConsoleData;
  u8                  Head;
  ULong               Body;
  u8                  Tail;
  u8                  NameSize;
  UShort              BufferSize;
  char                Extra[8];
};

STRT_TEST(0x020, IsConsoleApplication);
STRT_TEST(0x022, TitleSize);
STRT_TEST(0x024, Title);
STRT_TEST(0x22e, BaseDllNameSize);
STRT_TEST(0x230, BaseDllName);
STRT_TEST(0x330, DosPathSize);
STRT_TEST(0x332, DosPath);

CONN_TEST(0x8,   ServerName);
CONN_TEST(0xf,   ConsoleData);
CONN_TEST(0x54b, Head);
CONN_TEST(0x551, NameSize);
CONN_TEST(0x554, Extra);

static_assert(sizeof(Coords) == 4);
static_assert(sizeof(CSRConsoleStartInfo) == 0x53c);
static_assert(sizeof(CSRConnectionState)  == 0x38);
static_assert(sizeof(CSRConnectionObject) == 0x55c);

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

/// Proxy: `InitExeName`
/// Signature: `void(void)`
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

static inline int ParseShellInfo(
 UnicodeString& info, const wchar_t* keyword) {
  __hc_invariant(keyword != nullptr);
  const usize len = xcrt::wstringlen(keyword);
  wchar_t* off = xcrt::wfind_first_str(
    info.buffer, keyword, info.getSize());

  // IconVal = (SIZE_T)wcsstr(ShellInfoBuf,L"dde.");
  // if ((wchar_t *)IconVal != NULL) {
  //   Icon_Str.Buffer = (PWSTR)(IconVal + 8);
  //   for (ShellInfoBuf = Icon_Str.Buffer; (0x2f < (ushort)*ShellInfoBuf && ((ushort)*ShellInfoBuf < 0x3a) );
  //       ShellInfoBuf = ShellInfoBuf + 1) {
  //   }
  //   Icon_Str.Length = (short)ShellInfoBuf - (short)Icon_Str.Buffer;
  //   Icon_Str.MaximumLength = Icon_Str.Length;
  //   RtlUnicodeStringToInteger(&Icon_Str,0,&DDE_Val);
  //   IconVal = (SIZE_T)DDE_Val;
  // }

  if (off == nullptr)
    // TODO: Uhh? Check on this idk...
    return reinterpret_cast<uptr>(off);

  UnicodeString icon { .buffer = off };
  
  return int_cast<int>(0);
}

/// Proxy: `ConsoleInitializeServerData`
/// Signature: `NtStatus(CONSOLE_SERVER_DATA*)`
static NtStatus InitializeServerData(CSRConsoleStartInfo& info) {
  auto* PP = HcCurrentPEB()->process_params;
  const auto con_handle
    = ConsoleHandle::New(PP->console_handle);
  
  info.IsConsoleApplication = XCRT_NAMESPACE::isConsoleApp;
  info.CreateNoWindow = (con_handle == CreateNoWindow);
  info.WindowFlags = PP->flags;

  if (PP->group_id != 0) {
    info.ProcessGroupId = PP->group_id;
  } else {
    $unreachable_msg("Shit!");
    // info.ProcessGroupId = ClientId.UniqueProcess;
  }

  UnicodeString shell_info = PP->shell_info;
  if (shell_info.buffer != nullptr) {
    int hotkey = 0;
    // IconVal = (SIZE_T)wcsstr(ShellInfoBuf,L"dde.");
    // if ((wchar_t *)IconVal != NULL) {
    //   Icon_Str.Buffer = (PWSTR)(IconVal + 8);
    //   for (ShellInfoBuf = Icon_Str.Buffer; (0x2f < (ushort)*ShellInfoBuf && ((ushort)*ShellInfoBuf < 0x3a) );
    //       ShellInfoBuf = ShellInfoBuf + 1) {
    //   }
    //   Icon_Str.Length = (short)ShellInfoBuf - (short)Icon_Str.Buffer;
    //   Icon_Str.MaximumLength = Icon_Str.Length;
    //   RtlUnicodeStringToInteger(&Icon_Str,0,&DDE_Val);
    //   IconVal = (SIZE_T)DDE_Val;
    // }
    // pData->IconIndex = (int)IconVal;
    // 
    // if (!has_flagval<SF_UseHotkey>(PP->flags)) {
    //   HotKeyVal = wcsstr((PParams->ShellInfo).Buffer,L"hotkey.");
    //   if (HotKeyVal != NULL) {
    //     Hotkey_Str.Buffer = HotKeyVal + 7;
    //     for (ShellInfoBuf = Hotkey_Str.Buffer;
    //       (*ShellInfoBuf > L'/') && (*ShellInfoBuf < L':'); ShellInfoBuf += 1)
    //     {
    //     }
    //     Hotkey_Str.Length = (short)ShellInfoBuf - (short)Hotkey_Str.Buffer;
    //     Hotkey_Str.MaximumLength = Hotkey_Str.Length;
    //     RtlUnicodeStringToInteger(&Hotkey_Str,0,&Hotkey_Val);
    //     HotKeyVal = (wchar_t *)(ulonglong)Hotkey_Val;
    //   }
    // } else {
    //   hotkey = (wchar_t *)(ulonglong)*(uint *)&PParams->hStdInput;
    // }
    info.HotKey = hotkey;
  }

  return 0;
}

/// Proxy: `ConsoleIsCallerInLowbox`
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

/// Proxy: `ConsoleCloseIfConsoleHandle`
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

/// Proxy: `ConsoleCreateConnectionObject`
/// Signature: `NtStatus(HANDLE*, HANDLE, char*, void* buf, u16 buflen)
static NtStatus CreateConnectionObject(
 FileObjRef out, ConsoleHandle console,
 StrRef name, AddrRange buf
) {
  CSRConsoleStartInfo info {};


  // __array_memcpy(obj->ServerName, "server");

  return 0;
}

/// Proxy: `ConsoleAttatchOrAllocate`
/// Signature: `NtStatus(CONNECTION_STATE*, wchar_t*)`
static NtStatus AttatchOrAllocate(CSRConnectionState& state, wchar_t* str) {

  return 0;
}

/// Proxy: `ConsoleAllocate`
/// Signature: `NtStatus(CONNECTION_STATE*)`
static NtStatus AllocateConsole(CSRConnectionState& state) {
  NtStatus status = 0;
  if (!KUSER_SHARED_DATA.Dbg.ConsoleBrokerEnabled) {

  }

  return status;
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
  } else $scope {
    // TODO: CreateConnectionObject
    NtStatus status = 0;
    
    bool in_lowbox = false;
    if (status != /*STATUS_ACCESS_DENIED*/ 0xC0000022)
      break;
    if ($NtFail(IsCallerInLowbox(in_lowbox)))
      break;
    if (!in_lowbox)
      break;
    // sys::close_file(state.ConsoleHandle);
    // TODO: ALLOC_CONSOLE
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
