#!/bin/bash

WORKDIR=`pwd`/..


python ${WORKDIR}/build/gyp.py -D"component=shared_library" skynet/skynet.gyp
BUILDTYPE=Release make -j 4 -C ${WORKDIR}/build_linux
#BUILDTYPE=Debug make -j 4 -C ${WORKDIR}/build_linux

#cp ${WORKDIR}/3rd/libstdc++.so.6.0.21 ${WORKDIR}/build_linux/${BUILDTYPE}/libstdc++.so.6.0.21
#cp ${WORKDIR}/3rd/glog/lib/linux/libglog.so.0 ${WORKDIR}/build_linux/${BUILDTYPE}/libglog.so.0
#cp ${WORKDIR}/3rd/gperftools-2.5/lib/linux/libunwind.so.8 ${WORKDIR}/build_linux/${BUILDTYPE}/libunwind.so.8
#cp ${WORKDIR}/3rd/gperftools-2.5/.libs/libtcmalloc_and_profiler.so.4 ${WORKDIR}/build_linux/${BUILDTYPE}/libtcmalloc_and_profiler.so.4

