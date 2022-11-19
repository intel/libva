#!/usr/bin/env bash

set -euo pipefail

modified_lines=$(git status --short -uno | wc -l)
(( modified_lines == 0 )) && exit 0

echo >&2
echo >&2 "ERROR: Style changes detected"
echo >&2

git diff

echo >&2
echo >&2 "ERROR: Squash the above changes as needed"
echo >&2

exit 1
