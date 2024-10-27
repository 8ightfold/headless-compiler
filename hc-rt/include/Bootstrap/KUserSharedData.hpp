//===- Bootstrap/KUserSharedData.hpp --------------------------------===//
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
//  This file defines the KUSER_SHARED_DATA structure and global.
//  Doesn't follow our normal conventions, but I really don't care
//  as it's pretty special.
//
//===----------------------------------------------------------------===//

#pragma once

#include <Bootstrap/WinapiDefs.hpp>

namespace hc::bootstrap {

enum class NtProductType {
  WinNt = 1,
  LanManNt = 2,
  Server = 3
};

enum class AltArchType {
  StandardDesign = 0,
  NEC98x86 = 1,
  EndAlternatives = 2
};

enum ProcessorFeatureInfo {
  eProcessorFeatureMax = 64
};

struct XStateFeature {
  u32 offset;
  u32 size;
};

struct XStateConfig {
  u64           EnabledFeatures;                     
  u64           EnabledVolatileFeatures;             
  u32           Size;                                    
  union {
    u32         ControlFlags;                        
    struct {
      u32       OptimizedSave : 1;                 
      u32       CompactionEnabled : 1;             
      u32       ExtendedFeatureDisable : 1;        
    };
  };
  XStateFeature Features[eProcessorFeatureMax];
  u64           EnabledSupervisorFeatures;
  u64           AlignedFeatures;
  u32           AllFeatureSize;
  u32           AllFeatures[eProcessorFeatureMax];
  u64           EnabledUserVisibleSupervisorFeatures;
  u64           ExtendedFeatureDisableFeatures;
  u32           AllNonLargeFeatureSize;
  u32           __Spare;
};

//======================================================================//
// Implementation
//======================================================================//

struct KUserSharedData {
  u32                     TickCountLowDeprecated;
  u32                     TickCountMultiplier;
  volatile KSystemTime    InterruptTime;
  volatile KSystemTime    SystemTime;
  volatile KSystemTime    TimeZoneBias;
  u16                     ImageNumberLow;
  u16                     ImageNumberHigh;
  wchar_t                 NtSystemRoot[260];
  u32                     MaxStackTraceDepth;
  u32                     CryptoExponent;
  u32                     TimeZoneId;
  u32                     LargePageMinimum;
  u32                     AitSamplingValue;
  u32                     AppCompatFlag;
  u64                     RNGSeedVersion;
  u32                     GlobalValidationRunlevel;
  volatile i32            TimeZoneBiasStamp;
  u32                     NtBuildNumber;
  NtProductType           ProductType;
  Win64Bool               ProductTypeIsValid;
  Win64Bool               __Reserved0[1];
  u16                     NativeProcessorArchitecture;
  u32                     NtMajorVersion;
  u32                     NtMinorVersion;
  Win64Bool               ProcessorFeatures[eProcessorFeatureMax];
  u32                     __Reserved1[2];
  volatile u32            TimeSlip;
  AltArchType             AlternativeArchitecture;
  u32                     BootId;
  LargeInt                SystemExpirationDate;
  u32                     SuiteMask;
  Win64Bool               KdDebuggerEnabled;
  union {
    ubyte                 MitigationPolicies;
    struct {
      ubyte               NXSupportPolicy : 2;
      ubyte               SEHValidationPolicy : 2;
      ubyte               CurDirDevicesSkippedForDlls : 2;
      ubyte               __Reserved2 : 2;
    };
  };
  u16                     CyclesPerYield;
  volatile u32            ActiveConsoleId;
  volatile u32            DismountCount;
  u32                     ComPlusPackage;
  u32                     LastSystemRITEventTickCount;
  u32                     NumberOfPhysicalPages;
  Win64Bool               SafeBootMode;
  union {
    ubyte                 VirtualizationFlags;
    struct {
      ubyte               ArchStartedInEl2 : 1;
      ubyte               QcSlIsSupported : 1;
    };
  };
  ubyte                   __Reserved3[2];
  union {
    u32                   SharedDataFlags;
    struct {
      u32                 ErrorPortPresent : 1;
      u32                 ElevationEnabled : 1;
      u32                 VirtEnabled : 1;
      u32                 InstallerDetectEnabled : 1;
      u32                 LkgEnabled : 1;
      u32                 DynProcessorEnabled : 1;
      u32                 ConsoleBrokerEnabled : 1;
      u32                 SecureBootEnabled : 1;
      u32                 MultiSessionSku : 1;
      u32                 MultiUsersInSessionSku : 1;
      u32                 StateSeparationEnabled : 1;
      u32                 SpareBits : 21;
    } Dbg;
  };
  u32                     __DataFlagsPad[1];
  u64                     TestRetInstruction;
  i64                     QpcFrequency;
  u32                     SystemCall;
  u32                     __Reserved4;
  u64                     FullNumberOfPhysicalPages;
  u64                     __SystemCallPad[1];
  union {
    volatile KSystemTime  TickCount;
    volatile u64          TickCountQuad;
    struct {
      u32                 ReservedTickCountOverlay[3];
      u32                 __TickCountPad[1];
    };
  };
  u32                     Cookie;
  u32                     __CookiePad[1];
  i64                     ConsoleSessionForegroundProcessId;
  u64                     TimeUpdateLock;
  u64                     BaselineSystemTimeQpc;
  u64                     BaselineInterruptTimeQpc;
  u64                     QpcSystemTimeIncrement;
  u64                     QpcInterruptTimeIncrement;
  ubyte                   QpcSystemTimeIncrementShift;
  ubyte                   QpcInterruptTimeIncrementShift;
  u16                     UnparkedProcessorCount;
  u32                     EnclaveFeatureMask[4];
  u32                     TelemetryCoverageRound;
  u16                     UserModeGlobalLogger[16];
  u32                     ImageFileExecutionOptions;
  u32                     LangGenerationCount;
  u64                     __Reserved5;                                 
  volatile u64            InterruptTimeBias;                
  volatile u64            QpcBias;                          
  u32                     ActiveProcessorCount;                          
  volatile ubyte          ActiveGroupCount;                     
  ubyte                   __Reserved6;                                     
  union {
    u16                   QpcData;                                  
    struct {
      volatile ubyte      QpcBypassEnabled;             
      ubyte               QpcShift;                              
    };
  };
  LargeInt                TimeZoneBiasEffectiveStart;     
  LargeInt                TimeZoneBiasEffectiveEnd;       
  XStateConfig            XState;
  KSystemTime             FeatureConfigurationChangeStamp;
  u32                     __Spare;
  u64                     UserPointerAuthMask;
};

extern "C" {
  extern hc::bootstrap::KUserSharedData KUSER_SHARED_DATA;
  __asm__ (".equ KUSER_SHARED_DATA, 0x7ffe0000");
} // extern "C"

inline constinit XStateConfig&
  KUSER_XState = KUSER_SHARED_DATA.XState;

} // namespace hc::bootstrap
