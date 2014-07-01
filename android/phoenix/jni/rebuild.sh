#!/bin/sh

ndk-build || exit 0

LIBS=../../../../phoenix/libs/

cp -aR ../libs/armeabi-v7a $LIBS
cp -aR ../libs/x86 $LIBS

echo "phoenix updated"