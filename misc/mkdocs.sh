#!/bin/bash -x
################################################################################
# misc/mkdocs.sh
#
#
# All rights reserved. Published under the MIT License in the LICENSE file.
################################################################################

# Script to build and install python module and then to rebuild docs

set -e

pushd build/python
make -j8
cp \
    cobs_index.cpython-36m-x86_64-linux-gnu.so \
    ~/.local/lib64/python3.6/site-packages/
popd

pushd python/docs
rm -rf _build _generated
make html
popd

################################################################################
