#!/bin/bash

# Date
DATE=$(date +"%Y%m%d")

# Lookup the last commit ID
VERSION="$(git rev-parse --short HEAD)"

# Check if any uncommitted changes in tracked files
if [ -n "$(git status --untracked-files=no --porcelain)" ]; then
  VERSION="${VERSION}?"
fi

echo "const char szBuild[] = \"Build ${DATE} ${VERSION}\";" > build.h.tmp

# Only overwrite gitversion.h if the content has changed
# This reduced unnecessary re-compilation
rsync -checksum build.h.tmp build.h

rm -f build.h.tmp
