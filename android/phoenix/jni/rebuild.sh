#!/bin/sh

ndk-build || exit 0

cp -aR ../libs ../../../../phoenix/
echo "phoenix updated"