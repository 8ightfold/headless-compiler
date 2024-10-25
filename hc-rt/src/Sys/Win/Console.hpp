//===- Sys/Win/Console.hpp ------------------------------------------===//
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

#include "Nt/Filesystem.hpp"
#include "Nt/IOControl.hpp"
#include <Common/PtrRange.hpp>
#include <Common/PtrUnion.hpp>

// For more info:
// https://github.com/wine-mirror/wine/blob/master/include/winioctl.h
// https://learn.microsoft.com/en-us/windows/win32/devio/calling-deviceiocontrol
// https://github.com/dlunch/NewConsole

namespace hc::sys {
inline namespace __nt {

enum AssociatedInvocation : win::ULong {
  AI_GetConsoleCP                 = 0x1000000,
  AI_GetConsoleMode               = 0x1000001,
  AI_GetConsoleScreenBufferInfoEx = 0x2000007,
  AI_WriteConsole                 = 0x1000006,
  AI_ReadConsole                  = 0x1000005,
  AI_GetConsoleTitle              = 0x2000014,
  AI_SetConsoleMode               = 0x1000002,
  AI_SetTEBLangID                 = 0x1000008,
  AI_SetConsoleTitle              = 0x2000015,
  AI_SetConsoleCursorPosition     = 0x200000a,
  AI_SetConsoleTextAttribute      = 0x200000d,
  AI_FillConsoleOutput            = 0x2000000,
  AI_GetConsoleWindow             = 0x300001f,
  AI_UnkPowershellCall            = 0x3000004 // remove ??
};

struct CSRMessage {
  win::ULong length = 0;
  win::ULong unk = 0;
  void* buffer = 0;
};

using CSRMessageBuf = com::PtrRange<CSRMessage>;

struct CSRParams {
  template <class Self>
  Self& init(this Self& self, AssociatedInvocation from) {
    self.code = from;
    self.length = (sizeof(Self) - sizeof(CSRParams));
    return self;
  }
public:
  AssociatedInvocation code;
  win::ULong length = 0;
};

static_assert(sizeof(CSRMessage) == 16);
static_assert(sizeof(CSRParams) == 8);

//////////////////////////////////////////////////////////////////////////

__nt_attrs win::NtStatus control_device_file(
 win::FileHandle handle,
 win::IoStatusBlock& io,
 win::CtlCode code,
 com::AddrRange in,
 com::AddrRange out
) {
  return isyscall<NtSyscall::DeviceIoControlFile>(
    $unwrap_handle(handle), win::EventHandle::New(nullptr),
    win::IOAPCRoutinePtr(nullptr), nullptr,
    &io, code.toCode(),
    in.data(), win::ULong(in.size()),
    out.data(), win::ULong(out.size())
  );
}

/// Handles CSR messaging.
inline win::NtStatus write_generic(
 win::FileHandle handle,
 win::GenericHandle input_handle,
 CSRParams& params,
 CSRMessage* msg = nullptr,
 CSRMessageBuf msg_buf = {}
) {
  static constexpr usize maxBufs = 6;
  struct __packed_alignas(u32) Input {
    win::GenericHandle handle;
    win::ULong xcount = 0;
    win::ULong ycount = 0;
    // 2 extra for `CSRParams` info
    CSRMessage msg[maxBufs + 2];
  };

  static_assert(sizeof(Input) ==
    (maxBufs + 3) * sizeof(CSRMessage));

  const bool has_msg = msg;
  const usize total_size = has_msg + msg_buf.size();
  if (total_size > maxBufs)
    // STATUS_NOT_SUPPORTED
    return 0xC00000BB;
  
  __hc_assert(params.code);
  Input in {
    .handle = input_handle,
    .xcount = u32(has_msg) + 1,
    .ycount = u32(msg_buf.size()) + 1
  };

  auto& buf = in.msg;
  buf[0].length = params.length + sizeof(CSRParams);
  buf[0].buffer = &params;

  if (msg) {
    buf[1].length = msg->length;
    buf[1].buffer = msg->buffer;
  }

  buf[in.xcount].length = params.length;
  buf[in.xcount].buffer = &params + 1;

  for (usize Ix = 0; Ix < msg_buf.size(); ++Ix) {
    const usize off = in.xcount + 1;
    buf[Ix + off].length = msg_buf[Ix].length;
    buf[Ix + off].buffer = msg_buf[Ix].buffer;
  }

  win::IoStatusBlock io;
  return control_device_file(
    handle, io, win::consoleWrite,
    AddrRange::New(
      ptr_cast<>(&in),
      (total_size + 2 + /*End*/1) * sizeof(CSRMessage)
    ),
    AddrRange::New()
  );
}

inline win::NtStatus write_console(
 win::FileHandle handle, // TODO: Change type
 com::DualString str, usize count,
 usize* chars_written = nullptr
) {
  struct Params : CSRParams {
    win::ULong chars;
    win::Boolean is_wide;
  };

  (void) $unwrap_handle(handle);
  Params P {};
  AddrRange buf = str.visitR([&P, count](auto* S) -> AddrRange {
    P.is_wide = (sizeof(*S) > 1);
    return AddrRange(S, S + count);
  });

  CSRMessage msg {
    .length = u32(buf.sizeInBytes()),
    .buffer = buf.data()
  };

  win::NtStatus ret = write_generic(
    handle, win::GenericHandle(),
    P.init(AI_WriteConsole), &msg,
    CSRMessageBuf::New()
  );

  if ($NtSuccess(ret) && chars_written)
    *chars_written = (P.chars >> u32(P.is_wide));
  if (ret == /*STATUS_INVALID_PARAMETER*/ 0xC000000D)
    return 0xC0000008;
  return ret;
}

/*
struct CCSG_Message {
	win::ULong Length;
	void* Buffer;
};

struct CCSG_ParamsBase {
	win::ULong ControlCode;
	win::ULong Length;
};
 
win::NtStatus ConsoleCallServerGeneric(
  win::FileHandle FileHandle,
  win::FileHandle Handle,
  CCSG_ParamsBase* Buffer,
  win::ULong ControlCode,
  win::ULong Length,
  CCSG_Message* message, win::ULong nMessage,
  CCSG_Message* a8, win::ULong a9
) {
	if (nMessage + a9 > 8)
    // STATUS_NOT_SUPPORTED
    return 0xC00000BB;
	if (uptr(FileHandle) <= 0)
    return 0xC0000008;
 
	struct {
		win::FileHandle Handle;
		win::ULong Count;
		win::ULong Unk;
		struct
		{
			win::ULong Length, Unk;
			void* Buffer;
		} msg[3];
	} input;
 
	Buffer->ControlCode = ControlCode;
	Buffer->Length = Length;
 
	input.Handle = Handle;
	input.Count = nMessage + 1;
	input.Unk = a9 + 1;
 
	input.msg[0].Length = Length + sizeof(CCSG_ParamsBase);
	input.msg[0].Buffer = Buffer;
 
	input.msg[nMessage + 1].Length = Length;
	input.msg[nMessage + 1].Buffer = Buffer + 1;
 
	for (ULONG i = 0; i < nMessage; ++i)
	{
		input.msg[i + 1].Buffer = message[i].Buffer;
		input.msg[i + 1].Length = message[i].Length;
	}
	for (ULONG i = 0; i < a9; ++i)
	{
		input.msg[nMessage + i + 2].Length = a8[i].Length;
		input.msg[nMessage + i + 2].Buffer = a8[i].Buffer;
	}
 
	win::IoStatusBlock IoStatusBlock;
	return NtDeviceIoControlFile(FileHandle, 0, 0, 0, &IoStatusBlock, 0x500016, &input, 16 * (nMessage + a9 + 3), 0, 0);
}

win::NtStatus ConsoleCallServerWithBuffers(
  win::FileHandle Handle,
  CCSG_ParamsBase* Buffer,
  win::ULong ControlCode, win::ULong Length,
  CCSG_Message* message, BOOL nMessage,
  CCSG_Message* a7, win::ULong a8
) {
	return ConsoleCallServerGeneric(GetConsoleHandle(), Handle, Buffer, ControlCode, Length, message, nMessage, a7, a8);
}

win::NtStatus ConsoleCallServer(
  win::FileHandle FileHandle,
  CCSG_ParamsBase* Buffer,
  win::ULong ControlCode, win::ULong Length
) {
	return ConsoleCallServerGeneric(GetConsoleHandle(), FileHandle, Buffer, ControlCode, Length, 0, 0, 0, 0);
}
 
win::NtStatus WriteConsoleInternal(
  win::FileHandle hConsoleOutput,
  const void* lpBuffer, win::ULong nNumberOfCharsToWrite,
  win::ULong* lpNumberOfCharsWritten,
  bool bUnicode
) {
	struct Params : CCSG_ParamsBase {
		ULONG Chars;
		BOOLEAN bUnicode;
	};
 
	CCSG_Message message;
	message.Length = bUnicode ? nNumberOfCharsToWrite * 2 : nNumberOfCharsToWrite;
	message.Buffer = lpBuffer;
 
	Params params;
	params.bUnicode = bUnicode;
 
	NTSTATUS status = ConsoleCallServerGeneric(hConsoleOutput, NULL, &params, 16777222, 8, &message, TRUE, 0, 0);
	if (NT_SUCCESS(status) && lpNumberOfCharsWritten)
	{
		if (bUnicode) *lpNumberOfCharsWritten = (params.Chars / 2);
    else *lpNumberOfCharsWritten = params.Chars;
	}
	return (status == 0xC000000D) ? 0xC0000008 : status;
}
*/

} // inline namespace __nt
} // namespace hc::sys
