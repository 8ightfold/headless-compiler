//===- Bootstrap/CheckPacking.cpp -----------------------------------===//
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
//  Does checks to see if Ke types are correctly packed.
//
//===----------------------------------------------------------------===//

#include <Common/Features.hpp>
#include <Bootstrap/KUserSharedData.hpp>

using namespace hc::bootstrap;

#define KSD_TEST(offset, member) \
 static_assert($offsetof(member, KUserSharedData) == offset, \
  "KUSER_SHARED_DATA::" #member \
  " is offset incorrectly. Please report this.")

KSD_TEST(0x0,   TickCountLowDeprecated);
KSD_TEST(0x4,   TickCountMultiplier);
KSD_TEST(0x8,   InterruptTime);
KSD_TEST(0x14,  SystemTime);
KSD_TEST(0x20,  TimeZoneBias);
KSD_TEST(0x2c,  ImageNumberLow);
KSD_TEST(0x2e,  ImageNumberHigh);
KSD_TEST(0x30,  NtSystemRoot);
KSD_TEST(0x238, MaxStackTraceDepth);
KSD_TEST(0x23c, CryptoExponent);
KSD_TEST(0x240, TimeZoneId);
KSD_TEST(0x244, LargePageMinimum);
KSD_TEST(0x248, AitSamplingValue);
KSD_TEST(0x24c, AppCompatFlag);
KSD_TEST(0x250, RNGSeedVersion);
KSD_TEST(0x258, GlobalValidationRunlevel);
KSD_TEST(0x25c, TimeZoneBiasStamp);
KSD_TEST(0x260, NtBuildNumber);
KSD_TEST(0x264, ProductType);
KSD_TEST(0x268, ProductTypeIsValid);
KSD_TEST(0x26a, NativeProcessorArchitecture);
KSD_TEST(0x26c, NtMajorVersion);
KSD_TEST(0x270, NtMinorVersion);
KSD_TEST(0x274, ProcessorFeatures);
KSD_TEST(0x2bc, TimeSlip);
KSD_TEST(0x2c0, AlternativeArchitecture);
KSD_TEST(0x2c4, BootId);
KSD_TEST(0x2c8, SystemExpirationDate);
KSD_TEST(0x2d0, SuiteMask);
KSD_TEST(0x2d4, KdDebuggerEnabled);

KSD_TEST(0x2d5, MitigationPolicies);

KSD_TEST(0x2d6, CyclesPerYield);
KSD_TEST(0x2d8, ActiveConsoleId);
KSD_TEST(0x2dc, DismountCount);
KSD_TEST(0x2e0, ComPlusPackage);
KSD_TEST(0x2e4, LastSystemRITEventTickCount);
KSD_TEST(0x2e8, NumberOfPhysicalPages);
KSD_TEST(0x2ec, SafeBootMode);

KSD_TEST(0x2ed, VirtualizationFlags);

KSD_TEST(0x2f0, SharedDataFlags);

KSD_TEST(0x2f8, TestRetInstruction);
KSD_TEST(0x300, QpcFrequency);
KSD_TEST(0x308, SystemCall);

KSD_TEST(0x320, TickCount);
KSD_TEST(0x320, TickCountQuad);
KSD_TEST(0x320, ReservedTickCountOverlay);

KSD_TEST(0x330, Cookie);
KSD_TEST(0x338, ConsoleSessionForegroundProcessId);
KSD_TEST(0x340, TimeUpdateLock);
KSD_TEST(0x348, BaselineSystemTimeQpc);
KSD_TEST(0x350, BaselineInterruptTimeQpc);
KSD_TEST(0x358, QpcSystemTimeIncrement);
KSD_TEST(0x360, QpcInterruptTimeIncrement);
KSD_TEST(0x368, QpcSystemTimeIncrementShift);
KSD_TEST(0x369, QpcInterruptTimeIncrementShift);
KSD_TEST(0x36a, UnparkedProcessorCount);
KSD_TEST(0x36c, EnclaveFeatureMask);
KSD_TEST(0x37c, TelemetryCoverageRound);
KSD_TEST(0x380, UserModeGlobalLogger);
KSD_TEST(0x3a0, ImageFileExecutionOptions);
KSD_TEST(0x3a4, LangGenerationCount);
KSD_TEST(0x3b0, InterruptTimeBias);
KSD_TEST(0x3b8, QpcBias);
KSD_TEST(0x3c0, ActiveProcessorCount);
KSD_TEST(0x3c4, ActiveGroupCount);
KSD_TEST(0x3c6, QpcData);
KSD_TEST(0x3c6, QpcBypassEnabled);
KSD_TEST(0x3c7, QpcShift);
KSD_TEST(0x3c8, TimeZoneBiasEffectiveStart);
KSD_TEST(0x3d0, TimeZoneBiasEffectiveEnd);
KSD_TEST(0x3d8, XState);
KSD_TEST(0x720, FeatureConfigurationChangeStamp);
KSD_TEST(0x730, UserPointerAuthMask);
