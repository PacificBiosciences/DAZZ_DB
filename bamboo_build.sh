#!/bin/bash -xe
THISDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
type module >& /dev/null || source /mnt/software/Modules/current/init/bash

set -vex

PREFIX=$PWD/build
cd ${THISDIR}
module load gcc/6.4.0
module load git/2.8.3
module load ccache
export CCACHE_COMPILERCHECK='%compiler% -dumpversion'

export CPPFLAGS=-D_GNU_SOURCE
DEFAULT_PREFIX=$PWD/build
PREFIX=${PREFIX:-${DEFAULT_PREFIX}}
rm -rf ${PREFIX}
mkdir -p ${PREFIX}/lib ${PREFIX}/bin ${PREFIX}/include
make clean
make -j
make PREFIX=${PREFIX} install
cp *.h ${PREFIX}/include

cd -
if [[ $bamboo_planRepository_branchName == "develop" ]]; then
  cd ${PREFIX}
  tar zcf DAZZ_DB-SNAPSHOT.tgz bin lib include
  NEXUS_BASEURL=http://ossnexus.pacificbiosciences.com/repository
  NEXUS_URL=$NEXUS_BASEURL/unsupported/gcc-6.4.0
  curl -v -n --upload-file DAZZ_DB-SNAPSHOT.tgz $NEXUS_URL/DAZZ_DB-SNAPSHOT.tgz
  cd -
fi
