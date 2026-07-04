# tools

Standalone utilities that support the driver but are not part of it.

## testtone

`testtone.c` writes a mono 8-bit PCM WAV containing a square-wave test tone. The
driver plays PCM that applications stream to it; this tone is one such application
artifact, kept out of the driver on purpose. Play the generated WAV through the
installed driver to confirm the 8-bit PCM path is audible on the card (the
`call/0005` acceptance).

Build (either toolchain):

    cl testtone.c                      # Win2K DDK / VC6, native Windows
    cc -I. -o testtone testtone.c      # host

Run (defaults: `testtone.wav`, 22050 Hz sample rate, 440 Hz tone, 1 second):

    testtone [out.wav] [rate] [freq] [seconds]

The default 22050 Hz is one of the four hardware rates, so it plays without the
system resampler in the path. On the Windows 98SE test machine, generate the WAV
and play it (Media Player, Sound Recorder, or `sndPlaySound`); audible output is
the hardware acceptance for the 8-bit tone.

The tone and header generation live in `testtone.h` as pure, integer-only
functions, unit-tested by `tests/testtone_test.c` in the Tests CI lane.
