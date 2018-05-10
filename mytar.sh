#!/bin/bash

make clean
make
make clean
cd ..
mv bdcapdir bdcapcli-1.0.1
tar -Jcf bdcapcli-1.0.1.tar.xz bdcapcli-1.0.1
cp bdcapcli-1.0.1.tar.xz /router/lede/dl/ 
rm -f bdcapcli-1.0.1.tar.xz
mv bdcapcli-1.0.1 bdcapdir
cd /router/lede
make package/bdcapcli/clean V=99
make package/bdcapcli/compile  -j9 V=99
if [[ $? -ne 0 ]]
then
	echo "fail!!!"
else
	scp /router/lede/bin/packages/mips_24kc/packages/bdcapcli_1.0.1-1_mips_24kc.ipk  root@192.168.3.105:~  
	echo "chengggong"
fi
