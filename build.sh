#!/bin/sh
# Reproducible offline build of adlibgold.sys under Wine (call/0007, call/0008, call/0009).
# Consumes a pinned DDK+VC6 deps-bundle (Microsoft-licensed; hosted where the licence
# permits) and produces a byte-identical adlibgold.sys. No network at build time.
#   BUNDLE=<path-or-url-to-bundle.tar.gz> ./build.sh
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
PFX="${WINEPREFIX:-$HERE/.winebuild}"; export WINEPREFIX="$PFX" WINEDEBUG=-all
BUNDLE="${BUNDLE:?set BUNDLE to the deps-bundle tar.gz}"
DRVC="$PFX/drive_c"
wineboot --init >/dev/null 2>&1 || true
mkdir -p "$DRVC/temp" "$DRVC/drv"
# stage the pinned toolchain (offline) + the driver sources
tar -C "$DRVC" -xzf "$BUNDLE"
cp "$HERE"/*.cpp "$HERE"/*.h "$DRVC/drv/"
cp "$DRVC/tc/src/stdunk.cpp" "$DRVC/drv/"   # DDK helper, from the bundle
DEFS='/DWIN32=100 /D_WIN32_WINNT=0x0500 /DWINVER=0x0500 /D_WIN32_IE=0x0400 /D_X86_=1 /Di386=1 /DSTD_CALL /DCONDITION_HANDLING=1 /DNT_UP=1 /DWINNT=1 /DDEVL=1 /DNDEBUG /DUNICODE /D_UNICODE'
cc() { wine cmd /c "set TMP=C:\\temp&&set TEMP=C:\\temp&&set INCLUDE=C:\\tc\\ddkinc&&C:\\tc\\bin\\CL.EXE /nologo /c /Zel /Gz /Gy /Gm- /GF /W3 /Od $DEFS /FoC:\\drv\\$1.obj C:\\drv\\$1.cpp" ; }
for s in common adapter algtopo algwave fmsynth midi stdunk; do cc "$s"; done
wine cmd /c "C:\\tc\\bin\\LIB.EXE /nologo /out:C:\\tc\\ddklib\\stdunk.lib C:\\drv\\stdunk.obj" >/dev/null
wine cmd /c "set LIB=C:\\tc\\ddklib;C:\\tc\\vclib&&C:\\tc\\bin\\LINK.EXE /nologo /out:C:\\drv\\adlibgold.sys /machine:ix86 /subsystem:native,5.00 /driver /base:0x10000 /align:0x80 /entry:DriverEntry@8 /merge:_PAGE=PAGE /merge:_TEXT=.text /merge:.rdata=.text /nodefaultlib /release /incremental:no C:\\drv\\common.obj C:\\drv\\adapter.obj C:\\drv\\algtopo.obj C:\\drv\\algwave.obj C:\\drv\\fmsynth.obj C:\\drv\\midi.obj portcls.lib libcntpr.lib ntoskrnl.lib hal.lib stdunk.lib" >/dev/null
python3 "$HERE/pe_normalize.py" "$DRVC/drv/adlibgold.sys"
cp "$DRVC/drv/adlibgold.sys" "$HERE/adlibgold.sys"
echo "adlibgold.sys sha256: $(sha256sum "$HERE/adlibgold.sys" | cut -d' ' -f1)"
