#!/bin/sh
# Builds the project - scripts/build/build.sh, with the build's other scripts; here at the root to be found first.
# usage: sh build.sh [install]
exec sh "$(dirname "$0")/scripts/build/build.sh" "$@"
