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
#include <Bootstrap/StringMerger.hpp>
#include <Bootstrap/Win64KernelDefs.hpp>
#include <Bootstrap/KUserSharedData.hpp>
#include <Sys/Win/Console.hpp>
#include <Sys/Win/Filesystem.hpp>
#include <Sys/Win/Object.hpp>
#include <Sys/Win/Process.hpp>
#include <Sys/Win/Volume.hpp>

#include <BinaryFormat/COFF.hpp>
#include <Common/DefaultFuncPtr.hpp>
#include <Common/Flags.hpp>
#include <Common/Function.hpp>
#include <Common/InlineMemcpy.hpp>
#include <Common/Location.hpp>
#include <Common/MMatch.hpp>
#include <Meta/Intrusive.hpp>
#include <Meta/Unwrap.hpp>
#include <Parcel/StaticVec.hpp>

#define PACKING_TEST(obj, offset, member) \
 static_assert($offsetof(member, obj) == offset, \
  #obj "::" #member " is offset incorrectly. Please report this.")

#define SERV_TEST(offset, member) \
  PACKING_TEST(CSRConsoleServerInfo, offset, member)
#define CONN_TEST(offset, member) \
  PACKING_TEST(CSRConnectionObject, offset, member)

#define $load_symbol(name) \
  succeeded &= load_nt_symbol(name, #name)

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

struct __packed_align(4) CSRConsoleServerInfo {
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
  DeviceHandle ConnectionHandle;
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
  ULong               ExtraOffset;
  ULong               ID;
  char                ServerName[7];
  CSRConsoleServerInfo ConsoleData;
  u8                  Head;
  ULong               Body;
  u8                  Tail;
  u8                  NameSize;
  UShort              BufferSize;
  char                Extra[];
};

SERV_TEST(0x020, IsConsoleApplication);
SERV_TEST(0x022, TitleSize);
SERV_TEST(0x024, Title);
SERV_TEST(0x22e, BaseDllNameSize);
SERV_TEST(0x230, BaseDllName);
SERV_TEST(0x330, DosPathSize);
SERV_TEST(0x332, DosPath);

CONN_TEST(0x8,   ServerName);
CONN_TEST(0xf,   ConsoleData);
CONN_TEST(0x54b, Head);
CONN_TEST(0x551, NameSize);
CONN_TEST(0x554, Extra);

static_assert(sizeof(Coords) == 4);
static_assert(sizeof(CSRConsoleServerInfo) == 0x53c);
static_assert(sizeof(CSRConnectionState)  == 0x38);
static_assert(sizeof(CSRConnectionObject) == 0x554);

//////////////////////////////////////////////////////////////////////////
// Globals

__imut Win64LDRDataTableEntry* exeEntry = nullptr;
__imut pcl::StaticVec<wchar_t, RT_MAX_PATH> exeName {};
__imut CSRConnectionState connectionState {};

/// Signature of `RtlCreateProcessParametersEx`.
using CreatePPType = NtStatus(
  Win64ProcParams** PP,
  UnicodeString* image, UnicodeString* dll, UnicodeString* curr_dir,
  UnicodeString* cmdline, void* env, UnicodeString* wnd_title,
  UnicodeString* desktop_info, UnicodeString* shell_info,
  UnicodeString* rt_data, ULong flags
);

__imut DefaultFuncPtr<ULong(const char*, ...)> DbgPrint {}; // TODO: Replace
__imut DefaultFuncPtr<CreatePPType> RtlCreateProcessParametersEx {};
__imut DefaultFuncPtr<NtStatus(Win64ProcParams*)> RtlDestroyProcessParameters {};

} // namespace `anonymous`

constinit bool XCRT_NAMESPACE::isConsoleApp = false;
constinit bool XCRT_NAMESPACE::isConsoleSetUp = false;

//////////////////////////////////////////////////////////////////////////
// Logging

#if _HC_DEBUG && 0
# define dbg_if(expr...) if (expr...)
# define DBG_LOG(fmt, args...) do { \
  if (0) CheckLogArgs(fmt "\n", ##args); \
  DbgLog(SourceLocation::Current(), fmt "\n", ##args); \
} while(0)
# define DBG_DUMP(expr...) DbgDump(expr)
#else
# define dbg_if(expr...) if constexpr (false)
# define DBG_LOG(fmt, ...) (void(0))
# define DBG_DUMP(...) (void(0))
#endif

using CStr = const char[];

__attribute__((__format__(__printf__, 1, 2)))
static __always_inline void CheckLogArgs(
  [[maybe_unused]] const char fmt[], ...) {
}

static void DbgLog(SourceLocation loc, const char* __nonnull fmt, auto...args) {
  if __expect_false(!DbgPrint)
    return;
  // DbgPrint("[\"%s\", %i:%i] ", loc.__func, loc.__line, loc.__column);
  DbgPrint("At %s:%i:%i:\n  In '%s': ",
    loc.__file, loc.__line, loc.__column, loc.__func);
  DbgPrint(fmt, args...);
}

static void DbgPrintC(CStr x, CStr fmt, auto...args) {
  if __expect_false(!DbgPrint)
    return;
  DbgPrint("%s", x);
  DbgPrint(fmt, args...);
}

static void DbgDumpImpl(CStr x, CStr fmt) {
  if __expect_false(!DbgPrint)
    return;
  if (StrRef(fmt).beginsWith(" {"))
    DbgPrint(fmt);
  else
    DbgPrintC(x, fmt);
}

static void DbgDumpImpl(CStr x, CStr fmt, auto...args) {
  DbgPrintC(x, fmt, args...);
}

template <typename T>
static void DbgDumpImpl(CStr x, CStr fmt, CStr pad, CStr ty, CStr id, T* V) {
  if constexpr (meta::is_struct<T>) {
    if (V) {
      usize len = xcrt::stringlen(x) + xcrt::stringlen(pad);
      auto ls = $zdynalloc(len + 1, char);
      inline_memset(ls.data(), ' ', len);
      __builtin_dump_struct(V, DbgDumpImpl, ls.data());
      return;
    }
  }
  DbgPrintC(x, fmt, pad, ty, id, V);
}

template <typename E>
requires meta::is_enum<E>
static void DbgDumpImpl(CStr x, CStr fmt, CStr pad, CStr ty, CStr id, E e) {
  // "%s%s %s = %?\n"
  DbgPrintC(x, "%s%s %s = 0x%X\n", pad, ty, id, e);
}

static void DbgDump(auto& obj) {
  __builtin_dump_struct(&obj, DbgDumpImpl, "");
}

static void DbgDump(auto* obj) {
  if (!obj) {
    DBG_LOG("Passed nullptr!");
    return;
  }
  __builtin_dump_struct(obj, DbgDumpImpl, "");
}

//////////////////////////////////////////////////////////////////////////
// Utils

template <typename F>
static bool load_nt_symbol(
 DefaultFuncPtr<F>& func, StrRef symbol) {
  if __expect_true(func.isSet())
    return true;
  auto exp = __NtModule()->resolveExport<F>(symbol);
  return func.setSafe($unwrap(exp));
}

static bool InitRtlFuncs() {
  static bool has_succeeded = false;
  if __expect_false(!has_succeeded) {
    bool succeeded = true;
    $load_symbol(DbgPrint);
    $load_symbol(RtlCreateProcessParametersEx);
    $load_symbol(RtlDestroyProcessParameters);
    has_succeeded = succeeded;
  }
  return has_succeeded;
}

static inline constexpr AccessMask CreateConsoleAccess() {
  using enum AccessMask;
  return Sync | ReadControl | WriteAttributes | ReadAttributes
    | WriteEA | ReadEA | AppendData | WriteData | ReadData;
}

static bool InitConsoleFlag() {
  using namespace binfmt;
  auto O = __NtModule()->getHeader().win;

  if (!O.isEmpty()) {
    using enum COFF::WindowsSubsystemType;
    const auto subsystem = O.$extract_member(subsystem);
    const bool is_console = (subsystem == eSubsystemWindowsCUI);
    return (XCRT_NAMESPACE::isConsoleApp = is_console);
  }

  return (XCRT_NAMESPACE::isConsoleApp = false);
}

/// Proxy: `InitExeName`
/// Signature: `void(void)`
static void InitExeName() {
  if (!exeName.isEmpty())
    return;
  __hc_invariant(exeEntry != nullptr);
  const auto name = exeEntry->base_dll_name;

  if (name.getSize() < exeName.Capacity())
    exeName.resizeUninit(name.getSize());
  else
    exeName.resizeUninit(RT_MAX_PATH - 1);
  
  com::inline_memcpy(exeName.data(), name.buffer, name.__size);
  exeName.push(L'\0');
}

static inline int ParseShellInfo(
 UnicodeString info, const wchar_t* keyword) {
  __hc_invariant(keyword != nullptr);
  const usize len = xcrt::wstringlen(keyword);

  wchar_t* off = xcrt::wfind_first_str(
    info.buffer, keyword, info.getSize());
  if (off == nullptr)
    // TODO: Uhh? Check on this idk...
    return reinterpret_cast<uptr>(off);

  off += len;
  UnicodeString icon { .buffer = off };
  while (MMatch(*off).in(L'/' + 1, L':')) {
    ++off;
  }

  icon.__size = (off - icon.buffer) * sizeof(wchar_t);
  icon.__size_max = icon.__size;
  
  ULong out = 0;
  (void) boot::UnicodeStringToInteger(icon, out);
  return out;
}

/// Proxy: `ConsoleCopyStringToBuffer`
/// Signature: `NtStatus(USHORT*, WCHAR*, USHORT, UNICODE_STRING*)`
template <usize BufSize>
static inline usize CopyStringToBuffer(
 wchar_t(&buf)[BufSize], u16& buf_size, UnicodeString S) {
  usize len = S.getSize();
  if (BufSize - 1 <= S.getSize()) {
    len = BufSize - 1;
  }
  com::inline_memcpy(buf, S.buffer, len * sizeof(wchar_t));
  buf[len] = L'\0';
  buf_size = (len * sizeof(wchar_t));
  return len;
}

/// Proxy: `RtlGetCurrentServiceSessionId`
/// Signature: `ULong*(void)`
static ULong* GetCurrentServiceSessionId() {
  auto* data = ptr_cast<ULong>(HcCurrentPEB()->shared_data);
  if (data == nullptr)
    return data;
  return ptr_cast<ULong>(*data);
}

/// Proxy: `RtlGetNtSystemRoot`
/// Signature: `wchar_t*(void)`
static UnicodeString GetNtSystemRoot() {
  auto* session_id = GetCurrentServiceSessionId();
  if (session_id == nullptr) {
    auto& root = KUSER_SHARED_DATA.NtSystemRoot;
    return UnicodeString::New(root, meta::__get_size(root));
  }

  auto* data = ptr_cast<u8>(HcCurrentPEB()->shared_data);
  auto* root = ptr_cast<wchar_t>(data + 0x1e);
  return UnicodeString::New(root);
}

/// Proxy: `ConsoleIsCallerInLowbox`
/// Signature: `NtStatus(BOOLEAN*)`
static NtStatus IsCallerInLowbox(bool& out) {
  TokenHandle handle = ThreadEffectiveToken;
  ULong token_class = /*TokenIsAppContainer*/ 0x1D;
  ULong return_buf  = 0;
  ULong return_size = 0;
  constexpr ULong buf_size = sizeof(return_buf);
  
  NtStatus status = sys::isyscall<
   Syscall::QueryInformationToken>(
    handle, token_class,
    &return_buf, buf_size,
    &return_size
  );

  if ($NtSuccess(status)) {
    out = !!return_buf;
    status = 0;
  }

  return status;
}

static NtStatus SetSystemConsoleInfo(bool is_driver_loaded) {
  ULong system_class = /*SystemConsoleInformation*/ 0x84;
  ULong console_info = /*SYSTEM_CONSOLE_INFORMATION*/ is_driver_loaded;
  constexpr ULong info_size = sizeof(console_info);

  return sys::isyscall<Syscall::SetSystemInformation>(
    system_class, &console_info, info_size
  );
}

/// Proxy: `ConsoleLaunchServerProcess`
/// Signature: `NtStatus(UNICODE_STRING*, UNICODE_STRING*, HANDLE)`
static NtStatus LaunchServerProcess(
 UnicodeString& image, UnicodeString& cmdline, DeviceHandle server) {
  if __expect_false(!InitRtlFuncs()) {
    // STATUS_UNSUCCESSFUL
    return 0xC0000001;
  }

  NtStatus status = 0;
  Win64ProcParams* PP_new = nullptr;
  Win64ProcParams* PP = HcCurrentPEB()->process_params;
  IoStatusBlock io {};

  status = RtlCreateProcessParametersEx(
    &PP_new, &image, nullptr, nullptr,
    &cmdline, nullptr, nullptr,
    &PP->desktop, nullptr, nullptr,
    /*RTL_USER_PROC_PARAMS_NORMALIZED*/ 0x1
  );
  if (!PP_new || $NtFail(status)) {
    DBG_LOG("Failed to create ProcessParameters: 0x%X.", status);
    return status;
  }
  
  constexpr auto code = CtlCode::New(
    DeviceType::Console, 0xd,
    CtlMethod::Neither, AccessMask::Any
  );
  status = sys::control_device_file(
    server, io, code,
    AddrRange::New<void>(PP_new, PP_new->size),
    AddrRange::New()
  );

  RtlDestroyProcessParameters(PP_new);
  return status;
}

static ConsoleHandle GetProcessConsoleHandle() {
  auto* PP = HcCurrentPEB()->process_params;
  return ConsoleHandle::New(PP->console_handle);
}

//////////////////////////////////////////////////////////////////////////
// Implementation

/// Proxy: `ConsoleInitializeServerData`
/// Signature: `NtStatus(CONSOLE_SERVER_DATA*)`
static NtStatus InitializeServerData(CSRConsoleServerInfo& info) {
  // For some checks down the line.
  __hc_invariant(exeEntry != nullptr);

  auto* PP = HcCurrentPEB()->process_params;
  const auto con_handle
    = ConsoleHandle::New(PP->console_handle);
  
  info.IsConsoleApplication = XCRT_NAMESPACE::isConsoleApp;
  info.CreateNoWindow = (con_handle == CreateNoWindow);

  const auto flags = PP->window_flags;
  info.WindowFlags = flags;

  if (PP->group_id != 0) {
    info.ProcessGroupId = PP->group_id;
  } else {
    $unreachable_msg("Shit!");
    DBG_LOG("Failed to set ProcessGroupId.");
    // info.ProcessGroupId = ClientId.UniqueProcess;
  }

  if (has_flagval<SF_UseShowWindow>(flags)) {
    info.ShowWindow = PP->show_window;
  }
  if (has_flagval<SF_UseSize>(flags)) {
    info.WindowSize.X = PP->X_count;
    info.WindowSize.Y = PP->Y_count;
  }
  if (has_flagval<SF_UsePosition>(flags)) {
    info.WindowOrigin.X = PP->X;
    info.WindowOrigin.Y = PP->Y;
  }
  if (has_flagval<SF_UseCountChars>(flags)) {
    info.ScreenBufferSize.X = PP->X_chars;
    info.ScreenBufferSize.Y = PP->Y_chars;
  }
  if (has_flagval<SF_UseFillAttribute>(flags)) {
    info.FillAttribute = PP->fill_attrib;
  }

  // Shell info is required to be null-terminated.
  UnicodeString shell_info = PP->shell_info;
  if (shell_info.buffer != nullptr) {
    info.IconIndex = ParseShellInfo(shell_info, L"dde.");

    if (!has_flagval<SF_UseHotkey>(flags))
      info.HotKey = ParseShellInfo(shell_info, L"hotkey.");
    else
      info.HotKey = uptr(PP->std_in);
  }

  if (PP->window_title.getSize() > 0) {
    CopyStringToBuffer(info.Title, info.TitleSize, PP->window_title);
  } else {
    CopyStringToBuffer(info.Title, info.TitleSize, "Command Prompt"_UStr);
  }

  CopyStringToBuffer(
    info.BaseDllName, info.BaseDllNameSize,
    exeEntry->base_dll_name
  );

  return CopyStringToBuffer(
    info.DosPath, info.DosPathSize,
    PP->curr_dir.dos_path
  );
}

/// Proxy: `ConsoleCloseIfConsoleHandle`
/// Signature: `NtStatus(HANDLE*)`
static NtStatus CloseIfConsoleHandle(IOFile& handle) {
  auto tmp_handle = ConsoleHandle::New(handle);
  IoStatusBlock io {};
  auto info = sys::query_volume_info<FSDeviceInfo>(tmp_handle, io);

  if ($NtFail(io.status)) {
    DBG_LOG("Query failed: 0x%X.", io.status);
    return io.status;
  }
  if (info->device_type != DeviceType::Console)
    return io.status;
  
  const NtStatus ret
    = sys::close_file(tmp_handle);
  handle = nullptr;
  return ret;
}

/// Proxy: `ConsoleCreateConnectionObject`
/// Signature: `NtStatus(HANDLE*, HANDLE, char*, void* buf, u16 buflen)
static NtStatus CreateConnectionObject(
 ServeRef out, ConsoleHandle console,
 StrRef name, AddrRange buf
) {
  CSRConsoleServerInfo info {};
  InitializeServerData(info);

  usize size = $offsetof(Head, CSRConnectionObject);
  usize name_size = 0;
  usize extra_size = 0;

  if (!name.isEmpty()) {
    name_size = name.size() + 1;
    extra_size = name_size + buf.sizeInBytes();
    size = sizeof(CSRConnectionObject) + extra_size;
  }

  using Wrapper = XCRT_NAMESPACE::Box<CSRConnectionObject>;
  auto obj = Wrapper::NewExtra(extra_size);

  if (!obj) {
    // STATUS_NO_MEMORY
    return 0xC0000017;
  }

  com::inline_memset(obj.get(), 0, size);
  obj->ID = 0x53c0600; // ?
  obj->ExtraOffset = 0;
  __array_memcpy(obj->ServerName, "server");
  __clone(obj->ConsoleData, info);

  if (!name.isEmpty()) {
    obj->ExtraOffset = sizeof(CSRConnectionObject) + 2;
    // Fill out padding.
    obj->Head = 0;
    obj->Body = 0;
    obj->Tail = 0;
    // Fill out sizes.
    obj->NameSize = name.size();
    obj->BufferSize = buf.sizeInBytes();
    // Now copy into the buffers.
    com::inline_memcpy(obj->Extra, name.data(), name_size);
    com::inline_memcpy(obj->Extra + name_size, buf.data(), buf.sizeInBytes());
  }

  UnicodeString conname = L"\\Connect"_UStr;
  if (!console)
    conname = L"\\Device\\ConDrv\\Connect"_UStr;
  
  ObjectAttributes attr { .object_name = &conname };
  attr.attributes = ObjAttribMask::CaseInsensitive;
  attr.root_directory = console;

  IoStatusBlock io {};
  auto connection = sys::open_file_ex(
    CreateConsoleAccess(), attr,
    io, nullptr,
    FileAttribMask::None,
    FileShareMask::All,
    CreateDisposition::Create,
    CreateOptsMask::SyncIONoAlert,
    AddrRange::New<void>(obj.get(), size)
  );

  out = DeviceHandle::New(connection.get());
  return io.status;
}

static inline NtStatus CreateEmptyConnectionObject(
 ServeRef out, ConsoleHandle console) {
  return CreateConnectionObject(
    out, console, "", AddrRange::New());
}

/// Proxy: `ConsoleCreateHandle`
/// Signature: `NtStatus(HANDLE*, HANDLE, wchar_t* Where, bool, bool)`
static NtStatus CreateHandle(
 ServeRef out, DeviceHandle device,
 UnicodeString where, bool inherits = true, bool is_synced = true
) {
  ObjectAttributes attr { .object_name = &where };
  attr.attributes = ObjAttribMask::CaseInsensitive;
  if (inherits)
    attr.attributes |= ObjAttribMask::Inherit;
  attr.root_directory = device;

  IoStatusBlock io {};
  auto connection = sys::open_file(
    CreateConsoleAccess(), attr,
    io, nullptr,
    FileAttribMask::None,
    FileShareMask::All,
    CreateDisposition::Create,
    is_synced
      ? CreateOptsMask::SyncIONoAlert
      : CreateOptsMask::_None
  );

  out = ConsoleHandle::New(connection.get());
  if ($NtFail(io.status)) {
    DBG_LOG("Failed to create connection: 0x%X.", io.status);
    return io.status;
  }
  return 0;
}

/// Proxy: `ConsoleCreateStandardIoObjects`
/// Signature: `NtStatus(CONNECTION_STATE*)`
static NtStatus CreateStandardIoObjects(CSRConnectionState& state) {
  NtStatus status = 0;
  ConsoleHandle inp, out, err;
  // Create standard input.
  status = CreateHandle(&inp, state.ConnectionHandle, L"\\Input"_UStr);
  if ($NtFail(status)) {
    DBG_LOG("Failed to create input handle: 0x%X.", status);
    return status;
  }
  // Create standard output.
  status = CreateHandle(&out, state.ConnectionHandle, L"\\Output"_UStr);
  if ($NtFail(status)) {
    DBG_LOG("Failed to create output handle: 0x%X.", status);
    sys::close_file(inp);
    return status;
  }

  status = sys::duplicate_object(
    out, err, AccessMask::Any, ObjAttribMask::None,
    DupOptsMask::SameAccess | DupOptsMask::SameAttributes
  );
  if ($NtFail(status)) {
    DBG_LOG("Failed to create duplicate handle: 0x%X.", status);
    sys::close_file(inp);
    sys::close_file(out);
    return status;
  }

  state.InpHandle = handle_cast_ex<IOFile>(inp);
  state.OutHandle = handle_cast_ex<IOFile>(out);
  state.ErrHandle = handle_cast_ex<IOFile>(err);
  state.Flags = 0b111;

  return status;
}

/// Proxy: `ConsoleSanitizeStandardIoObjects`
/// Signature: `NtStatus(CONNECTION_STATE*)`
static NtStatus SanitizeStandardIoObjects(CSRConnectionState& state) {
  auto* PP = HcCurrentPEB()->process_params;
  NtStatus status = 0;
  ULong flags = 0;

  auto inp = ConsoleHandle::New(nullptr);
  auto out = ConsoleHandle::New(nullptr);
  auto err = ConsoleHandle::New(nullptr);
  
  auto needs_sanitizing = [](IOFile file) -> bool {
    if (file == nullptr)
      return true;
    const uptr mask = uptr(file) & 0x10000003;
    return (mask == 0x3);
  };

  auto close_all = [&]() -> NtStatus {
    DBG_LOG("Failed to sanitize handles: 0x%X.", status);
    if (inp) sys::close_file(inp);
    if (out) sys::close_file(out);
    if (err) sys::close_file(err);
    return status;
  };

  // Create standard input (if required).
  if (needs_sanitizing(PP->std_in)) {
    status = CreateHandle(&inp, state.ConnectionHandle, L"\\Input"_UStr);
    if ($NtFail(status))
      return close_all();
    flags |= 0b001;
  }
  // Create standard output (if required).
  if (needs_sanitizing(PP->std_out)) {
    status = CreateHandle(&out, state.ConnectionHandle, L"\\Output"_UStr);
    if ($NtFail(status))
      return close_all();
    flags |= 0b010;
  }
  // Create standard error, or clone output (if required).
  if (needs_sanitizing(PP->std_err)) {
    if (!out) {
      status = CreateHandle(&err, state.ConnectionHandle, L"\\Output"_UStr);
    } else {
      status = sys::duplicate_object(
        out, err, AccessMask::Any, ObjAttribMask::None,
        DupOptsMask::SameAccess | DupOptsMask::SameAttributes
      );
    }
    if ($NtFail(status))
      return close_all();
    flags |= 0b100;
  }

  if (has_flag<0>(flags))
    // Assign standard in.
    state.InpHandle = handle_cast_ex<IOFile>(inp);
  if (has_flag<1>(flags))
    // Assign standard out.
    state.OutHandle = handle_cast_ex<IOFile>(out);
  if (has_flag<2>(flags))
    // Assign standard err.
    state.ErrHandle = handle_cast_ex<IOFile>(err);
  
  state.Flags = flags;
  return status;
}

/// Proxy: `ConsoleCleanupConnectionState`
/// Signature: `void(CONNECTION_STATE*)`
static void CleanupConnectionState(CSRConnectionState& state) {
  if (state.IsValidHandle) {
    sys::close_file(state.ConnectionHandle);
    sys::close_file(state.ConsoleHandle);
  }

  auto do_close = [](IOFile file) {
    if (!file) return;
    const auto hnd = ConsoleHandle::New(file);
    sys::close_file(hnd);
  };

  if (state.Flags & 0b111) {
    if (state.HasInp)
      do_close(state.InpHandle);
    if (state.HasOut)
      do_close(state.OutHandle);
    if (state.HasErr)
      do_close(state.ErrHandle);
  }
}

static NtStatus CreateIOState(
 CSRConnectionState& state,
 DeviceHandle& device,
 ConsoleHandle* pconsole = nullptr
) {
  NtStatus status = 0;
  auto local_console
    = ConsoleHandle::New(nullptr);
  if (pconsole == nullptr) {
    // Initialize local console handle.
    status = CreateHandle(
      &local_console, device, L"\\Reference"_UStr, false);
    if ($NtFail(status)) {
      DBG_LOG("Failed to create reference handle: 0x%X.", status);
      if (device)
        sys::close_file(device);
      return status;
    }
    pconsole = &local_console;
  }

  auto* PP = HcCurrentPEB()->process_params;
  __zero_memory(state);
  state.IsValidHandle = !!device;
  state.ConnectionHandle = device;
  state.ConsoleHandle = *pconsole;

  if (!has_flagval<SF_UseStdHandles>(PP->window_flags))
    status = CreateStandardIoObjects(state);
  else
    status = SanitizeStandardIoObjects(state);
  
  if ($NtFail(status)) {
    DBG_LOG("Failed to create iostate: 0x%X.", status);
    CleanupConnectionState(state);
  }
  return status;
}

/// Proxy: `ConsoleAttatchOrAllocate`
/// Signature: `NtStatus(CONNECTION_STATE*, wchar_t*)`
static NtStatus AttatchOrAllocate(CSRConnectionState& state, wchar_t* str) {
  static constexpr usize desktopMax = 260;
  auto* PP = HcCurrentPEB()->process_params;
  NtStatus status = 0;
  StrRef name;
  AddrRange buf;
  wchar_t desktop_buf[desktopMax];

  if (str == nullptr) {
    const auto& desktop = PP->desktop;
    usize buf_len = desktopMax - 1;
    buf_len = std::min(buf_len, desktop.getSize());

    com::inline_memcpy(
      desktop_buf, desktop.buffer,
      buf_len * sizeof(wchar_t)
    );
    desktop_buf[buf_len] = L'\0';

    buf = AddrRange::New<void>(
      desktop_buf, sizeof(desktop_buf));
    name = "broker";
  } else {
    buf = AddrRange::New<void>(&str, sizeof(str));
    name = "attach";
  }

  auto device = DeviceHandle::New(nullptr);
  status = CreateConnectionObject(
    &device, ConsoleHandle::New(nullptr),
    name, buf
  );
  if ($NtFail(status)) {
    DBG_LOG("Failed to attach/init console: 0x%X.", status);
    return status;
  }
  
  return CreateIOState(state, device);
}

/// Proxy: `ConsoleAllocate`
/// Signature: `NtStatus(CONNECTION_STATE*)`
static NtStatus AllocateConsole(CSRConnectionState& state) {
  NtStatus status = 0;
  if (KUSER_SHARED_DATA.Dbg.ConsoleBrokerEnabled)
    return status;
  status = AttatchOrAllocate(state, nullptr);
  if (status != /*STATUS_INVALID_HANDLE*/0xC0000008)
    return status;
  
  auto* PP = HcCurrentPEB()->process_params;
  UnicodeString root = GetNtSystemRoot();

  UStringMerger<280> buf {};
  buf.merge(L"\\??\\", root, L"\\system32\\conhost.exe");
  UnicodeString image_path = buf.toUStr();
  buf.merge(L" 0xffffffff -ForceV1");
  UnicodeString cmdline = buf.toUStr();
  
  auto server = DeviceHandle::New(nullptr);
  auto console = ConsoleHandle::New(nullptr);

  const auto where = L"\\Device\\ConDrv\\Server"_UStr;
  status = CreateHandle(
    &server, DeviceHandle::New(nullptr),
    where, true, false
  );
  if ($NtFail(status)) {
    SetSystemConsoleInfo(true);
    status = CreateHandle(
      &server, DeviceHandle::New(nullptr),
      where, true, false
    );
    if ($NtFail(status)) {
      DBG_LOG("Failed to create server handle: 0x%X.", status);
      return status;
    }
  }

  status = CreateHandle(
    &console, server, L"\\Reference"_UStr, false);
  if ($NtFail(status)) {
    DBG_LOG("Failed to create reference handle: 0x%X.", status);
    sys::close_file(server);
    return status;
  }

  status = LaunchServerProcess(image_path, cmdline, server);
  sys::close_file(server);
  if ($NtFail(status)) {
    DBG_LOG("Failed to launch server: 0x%X.", status);
    sys::close_file(console);
    return status;
  }

  auto device = DeviceHandle::New(nullptr);
  status = CreateEmptyConnectionObject(device, console);
  if ($NtFail(status)) {
    DBG_LOG("Failed to create connection: 0x%X.", status);
    sys::close_file(console);
    return status;
  }

  return CreateIOState(state, device, &console);
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
    // TODO: Check on this
    = ConsoleHandle::New(state.ConnectionHandle.get());
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
  } else {
    DBG_LOG("Invalid ConnectionHandle.");
    return /*STATUS_INVALID_HANDLE*/0xC0000008;
  }

  connectionState = state; // Save state.
  if (state.Flags & 0b111) {
    if (state.HasInp)
      PP->std_in  = state.InpHandle;
    if (state.HasOut)
      PP->std_out = state.OutHandle;
    if (state.HasErr)
      PP->std_err = state.ErrHandle;
  }

  out_handle |= 1U; // No idea why we do this...
  sys::set_process<ProcInfo::ConsoleHostProcess>(out_handle);
  // TODO:
  // InitCtrlHandling();
  // status = SetTEBLangID();
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// Top-level

static bool SetupCUIApp() {
  NtStatus status = 0;
  auto* PP = HcCurrentPEB()->process_params;
  auto console = GetProcessConsoleHandle();
  const MMatch M(console);

  if (com::has_flag<2>(PP->console_flags)) {
    CloseIfConsoleHandle(PP->std_in);
    CloseIfConsoleHandle(PP->std_in);
    CloseIfConsoleHandle(PP->std_err);
  }

  CSRConnectionState state {};
  auto alloc_console = [&state] () -> bool {
    NtStatus status = AllocateConsole(state);
    if ($NtFail(status)) {
      DBG_LOG("Failed to allocate console: 0x%X.", status);
      return false;
    }
    status = CommitState(state);
    return $NtSuccess(status);
  };

  if (!console || M.is(CreateNewConsole, CreateNoWindow)) {
    return alloc_console();
  } else if (console == InvalidHandle) {
    status = CommitState(state);
    return $NtSuccess(status);
  }
  
  auto device = DeviceHandle::New(nullptr);
  status = CreateEmptyConnectionObject(device, console);
  if ($NtFail(status)) {
    if (status == /*STATUS_ACCESS_DENIED*/0xC0000022) $scope {
      bool in_lowbox = false;
      status = IsCallerInLowbox(in_lowbox);
      if ($NtFail(status)) break;
      if (!in_lowbox) break;
      sys::close_file(
        GetProcessConsoleHandle());
      return alloc_console();
    }
    DBG_LOG("Failed and not in lowbox: 0x%X.", status);
    return false;
  }

  state.IsValidHandle = !!device;
  state.ConnectionHandle = device;
  state.ConsoleHandle = GetProcessConsoleHandle();

  $scope {
    if (com::has_flag<1>(PP->console_flags)) {
      status = SanitizeStandardIoObjects(state);
      if ($NtSuccess(status))
        break;
    }

    CleanupConnectionState(state);
    return false;
  }

  status = CommitState(state);
  return $NtSuccess(status);
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
  if (!InitRtlFuncs()) {
    DBG_LOG("Failed to initialize Rtl functions.");
    XCRT_NAMESPACE::isConsoleSetUp = false;
    return false;
  }

  XCRT_NAMESPACE::log_console_state();
  InitConsoleFlag();
  // InitCtrlHandling();
  InitExeName();

  bool did_setup = false;
  if (XCRT_NAMESPACE::isConsoleApp)
    did_setup = SetupCUIApp();
  else
    did_setup = SetupGUIApp();
  
  XCRT_NAMESPACE::isConsoleSetUp = did_setup;
  return did_setup;
}

void XCRT_NAMESPACE::log_console_state() {
  if (!DbgPrint)
    return;
  auto* PP = HcCurrentPEB()->process_params;
  DbgPrint("isConsoleSetUp: %s\n",
    XCRT_NAMESPACE::isConsoleSetUp ? "true" : "false");
  DbgPrint("console: 0x%p\n", PP->console_handle);
  DbgPrint("stdin:   0x%p\n", PP->std_in);
  DbgPrint("stdout:  0x%p\n", PP->std_out);
  DbgPrint("stderr:  0x%p\n", PP->std_err);
  DbgPrint("\n");
}
