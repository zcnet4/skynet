#!/bin/bash

WORKDIR=`pwd`/..
BUILDTYPE=Release
#BUILDTYPE=Debug 
#DST=skynet-${BUILDTYPE}-0.0
DST=skynet-0.0

# gen
python ${WORKDIR}/build/gyp.py -D"component=shared_library" skynet.gyp
# build
BUILDTYPE=${BUILDTYPE} make -j 4 -C ${WORKDIR}/build_linux

pushd .
cd ${WORKDIR}/build_linux
rm -rf ${DST}
mkdir ${DST}

mkdir ${DST}/bin
# 复制第三方so
#cp ${WORKDIR}/3rd/libstdc++.so.6.0.21 ${DST}/bin

# 复制
cp ${BUILDTYPE}/skynet ${DST}/bin
cp ${BUILDTYPE}/*.so ${DST}/bin
cp -r ${BUILDTYPE}/lib.target ${DST}/bin
cp ${BUILDTYPE}/lib.target/memory.so ${DST}/bin
cp ${BUILDTYPE}/lib.target/skynet.so ${DST}/bin
cp ${BUILDTYPE}/lib.target/protobuf.so ${DST}/bin

# 复制skynet系统脚本
mkdir ${DST}/skynet_script
cp -r ${WORKDIR}/skynet/lualib ${DST}/skynet_script/lualib
cp -r ${WORKDIR}/skynet/service ${DST}/skynet_script/service
#cp -r ${WORKDIR}/skynet_script/yx ${DST}/skynet_script/yx

tar -czf ${DST}.tar.gz ${DST}
cp ${DST}.tar.gz ${WORKDIR}

popd

