#!/bin/bash

python ./configure --dest-os=ios --dest-cpu=arm64 --enable-static --engine=jsc --openssl-no-asm --without-etw --without-snapshot --without-perfctr --with-intl=none
make -j$(getconf _NPROCESSORS_ONLN)

xcodebuild build -project tools/NodeIOS/NodeIOS.xcodeproj -target "NodeIOS" -configuration Release -arch arm64 -sdk "iphoneos"
