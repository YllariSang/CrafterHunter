# CrafterHunter SharpPluginLoader endpoint

This is the supported first-stage MHW adapter. It is a .NET 8 plugin for
[SharpPluginLoader](https://github.com/Fexty12573/SharpPluginLoader), whose
Linux release officially supports Steam Proton/Wine.

The current plugin registers the MHW process with the local CrafterHunter
bridge, sends heartbeats, and publishes read-only camera telemetry through
SharpPluginLoader's supported camera API. Version 0.2 also has an opt-in
wireframe rendering probe using SharpPluginLoader's `OnRender` and 3D primitive
API. It does not modify gameplay. See
[the render-probe test](../../docs/render-probe-test.md) for setup and the
real-game acceptance questions.

Camera access is fail-closed: the endpoint waits for a valid bridge `HelloAck`
and a ten-second startup grace period before touching MHW's native camera
singleton. Managed camera errors disable telemetry instead of escaping the
plugin callback. Startup stages are appended to
`nativePC/plugins/CSharp/CrafterHunter/CrafterHunter.runtime.log` so a crash can
be separated into loader, endpoint, and camera-read stages without attaching a
debugger.

For a staged crash check, first launch MHW without the CrafterHunter bridge.
The endpoint can load and write its first two diagnostic lines, but camera
access stays disabled. Then run the bridge before the next launch. Interpret
the last runtime-log line as follows:

- no new line: the process stopped before the endpoint's `OnLoad` callback;
- `Endpoint task started`: camera access never became eligible;
- `Beginning the first guarded camera read`: native camera access began but did
  not return;
- `First guarded camera read succeeded`: the camera path completed at least
  once.

Build it on Linux with:

```bash
./tools/build-mhw-spl.sh
```

After installing SharpPluginLoader from its official Linux release, install
the CrafterHunter plugin with:

```bash
./tools/install-mhw-spl-plugin.sh install
```

The installer only manages
`nativePC/plugins/CSharp/CrafterHunter/CrafterHunter.MHW.dll`. It refuses to
replace an unmarked directory or a file changed outside the installer.
