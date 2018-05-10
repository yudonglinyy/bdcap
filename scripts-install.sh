#!/bin/bash

cd /router/lede
#./scripts/feeds update mypackages
#./scripts/feeds update mypackages
#make menuconfig
make package/bdcapcli/clean V=99
make package/bdcapdir/compile -j8 V=s
