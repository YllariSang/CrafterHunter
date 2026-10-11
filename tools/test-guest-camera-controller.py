#!/usr/bin/env python3
"""Headless controller tests with fake installations; never accesses live games."""
import importlib.util
import sys
import tempfile
from pathlib import Path

spec = importlib.util.spec_from_file_location("guest_controller", Path(__file__).with_name("control-guest-camera.py"))
controller = importlib.util.module_from_spec(spec)
spec.loader.exec_module(controller)

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    mc, mhw, channel = root / "mc", root / "mhw", root / "channel"
    render = mhw / "nativePC/plugins/CSharp/CrafterHunter/render"
    render.mkdir(parents=True)
    (mc / "crafterhunter").mkdir(parents=True)
    channel.mkdir()
    (render.parent / "CrafterHunter.Render.dll").touch()
    world = mc / "crafterhunter/world-stream.enabled"
    world.write_text("0\n")
    (render / "world-upload.enabled").write_text("16\n")
    for name in ("world.frame", "player.pose", "player.asset", "camera.pose"):
        (channel / name).write_bytes(b"unchanged")
    disarms = []
    controller.disarm_before_resolving = lambda path: disarms.append(path)
    controller.fresh_resolution = lambda pid: "on 1 2 3,4 5,6 7\n" if pid == 999 else None
    controller.secrets.randbits = lambda bits: 12345
    def run(action, *extra):
        sys.argv = ["controller", action, "--minecraft", str(mc), "--mhw", str(mhw),
                    "--channel", str(channel), *extra]
        controller.main()
    run("start", "--pid", "999")
    assert disarms == [render]
    assert (render / "exclusive-input.request").read_text() == "on 1 2 3,4 5,6 7\n"
    assert (render / "guest-camera.request").read_text() == "on 12345\n"
    assert (mc / "crafterhunter/guest-control.enabled").read_text() == "12345\n"
    assert world.read_text() == "0\n" and (render / "world-upload.enabled").read_text() == "16\n"
    run("stop")
    assert not (mc / "crafterhunter/guest-control.enabled").exists()
    assert (render / "guest-camera.request").read_text() == "off\n"
    assert (render / "exclusive-input.request").read_text() == "off\n"
    for name in ("world.frame", "player.pose", "player.asset", "camera.pose"):
        assert (channel / name).read_bytes() == b"unchanged"
    try:
        run("start")
        raise AssertionError("missing PID accepted")
    except SystemExit as error:
        assert error.code == 2
print("guest camera controller atomic commands/preserved channels/cadence passed")
