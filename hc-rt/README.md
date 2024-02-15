# The HC Runtime

This folder defines the HC compiler runtime.
Currently only supported on Windows.

The runtime is split into two parts: backend and frontend.
The backend is found in ``xcrt``, and defines all the things
going on "behind the scenes" (startup, static init, builtins, ...).
The frontend is found in ``include``/``src``, and has all
the definitions used externally.

Many of the section's functions are obvious,
but some of the less intuitive ones are:

- ``xcrt/Phase0``: The entry point, constructor/cookie setup
- ``xcrt/Phase1``: Static init, TLS definitions, argv parsing, etc.
- ``Bootstrap``: Definitions for Windows syscall extraction/loading
- ``Parcel``: Stack-allocated dynamic data structures (and their dependencies)
- ``Std``: Implements things from the STL (suggested with warnings)
