#pragma once

typedef char CHAR;
typedef wchar_t WCHAR;
typedef unsigned char UCHAR;
typedef short SHORT;
typedef unsigned short USHORT;
typedef long LONG;
typedef unsigned long ULONG;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;

typedef UCHAR BOOLEAN;
typedef long DWORD;
typedef ULONGLONG ULONG64;
typedef ULONGLONG SIZE_T;
typedef int NTSTATUS;

typedef void *PVOID;
typedef struct __handle_* HANDLE;

typedef struct _KSYSTEM_TIME {
  ULONG LowPart;
  LONG  High1Time;
  LONG  High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

typedef enum _NT_PRODUCT_TYPE {
  NtProductWinNt = 1,
  NtProductLanManNt = 2,
  NtProductServer = 3
} NT_PRODUCT_TYPE;

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE {
  StandardDesign = 0,
  NEC98x86 = 1,
  EndAlternatives = 2
} ALTERNATIVE_ARCHITECTURE_TYPE;

#define PROCESSOR_FEATURE_MAX 64

typedef union _LARGE_INTEGER {
  struct {
    DWORD LowPart;
    LONG  HighPart;
  } u;
  LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

struct _XSTATE_FEATURE {
  ULONG Offset;
  ULONG Size;
}; 

typedef struct _XSTATE_CONFIGURATION {
  ULONGLONG                     EnabledFeatures;                     
  ULONGLONG                     EnabledVolatileFeatures;             
  ULONG                         Size;                                    
  union {
    ULONG                       ControlFlags;                        
    struct {
      ULONG                     OptimizedSave : 1;                 
      ULONG                     CompactionEnabled : 1;             
      ULONG                     ExtendedFeatureDisable : 1;        
    };
  };
  struct _XSTATE_FEATURE        Features[64];
  ULONGLONG                     EnabledSupervisorFeatures;
  ULONGLONG                     AlignedFeatures;
  ULONG                         AllFeatureSize;
  ULONG                         AllFeatures[64];
  ULONGLONG                     EnabledUserVisibleSupervisorFeatures;
  ULONGLONG                     ExtendedFeatureDisableFeatures;
  ULONG                         AllNonLargeFeatureSize;
  ULONG                         Spare;
} XSTATE_CONFIGURATION, *PXSTATE_CONFIGURATION;

typedef enum _MEMORY_INFORMATION_CLASS {
  MemoryBasicInformation,           // MEMORY_BASIC_INFORMATION
  MemoryWorkingSetInformation,      // MEMORY_WORKING_SET_INFORMATION
  MemoryMappedFilenameInformation,  // UNICODE_STRING
  MemoryRegionInformation,          // MEMORY_REGION_INFORMATION
  MemoryWorkingSetExInformation,    // MEMORY_WORKING_SET_EX_INFORMATION
  MemorySharedCommitInformation     // MEMORY_SHARED_COMMIT_INFORMATION
} MEMORY_INFORMATION_CLASS;

struct _KUSER_SHARED_DATA {
  ULONG                         TickCountLowDeprecated;
  ULONG                         TickCountMultiplier;
  volatile KSYSTEM_TIME         InterruptTime;
  volatile KSYSTEM_TIME         SystemTime;
  volatile KSYSTEM_TIME         TimeZoneBias;
  USHORT                        ImageNumberLow;
  USHORT                        ImageNumberHigh;
  WCHAR                         NtSystemRoot[260];
  ULONG                         MaxStackTraceDepth;
  ULONG                         CryptoExponent;
  ULONG                         TimeZoneId;
  ULONG                         LargePageMinimum;
  ULONG                         AitSamplingValue;
  ULONG                         AppCompatFlag;
  ULONGLONG                     RNGSeedVersion;
  ULONG                         GlobalValidationRunlevel;
  volatile LONG                 TimeZoneBiasStamp;
  ULONG                         NtBuildNumber;
  NT_PRODUCT_TYPE               NtProductType;
  BOOLEAN                       ProductTypeIsValid;
  BOOLEAN                       Reserved0[1];
  USHORT                        NativeProcessorArchitecture;
  ULONG                         NtMajorVersion;
  ULONG                         NtMinorVersion;
  BOOLEAN                       ProcessorFeatures[PROCESSOR_FEATURE_MAX];
  ULONG                         Reserved1;
  ULONG                         Reserved3;
  volatile ULONG                TimeSlip;
  ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;
  ULONG                         BootId;
  LARGE_INTEGER                 SystemExpirationDate;
  ULONG                         SuiteMask;
  BOOLEAN                       KdDebuggerEnabled;
  union {
    UCHAR                       MitigationPolicies;
    struct {
      UCHAR                     NXSupportPolicy : 2;
      UCHAR                     SEHValidationPolicy : 2;
      UCHAR                     CurDirDevicesSkippedForDlls : 2;
      UCHAR                     Reserved : 2;
    };
  };
  USHORT                        CyclesPerYield;
  volatile ULONG                ActiveConsoleId;
  volatile ULONG                DismountCount;
  ULONG                         ComPlusPackage;
  ULONG                         LastSystemRITEventTickCount;
  ULONG                         NumberOfPhysicalPages;
  BOOLEAN                       SafeBootMode;
  union {
    UCHAR                       VirtualizationFlags;
    struct {
      UCHAR                     ArchStartedInEl2 : 1;
      UCHAR                     QcSlIsSupported : 1;
    };
  };
  UCHAR                         Reserved12[2];
  union {
    ULONG                       SharedDataFlags;
    struct {
      ULONG                     ErrorPortPresent : 1;
      ULONG                     ElevationEnabled : 1;
      ULONG                     VirtEnabled : 1;
      ULONG                     InstallerDetectEnabled : 1;
      ULONG                     LkgEnabled : 1;
      ULONG                     DynProcessorEnabled : 1;
      ULONG                     ConsoleBrokerEnabled : 1;
      ULONG                     SecureBootEnabled : 1;
      ULONG                     MultiSessionSku : 1;
      ULONG                     MultiUsersInSessionSku : 1;
      ULONG                     StateSeparationEnabled : 1;
      ULONG                     SpareBits : 21;
    } Dbg;
  } DUMMYUNIONNAME2;
  ULONG                         DataFlagsPad[1];
  ULONGLONG                     TestRetInstruction;
  LONGLONG                      QpcFrequency;
  ULONG                         SystemCall;
  ULONG                         Reserved2;
  ULONGLONG                     FullNumberOfPhysicalPages;
  ULONGLONG                     SystemCallPad[1];
  union {
    volatile KSYSTEM_TIME       TickCount;
    volatile ULONG64            TickCountQuad;
    struct {
      ULONG                     ReservedTickCountOverlay[3];
      ULONG                     TickCountPad[1];
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME3;
  ULONG                         Cookie;
  ULONG                         CookiePad[1];
  LONGLONG                      ConsoleSessionForegroundProcessId;
  ULONGLONG                     TimeUpdateLock;
  ULONGLONG                     BaselineSystemTimeQpc;
  ULONGLONG                     BaselineInterruptTimeQpc;
  ULONGLONG                     QpcSystemTimeIncrement;
  ULONGLONG                     QpcInterruptTimeIncrement;
  UCHAR                         QpcSystemTimeIncrementShift;
  UCHAR                         QpcInterruptTimeIncrementShift;
  USHORT                        UnparkedProcessorCount;
  ULONG                         EnclaveFeatureMask[4];
  ULONG                         TelemetryCoverageRound;
  USHORT                        UserModeGlobalLogger[16];
  ULONG                         ImageFileExecutionOptions;
  ULONG                         LangGenerationCount;
  ULONGLONG                     Reserved4;                                 
  volatile ULONGLONG            InterruptTimeBias;                
  volatile ULONGLONG            QpcBias;                          
  ULONG                         ActiveProcessorCount;                          
  volatile UCHAR                ActiveGroupCount;                     
  UCHAR                         Reserved9;                                     
  union {
    USHORT                      QpcData;                                  
    struct {
      volatile UCHAR            QpcBypassEnabled;             
      UCHAR                     QpcShift;                              
    };
  };
  LARGE_INTEGER                 TimeZoneBiasEffectiveStart;     
  LARGE_INTEGER                 TimeZoneBiasEffectiveEnd;       
  XSTATE_CONFIGURATION          XState;
  KSYSTEM_TIME                  FeatureConfigurationChangeStamp;
  ULONG                         Spare;
  ULONGLONG                     UserPointerAuthMask;
};

typedef NTSTATUS(__stdcall *PPSS_CAPTURE_ROUTINE)(HANDLE, PVOID, MEMORY_INFORMATION_CLASS, PVOID, SIZE_T, SIZE_T *);

typedef struct _MEMORY_BASIC_INFORMATION {
  PVOID    BaseAddress;
  PVOID    AllocationBase;
  DWORD    AllocationProtect;
  SIZE_T   RegionSize;
  DWORD    State;
  DWORD    Protect;
  DWORD    Type;
} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;

extern _KUSER_SHARED_DATA KUSER_SHARED_DATA;
__asm__ (".equ KUSER_SHARED_DATA, 0x7FFE0000");
