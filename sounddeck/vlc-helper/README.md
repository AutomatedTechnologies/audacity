# sounddeck-vlc-helper — SD-T4 libVLC helper prototype

Added by Detroit Automation Academy, 2026-11-09 (SoundDeck tranche
SD-T4 / D3 combine). This directory is **not** part of upstream Audacity;
it is new Academy code in the fork, licensed GPLv3 (the combined-work
license from the Gate 0 memo, `files/02` in the SoundDeck goal record).

## What this is

The build-level prototype of SoundDeck's media-engine helper process:
a small C program (`sounddeck-vlc-helper.c`) written against the public
libVLC API (`libvlc_*`) and nothing else. Gate 0 Condition 2: SoundDeck
uses **libVLC only** — no VLC application code is forked, copied, or
linked here.

It proves, in CI, four things about the pinned combination — Audacity
3.7.9 line (this fork branch) × libVLC from VLC **3.0.24**
(commit `6de05adcbaf2e8b85fe86aad4169393098628119`, SoundDeck `files/03`
pin):

1. `--version` — the built libVLC loads in-process and reports its own
   version string (`libvlc_get_version()`), asserted to contain 3.0.24.
2. `--probe-play <file>` — a media file plays to completion
   (`libvlc_Ended`) through libVLC with dummy audio/video outputs.
3. `--probe-transcode <in> <out.wav>` — the stream-output chain
   (`#transcode{acodec=s16l,channels=2,samplerate=44100}:std{access=file,mux=wav}`)
   converts media to a 44.1 kHz stereo 16-bit WAV, verified afterwards
   with Python's `wave` module.
4. `--probe-extract <in.mp4> <out.wav>` — the same chain with
   `vcodec=none` extracts only the audio of an A/V file into WAV.

Plus `--serve`, the line-protocol skeleton the real helper will speak
(LOAD / PLAY / PAUSE / SEEK / TIME / STOP / QUIT over stdin/stdout,
every reply prefixed `OK` or `ERR`) — it genuinely drives one
`libvlc_media_player_t`; it is not a mock.

Every verdict is read from libVLC runtime state or from the output
files themselves — never from build configuration or cache text
(the SD-T3 lesson).

## What this is not

- Not the shipped helper. The production helper's process boundary,
  sandboxing, module-set trimming, and Audacity-side UI integration are
  SD-T4 design / SD-T5 packaging work, not this prototype.
- Not a High Sierra verification. CI builds and runs on a current
  macOS runner against a 10.13 deployment target; the LC_VERSION_MIN
  floor evidence is captured in the run's binary-evidence file, and
  the on-rig (MacBook Air, 10.13) run remains a rig item.
- Not a publication artifact. Nothing here enters the Distro catalog.

## How CI builds it

`.github/workflows/sounddeck-t4-libvlc.yml` (push-triggered on branch
`sounddeck-t4-libvlc`, macos-15, 300-minute timeout):

1. Checks out this fork branch and records a build manifest (runner,
   Xcode, branch base, both pins).
2. Checks out `videolan/vlc` at tag `3.0.24` into `vlc-3.0.24/`.
3. Pins the VLC macOS build floor: `MINIMAL_OSX_VERSION` in
   `extras/package/macosx/build.sh` is rewritten `10.7` → `10.13`
   (build-environment pin only; no VLC source file is modified and the
   change is recorded in the manifest).
4. Builds VLC with upstream's own script
   (`sh ../vlc-3.0.24/extras/package/macosx/build.sh` from `vlc-build/`),
   which uses the prebuilt contribs and finishes with `make VLC.app`.
5. Compiles the helper with
   `clang -arch x86_64 -mmacosx-version-min=10.13` against the built
   `VLC.app/Contents/MacOS/lib/libvlc.dylib` and the pinned tree's
   `include/` headers, with an rpath to that lib directory.
6. Generates fixtures (a 440 Hz / 2 s WAV via Python; a 2 s H.264/AAC
   MP4 via ffmpeg — fixture tooling only, not part of the artifact),
   runs the four proofs, and captures binary evidence (`file`,
   `lipo -info`, `otool -L`, `otool -l` LC_VERSION_MIN_MACOSX, and a
   full file inventory of the built `lib/` and `plugins/` directories
   for the Gate 0 Condition 6 inventory).
7. Uploads one artifact: helper binary, build manifest, VLC build log,
   proof logs, binary evidence, and output hashes.
