#!/bin/sh
# Run TLC on one spec, exactly as the Timing CI lane does: the pinned tla2tools
# v1.8.0 (checksum-verified, cached under the user cache directory), the spec's
# sibling .cfg, and -deadlock because the bounded models terminate by design.
# This makes every spec verify line re-runnable and re-derivable off-CI
# (plan/0016#tlc-harness).
set -eu
cd "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
spec=${1:?usage: tlc.sh <SpecName>}
jar="${XDG_CACHE_HOME:-$HOME/.cache}/tla2tools-v1.8.0.jar"
sha=9e27b5e19a69ae1f56aabf8403a6ed5598dbfa6e638908e5278ac39736c1543d
if [ ! -f "$jar" ] || [ "$(sha256sum "$jar" | cut -d' ' -f1)" != "$sha" ]; then
    curl -sSL -o "$jar" \
        https://github.com/tlaplus/tlaplus/releases/download/v1.8.0/tla2tools.jar
    echo "$sha  $jar" | sha256sum -c -
fi
exec java -cp "$jar" tlc2.TLC -deadlock -config "$spec.cfg" "$spec.tla"
