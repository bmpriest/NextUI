# Miyoo A30 test records

Build the Phase 0 probe from the NextUI repository root:

```sh
make -f makefile.toolchain PLATFORM=my282 diagnostic
```

Copy `workspace/my282/diagnostics/build/my282-probe.elf` and
`workspace/my282/diagnostics/run-on-spruce.sh` to
`/mnt/SDCARD/NextUI-my282`. The wrapper temporarily pauses Spruce MainUI,
continuously presents the test for ten seconds, and resumes MainUI on exit:

```sh
cd /mnt/SDCARD/NextUI-my282
chmod +x my282-probe.elf run-on-spruce.sh
./run-on-spruce.sh
```

First retry without the `SDL_VIDEODRIVER` override if `mali` is not listed by
the probe. Do not replace system libraries during this test.

The expected display for ten seconds is:

```text
+-----------+-----------+
|    RED    |   GREEN   |
+-----------+-----------+
|   BLUE    |  YELLOW   |
+-----------+-----------+
```

This establishes landscape orientation as well as successful buffer swaps.
The text report must show:

- the actual framebuffer geometry;
- an SDL drawable of 480x640 on the portrait-mounted panel;
- an OpenGL ES 2.x context from the Mali-400;
- a complete `GL_RGBA` 640x480 framebuffer object;
- input device names and readable battery/brightness paths.

Copy the output into a dated Markdown file in this directory and add the
firmware version, NextUI commit, whether the quadrant test matched, and how the
probe was launched.
