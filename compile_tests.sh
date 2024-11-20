#!/bin/bash

_TEST_FOLDER="test"
_OUT_FOLDER="out"

_PLATFORMS=("win" "linux")

# start with windows
CC="x86_64-w64-mingw32-gcc"
_LINUX_CC="gcc"

for platform in ${_PLATFORMS[@]}; do
    mkdir -p "${_OUT_FOLDER}/${platform}"
    if [[ $platform == "linux" ]]; then CC=${_LINUX_CC}; fi

    for filename in ${_TEST_FOLDER}/${platform}/*.c; do
        name=${filename##*/}
        base=${name%.c}
        ${CC} "${filename}" -o "${_OUT_FOLDER}/$platform/${base}" -Iinclude
    done
done
