# CrafterHunter MHW endpoint

This directory is an experimental 64-bit native Windows DLL reserved for later
graphics work. It is not CrafterHunter's supported first-stage MHW endpoint;
use `native/mhw-spl-plugin` for the Proton loader proof.

The current DLL only connects to the local bridge and sends lifecycle
heartbeats. It performs no signature scanning, memory reads, patching, graphics
hooking, or gameplay modification.

Build from an x64 Visual Studio developer prompt:

```powershell
cmake -S native/mhw-plugin -B native/mhw-plugin/build -A x64
cmake --build native/mhw-plugin/build --config Release
```

Windows is not required. On Linux, install your distribution's CMake and
MinGW-w64 C++ packages, then cross-compile the Windows DLL:

```bash
./tools/build-mhw-linux.sh
```

The output is `native/mhw-plugin/build-mingw64/CrafterHunter.MHW.dll`. MHW
continues to run as a Windows process under Proton; only the build host is
Linux.

Do not install this DLL. It has no supported loader contract yet.
