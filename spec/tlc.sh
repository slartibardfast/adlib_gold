#!/bin/sh
# Run TLC on one spec, exactly as the Timing CI lane does: the tla2tools v1.8.0
# jar vendored beside this script (checksum-verified, so the pin is immutable and
# the lane is hermetic with no network fetch), the spec's sibling .cfg, and
# -deadlock because the bounded models terminate by design. This makes every spec
# verify line re-runnable and re-derivable off-CI (plan/0016#tlc-harness).
set -eu
cd "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
spec=${1:?usage: tlc.sh <SpecName>}
jar=tla2tools.jar
sha=58d44845a37a8d776deaf8cf3a623213b59d311bc0ec287bcdfbe148dd11bb3d
echo "$sha  $jar" | sha256sum -c -
exec java -cp "$jar" tlc2.TLC -deadlock -config "$spec.cfg" "$spec.tla"
