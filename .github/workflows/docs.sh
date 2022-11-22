#!/usr/bin/env bash

set -euo pipefail

diff -q $1 $2 && exit 0

echo >&2
echo >&2 "ERROR: Documentation issues detected"
echo >&2

diff -u $1 $2

echo >&2
echo >&2 "ERROR: Documentation issues detected"
echo >&2

exit 1
