include_guard(DIRECTORY)

target_sources(hcrt-xcrt PRIVATE
  Phase0/Chkstk.cpp
  Phase0/InitCookie.cpp
  Phase0/Startup.cpp
  Phase0/Xtors.cpp

  Phase1/ArgParser.cpp
  Phase1/ConsoleSetup.cpp
  Phase1/Initialization.cpp
  Phase1/Locks.cpp
  Phase1/StdIO.cpp

  Memory/Box.cpp

  Shutdown/Atexit.cpp
  Shutdown/Exit.cpp
)

if(HC_EMUTLS)
  target_sources(hcrt-xcrt PRIVATE Phase1/EmuTLS.cpp)
else()
  target_sources(hcrt-xcrt PRIVATE Phase1/TLS.cpp)
endif()
