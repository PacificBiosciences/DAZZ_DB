#!/bin/bash -e
type module >& /dev/null || source /mnt/software/Modules/current/init/bash

module load gcc
module load git
module load meson
module load ninja
module load ccache

set -vex

export CCACHE_COMPILERCHECK='%compiler% -dumpversion'

case "${bamboo_planRepository_branchName}" in
  develop|master)
    export PREFIX_ARG="/mnt/software/d/dazzdb/${bamboo_planRepository_branchName}"
    export BUILD_NUMBER="${bamboo_globalBuildNumber:-0}"
    ;;
  *)
    export PREFIX_ARG=/PREFIX
    export BUILD_NUMBER="0"
    ;;
esac

# rm -rf ./build
meson --buildtype=release --strip --libdir=lib --prefix="${PREFIX_ARG}" -Dtests=false --wrap-mode nofallback ./build .

TERM='dumb' ninja -C ./build -v

DESTDIR="$(pwd)/DESTDIR"
rm -rf "${DESTDIR}"
TERM='dumb' DESTDIR="${DESTDIR}" ninja -C ./build -v install

# TODO: Add test here.

rm -rf "${DESTDIR}"

case "${bamboo_planRepository_branchName}" in
  develop|master)
    DESTDIR=
    TERM='dumb' DESTDIR="${DESTDIR}" ninja -C ./build -v install
    chmod -R a+rwx "${DESTDIR}${PREFIX_ARG}"/*
    module load dazzdb/${bamboo_planRepository_branchName}
    ldd -r $(which DBshow)
    ;;
esac
