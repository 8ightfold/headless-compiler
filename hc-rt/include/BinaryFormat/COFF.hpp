//===- BinaryFormat/COFF.hpp ----------------------------------------===//
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

#include <Common/Features.hpp>
#include <Common/Fundamental.hpp>
#include <Common/PtrUnion.hpp>
#include <Common/PtrRange.hpp>
#include <Common/TaggedEnum.hpp>

// For more info:
// https://wiki.osdev.org/PE
// https://wiki.osdev.org/COFF
// http://justsolve.archiveteam.org/wiki/MS-DOS_EXE
// http://www.microsoft.com/whdc/system/platform/firmware/PECOFF.mspx

namespace hc::binfmt {
namespace COFF {
  $Enum((COFFInfo, u32),
    // Dos stuff
    (eDosMagic,           0x5A4D),
    (eDosPageSize,        512),
    (eDosParagraphSize,   16),
    // COFF
    (eCOFFMagic,          0x00004550),
    (eCOFFOptMagicPE32,   0x010B),
    (eCOFFOptMagicPE64,   0x020B),
    (eCOFFOptMagicROM,    0x0107),
    // General info
    (eCOFFMaxSections,    96),
    (eCOFFNameSize,       8)
  );

  $MarkPrefix(COFFInfo, "e")

  //====================================================================//
  // DOS
  //====================================================================//

  struct DosHeader {
    u16 magic = COFFInfo::eDosMagic;
    u16 last_page_bytes;
    u16 page_count;  // N * eDosPageSize
    u16 relocation_count;
    u16 header_size; // N * eDosParagraphSize
    u16 min_allocation;
    u16 max_allocation;
    i16 ss_register_init;
    u16 sp_register_init;
    u16 checksum = 0;
    u16 ip_register_init;
    i16 cs_register_init;
    u16 relocation_offset;
    u16 __dos_reserved[5];
    u16 OEM_id;
    u16 OEM_info;
    u16 __oem_reserved[10];
    u16 coff_header_offset;
  };

  //====================================================================//
  // COFF Header
  //====================================================================//

  $Enum((MachineType, u16),
    (eMachineAny,    0x0000), // ugh
    (eMachineAMD64,  0x8664), // x64
    (eMachineEBC,    0x0EBC), // EFI bytecode
    (eMachineI386,   0x014C), // Intel 386+
    (eMachineIA64,   0x0200)  // Intel Itanium
    // ...
  );

  $Enum((Characteristics, u16),
    (eStrippedRelocations,  0x0001),
    (eExecutableImage,      0x0002),
    (eLargeAddressAware,    0x0020),
    // ...
    (e32BitMachine,         0x0100),
    (eStrippedDebugInfo,    0x0200),
    // ...
    (eFiletypeSystem,       0x1000),
    (eFiletypeDLL,          0x2000),
    (eUniprocessorOnly,     0x4000)
    // ...
  );

  $MarkPrefix(MachineType, "eMachine")
  $MarkPrefix(Characteristics, "e")

  struct [[gnu::packed]] FileHeader {
    u32 magic = COFFInfo::eCOFFMagic;
    MachineType machine_type;
	  u16 section_count; // Must be < eCOFFMaxSections
	  u32 timestamp;
	  u32 sym_tbl_addr = 0;
	  u32 symbol_count = 0;
	  u16 optional_header_size;
	  Characteristics info_flags;
  };

  // Optional Headers

  struct OptPE32Header {
    u32 magic = COFFInfo::eCOFFOptMagicPE32;
    u8  major_linker_version;
	  u8  minor_linker_version;
	  u32 code_size;
	  u32 initialized_data_size;
	  u32 uninitialized_data_size;
	  u32 entry_point_addr;
    u32 base_of_data;
  };

  struct OptPE64Header {
    u32 magic = COFFInfo::eCOFFOptMagicPE64;
    u8  major_linker_version;
	  u8  minor_linker_version;
	  u32 code_size;
	  u32 initialized_data_size;
	  u32 uninitialized_data_size;
	  u32 entry_point_addr;
  };

  using OptPEHeader = $PUnion(
    OptPE64Header, OptPE32Header);

  //====================================================================//
  // Windows COFF Extension
  //====================================================================//

  $Enum((WindowsSubsystemType, u16),
    (eSubsystemUnknown,    0),
    (eSubsystemNative,     1),
    (eSubsystemWindowsGUI, 2),
    (eSubsystemWindowsCUI, 3),
    (eSubsystemPosixCUI,   7),
    // ...
    (eSubsystemEFIApp,     10),
    (eSubsystemEFIBoot,    11),
    (eSubsystemEFIRuntime, 12),
    (eSubsystemEFIROM,     13)
    // ...
  );

  $Enum((WindowsDLLCharacteristics, u16),
    // ...
    (eDLLHighEntropy,         0x0020),
    (eDLLDynamicBase,         0x0040),
    (eDLLForceIntegrity,      0x0080),
    (eDLLNXCompatible,        0x0100),
    (eDLLNoIsolation,         0x0200),
    (eDLLNoSEH,               0x0400),
    (eDLLNoBind,              0x0800),
    (eDLLAppContainer,        0x1000),
    (eDLLWDMDriver,           0x2000),
    (eDLLGuardControlFlow,    0x4000),
    (eDLLTerminalServerAware, 0x8000)
  );

  $MarkPrefix(WindowsSubsystemType, "eSubsystem")
  $MarkPrefix(WindowsDLLCharacteristics, "eDLL")

  template <usize PointerSize = sizeof(void*)>
  struct OptPEWindowsHeader {
    using IPtr = intn_t<PointerSize>;
    using UPtr = uintn_t<PointerSize>;
  public:
    UPtr image_base;
    u32  section_alignment;
    u32  file_alignment;
    u16  major_OS_version;
    u16  minor_OS_version;
    u16  major_image_version;
    u16  minor_image_version;
    u16  major_subsystem_version;
    u16  minor_subsystem_version;
    u32  __win32_version = 0;
    u32  size_of_image;
    u32  size_of_headers;
    u32  checksum;
    WindowsSubsystemType subsystem;
    WindowsDLLCharacteristics dll_flags;
    UPtr stack_reserve_size;
    UPtr stack_commit_size;
    UPtr heap_reserve_size;
    UPtr heap_commit_size;
    u32  __loader_flags = 0;
    u32  RVA_and_sizes_count;
  };

  using PEWindowsHeader = $PUnion(
    OptPEWindowsHeader<8>, OptPEWindowsHeader<4>);

  //====================================================================//
  // Data Directory
  //====================================================================//

  struct DataDirectoryHeader {
    u32 RVA;
    u32 size;
  };

  $Enum((DataDirectories, u32),
    (eDirectoryExportTable, 0),
    (eDirectoryImportTable),
    (eDirectoryResourceTable),
    (eDirectoryExceptionTable),
    (eDirectoryCertificateTable),
    (eDirectoryBaseRelocationTable),
    (eDirectoryDebug),
    (eDirectoryArchitecture),
    (eDirectoryGlobalPtr),
    (eDirectoryTLSTable),
    (eDirectoryLoadConfigTable),
    (eDirectoryBoundImport),
    (eDirectoryImportAddressTable),
    (eDirectoryDelayImportDescriptor),
    (eDirectoryCLRRuntimeHeader),
    (eDirectoryMaxValue)
  );

  $MarkPrefix(DataDirectories, "eDirectory")
  using DataDirectoryTable = $PRange(DataDirectoryHeader);

  //====================================================================//
  // Section Data
  //====================================================================//

  $Enum((SectionFlags, u32),
    // ...
    (eSectionNoPad             , 0x00000008),
    // ...
    (eSectionCntCode           , 0x00000020),
    (eSectionInitializedData   , 0x00000040),
    (eSectionUninitializedData , 0x00000080),
    (eSectionLinkOther         , 0x00000100),
    (eSectionLinkInfo          , 0x00000200),
    // ...
    (eSectionLinkRemove        , 0x00000800),
    (eSectionLinkComdat        , 0x00001000),
    (eSectionGlobalPtrRelative , 0x00008000),
    // Reserved Mem flags...
    (eSectionAlign1Bytes       , 0x00100000),
    (eSectionAlign2Bytes       , 0x00200000),
    (eSectionAlign4Bytes       , 0x00300000),
    (eSectionAlign8Bytes       , 0x00400000),
    (eSectionAlign16Bytes      , 0x00500000),
    (eSectionAlign32Bytes      , 0x00600000),
    (eSectionAlign64Bytes      , 0x00700000),
    (eSectionAlign128Bytes     , 0x00800000),
    (eSectionAlign256Bytes     , 0x00900000),
    (eSectionAlign512Bytes     , 0x00A00000),
    (eSectionAlign1024Bytes    , 0x00B00000),
    (eSectionAlign2048Bytes    , 0x00C00000),
    (eSectionAlign4096Bytes    , 0x00D00000),
    (eSectionAlign8192Bytes    , 0x00E00000),
    (eSectionLink32BitReloc    , 0x01000000),
    (eSectionMemDiscardable    , 0x02000000),
    (eSectionMemNotCached      , 0x04000000),
    (eSectionMemNotPaged       , 0x08000000),
    (eSectionMemShared         , 0x10000000),
    (eSectionMemExecute        , 0x20000000),
    (eSectionMemRead           , 0x40000000),
    (eSectionMemWrite          , 0x80000000)
  );

  $MarkPrefix(SectionFlags, "eSection")

  struct [[gnu::packed]] SectionHeader {
    char name[eCOFFNameSize]; // TODO: handle string table offset
    u32  virtual_size;
    u32  virtual_addr;
    u32  raw_data_size;
    u32  raw_data_addr;
    u32  relocation_addr;
    u32  __linenumber_addr = 0;
    u16  relocation_count;
    u16  __linenumber_count = 0;
    SectionFlags characteristics;
  };

  using SectionTable = $PRange(SectionHeader);

  //====================================================================//
  // Symbol Table
  //====================================================================//

  /// Least significant byte
  $Enum((SymbolBaseType, u8),
    (eSymTypeNull,        0),
    (eSymTypeVoid,        1),
    (eSymTypeChar,        2),
    (eSymTypeShort,       3),
    (eSymTypeInt,         4),
    (eSymTypeLong,        5),
    (eSymTypeFloat,       6),
    (eSymTypeDouble,      7),
    (eSymTypeStruct,      8),
    (eSymTypeUnion,       9),
    (eSymTypeEnum,        10),
    (eSymTypeEnumMember,  11),
    (eSymTypeByte,        12),
    (eSymTypeWord,        13),
    (eSymTypeUInt,        14),
    (eSymTypeDWord,       15)
  );

  /// Most significant byte
  $Enum((SymbolComplexType, u8),
    (eSymDTypeNull,       0),
    (eSymDTypePointer,    1),
    (eSymDTypeFunction,   2),
    (eSymDTypeArray,      3)
  );

  $Enum((SymbolClass, u8),
    // Storage Specifiers
    (eSymClassNull,             0),
    (eSymClassAuto,             1),
    (eSymClassExtern,           2),
    (eSymClassExternDef,        5),
    (eSymClassWeakExtern,       105),
    (eSymClassStatic,           3),
    (eSymClassUndefinedStatic,  14),
    (eSymClassRegister,         4),
    (eSymClassRegisterParam,    17),
    (eSymClassLabel,            6),
    (eSymClassUndefinedLabel,   7),
    // Types
    (eSymClassTypedef,          13),
    (eSymClassStructMember,     8),
    (eSymClassStructTag,        10),
    (eSymClassUnionMember,      11),
    (eSymClassUnionTag,         12),
    (eSymClassEnumMember,       16),
    (eSymClassEnumTag,          15),
    (eSymClassBitfield,         18),
    // Records
    (eSymClassFunction,         101),
    (eSymClassArgument,         9),
    (eSymClassEndOfFunction,    0xFF),
    (eSymClassEndOfStruct,      102),
    (eSymClassBlock,            100),
    (eSymClassFile,             103),
    (eSymClassSection,          104)
  );

  $MarkPrefix(SymbolBaseType,    "eSymType")
  $MarkPrefix(SymbolComplexType, "eSymDType")
  $MarkPrefix(SymbolClass,       "eSymClass")

  union SymbolName {
    char short_name[eCOFFNameSize];
    struct [[gnu::packed]] {
      u32 zeroes;
      u32 table_offset;
    };
  };

  union SymbolType {
    u16 value;
    struct [[gnu::packed]] {
      SymbolBaseType    LSB;
      SymbolComplexType MSB;
    };
  };

  struct [[gnu::packed]] SymbolRecord {
    SymbolName  name;
    u32         value; // Depends on `section_number` and `storage_class`.
    i16         section_number; // Beware [-2, 0]
    SymbolType  type;
    SymbolClass storage_class;
    u8          aux_count;
  };

  using SymbolTable = $PRange(SymbolRecord);

  //====================================================================//
  // Misc.
  //====================================================================//

  struct StringTableHeader {
    u32  size_in_bytes;
    char strings[];
  };

  // Export Directory

  struct ExportDirectoryTable {
    u32 __export_flags = 0;
    u32 timestamp;
    u16 major_version;
    u16 minor_version;
    u32 name_RVA;
    u32 ordinal_base = 1;
    u32 export_address_table_count;
    u32 name_pointer_table_count;
    u32 export_address_table_RVA;
    u32 name_pointer_table_RVA;
    u32 ordinal_table_RVA;
  };

  using NamePointerType = u32;
  using OrdinalType = u16;

  // TODO: Other export tables


  static_assert(sizeof(FileHeader) == 20 + 4); // Size + Magic
  static_assert(sizeof(SectionHeader) == 40);
  static_assert(sizeof(SymbolName) == 8);
  static_assert(sizeof(SymbolType) == 2);
  static_assert(sizeof(SymbolRecord) == 18);
  static_assert(sizeof(StringTableHeader) == 4);
  static_assert(sizeof(ExportDirectoryTable) == 40);

} // namespace COFF
} // namespace hc::binfmt
