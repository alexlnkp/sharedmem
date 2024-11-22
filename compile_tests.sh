#!/bin/bash

_TEST_FOLDER="test"
_OUT_FOLDER="out"

_PLATFORMS=("win" "linux")
_IMPLEMENTATIONS=("c" "cpp")

# start with windows
CC="x86_64-w64-mingw32-gcc"
_LINUX_CC="gcc"

_WIN_CPP_C="x86_64-w64-mingw32-g++"
_LIN_CPP_C="g++"

# C89 surprisingly works!

for platform in ${_PLATFORMS[@]}; do
    mkdir -p "${_OUT_FOLDER}/platformspecific/${platform}"
    if [[ $platform == "linux" ]]; then CC=${_LINUX_CC}; fi

    for filename in ${_TEST_FOLDER}/${platform}/*.c; do
        name=${filename##*/}
        base=${name%.c}
        ${CC} -std=c89 "${filename}" -o "${_OUT_FOLDER}/platformspecific/$platform/${base}" -Iinclude
    done

    for implementation in ${_IMPLEMENTATIONS[@]}; do
        mkdir -p "${_OUT_FOLDER}/crossplatform/${platform}/${implementation}"
        if [[ $implementation == "cpp" && $platform == "linux" ]]; then CC=${_LIN_CPP_C}; fi
        if [[ $implementation == "c" && $platform == "linux" ]]; then CC=${_LINUX_CC}; fi
        if [[ $implementation == "cpp" && $platform == "win" ]]; then CC=${_WIN_CPP_C}; fi
        for filename in ${_TEST_FOLDER}/crossplatform/${implementation}/*.${implementation}; do
            name=${filename##*/}
            base=${name%.${implementation}}
            ${CC} "${filename}" -o "${_OUT_FOLDER}/crossplatform/${platform}/${implementation}/${base}" -Iinclude
        done
    done
done
