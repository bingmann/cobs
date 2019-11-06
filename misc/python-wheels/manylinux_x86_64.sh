#!/bin/bash -x

# run script inside a docker with
#
# docker pull quay.io/pypa/manylinux2010_x86_64
# docker run --rm -it -v /home/tb/cobs:/src quay.io/pypa/manylinux2010_x86_64

set -e

yum update -y
yum install -y wget zlib-devel

# build newer cmake
rm -rf /tmp/cmake
mkdir /tmp/cmake
cd /tmp/cmake

wget https://cmake.org/files/v3.9/cmake-3.9.2.tar.gz
tar -zxf cmake-3.9.2.tar.gz
cd cmake-3.9.2

./bootstrap --prefix=/usr/local
make -j4
make install

# build many wheels
rm -rf /src/build
mkdir -p /src/dist
for p in /opt/python/*; do
    $p/bin/pip wheel --verbose /src -w /src/dist
done

# repair wheels
for w in /src/dist/cobs_index-*whl; do
    auditwheel repair $w -w /src/dist/
    rm -f $w
done
