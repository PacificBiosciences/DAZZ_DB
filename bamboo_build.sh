#!/bin/bash -xe
type module >& /dev/null || source /mnt/software/Modules/current/init/bash

set -vex

module load gcc/4.9.2
module load git/2.8.3
module load ccache

rm -rf build
mkdir -p build/lib build/bin build/include
cd DAZZ_DB
make clean
make -j 
make PREFIX=$PWD/../build install
cp *.h ../build/include
cd -
cd build
tar zcf DAZZ_DB-SNAPSHOT.tgz bin lib include
NEXUS_BASEURL=http://ossnexus.pacificbiosciences.com/repository
NEXUS_URL=$NEXUS_BASEURL/unsupported/gcc-4.9.2
curl -v -n --upload-file DAZZ_DB-SNAPSHOT.tgz $NEXUS_URL/DAZZ_DB-SNAPSHOT.tgz
cd -
