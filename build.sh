#!/bin/sh
# Reproducible offline build of adlibgold.sys under Wine (call/0007, call/0008, call/0009).
# Consumes a pinned DDK+VC6 deps-bundle (Microsoft-licensed; hosted where the licence
# permits) and produces a byte-identical adlibgold.sys. No network at build time.
#   BUNDLE=<path-or-url-to-bundle.tar.gz> ./build.sh
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
# Keep the Wine prefix OUT of the worktree. Wine fills a prefix with shell-folder
# symlinks (My Documents -> $HOME, My Videos -> My Documents, ...) that escape the tree
# and form cycles; an in-worktree prefix traps any recursive worktree walk in that cycle.
PFX="${WINEPREFIX:-${XDG_CACHE_HOME:-$HOME/.cache}/adlibgold-winebuild}"; export WINEPREFIX="$PFX" WINEDEBUG=-all
BUNDLE="${BUNDLE:?set BUNDLE to the deps-bundle tar.gz}"
DRVC="$PFX/drive_c"
mkdir -p "$PFX"   # wineboot creates the prefix leaf but not a missing parent (~/.cache)
wineboot --init >/dev/null 2>&1 || true
mkdir -p "$DRVC/temp" "$DRVC/drv"
# stage the pinned toolchain (offline) + the driver sources
tar -C "$DRVC" -xzf "$BUNDLE"
cp "$HERE"/*.cpp "$HERE"/*.h "$DRVC/drv/"
cp "$DRVC/tc/src/stdunk.cpp" "$DRVC/drv/"   # DDK helper, from the bundle
# DBG=1 makes a checked build: turn on the DDK ASSERT / PAGED_CODE / _DbgPrintF
# facilities (all gated on DBG) and raise the print threshold above the default TERSE so
# the driver's DEBUGLVL_VERBOSE flow traces actually appear (ksdebug.h). Unset (the
# default) builds the free, deployable, byte-reproducible artifact, and this DEFS string
# is unchanged. A checked build is a separate diagnostic artifact (adlibgold.chk.sys) with
# its own hash; it is not the recorded deploy artifact and is not re-pinned.
if [ -n "${DBG:-}" ]; then DBGDEF='/DDBG=1 /DDEBUG_LEVEL=DEBUGLVL_VERBOSE'; else DBGDEF='/DNDEBUG'; fi
DEFS="/DWIN32=100 /D_WIN32_WINNT=0x0500 /DWINVER=0x0500 /D_WIN32_IE=0x0400 /D_X86_=1 /Di386=1 /DSTD_CALL /DCONDITION_HANDLING=1 /DNT_UP=1 /DWINNT=1 /DDEVL=1 $DBGDEF /DUNICODE /D_UNICODE"
cc() { wine cmd /c "set TMP=C:\\temp&&set TEMP=C:\\temp&&set INCLUDE=C:\\tc\\ddkinc&&C:\\tc\\bin\\CL.EXE /nologo /c /Zel /Gz /Gy /Gm- /GF /W3 /Od $DEFS /FoC:\\drv\\$1.obj C:\\drv\\$1.cpp" ; }
for s in common adapter algtopo algwave fmsynth midi stdunk; do cc "$s"; done
wine cmd /c "C:\\tc\\bin\\LIB.EXE /nologo /out:C:\\tc\\ddklib\\stdunk.lib C:\\drv\\stdunk.obj" >/dev/null
wine cmd /c "set LIB=C:\\tc\\ddklib;C:\\tc\\vclib&&C:\\tc\\bin\\LINK.EXE /nologo /out:C:\\drv\\adlibgold.sys /machine:ix86 /subsystem:native,5.00 /driver /base:0x10000 /align:0x80 /entry:DriverEntry@8 /merge:_PAGE=PAGE /merge:_TEXT=.text /merge:.rdata=.text /nodefaultlib /release /incremental:no C:\\drv\\common.obj C:\\drv\\adapter.obj C:\\drv\\algtopo.obj C:\\drv\\algwave.obj C:\\drv\\fmsynth.obj C:\\drv\\midi.obj portcls.lib libcntpr.lib ntoskrnl.lib hal.lib stdunk.lib" >/dev/null
python3 "$HERE/pe_normalize.py" "$DRVC/drv/adlibgold.sys"
OUT="adlibgold.sys"; [ -n "${DBG:-}" ] && OUT="adlibgold.chk.sys"
cp "$DRVC/drv/adlibgold.sys" "$HERE/$OUT"
echo "$OUT sha256: $(sha256sum "$HERE/$OUT" | cut -d' ' -f1)"
