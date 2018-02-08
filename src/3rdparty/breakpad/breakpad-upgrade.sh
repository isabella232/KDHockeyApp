#!/bin/bash -e

REPOSITORIES=(
https://chromium.googlesource.com/breakpad/breakpad
https://github.com/google/protobuf.git
https://chromium.googlesource.com/linux-syscall-support
https://chromium.googlesource.com/external/gyp
https://github.com/google/googletest.git
)

KDHOCKEYAPP_BREAKPAD=$(pwd)
KDHOCKEYAPP_BREAKPAD_README="${KDHOCKEYAPP_BREAKPAD}/README"
BREAKPAD_WORKDIR=upgrade

# $1: Git url
# $2: Target
clone() {
    echo "*** Upgrading ${1} @ ${2}"
    if [ -d "${2}" ]; then
        pushd "${2}" &> /dev/null
        git pull --rebase
        popd &> /dev/null
    else
        git clone "${1}" "${2}"
    fi
}

# $1: Git repo basename
breakPadTarget() {
    if [ "${1}" == "breakpad" ]; then
        echo "${KDHOCKEYAPP_BREAKPAD}/src"
    elif [ "${1}" == "protobuf" ]; then
        echo "${KDHOCKEYAPP_BREAKPAD}/src/src/third_party/protobuf/protobuf"
    elif [ "${1}" == "linux-syscall-support" ]; then
        echo "${KDHOCKEYAPP_BREAKPAD}/src/src/third_party/lss"
    elif [ "${1}" == "gyp" ]; then
        echo "${KDHOCKEYAPP_BREAKPAD}/src/src/tools/gyp"
    elif [ "${1}" == "googletest" ]; then
        echo "${KDHOCKEYAPP_BREAKPAD}/src/src/testing"
    fi
}

# $1: Source
sync() {
    local target=$(breakPadTarget "${1}")
    echo "*** Syncing ${1} @ ${target}"
    rm -fR "${target}"
    mkdir -p "${target}"
    git -C "${1}" archive master | tar -x -C "${target}/"
}

echo "This copy of Google Breakpad was fetched on $(date "+%Y-%m-%d")," > "${KDHOCKEYAPP_BREAKPAD_README}"
echo "and is based upon the following git checkouts:" >> "${KDHOCKEYAPP_BREAKPAD_README}"
echo "" >> "${KDHOCKEYAPP_BREAKPAD_README}"

mkdir -p "${BREAKPAD_WORKDIR}"
pushd "${BREAKPAD_WORKDIR}" &> /dev/null
for repository in ${REPOSITORIES[@]}; do
    repository_target=$(basename -s .git "${repository}")
    clone "${repository}" "${repository_target}"
    sync "${repository_target}"

    echo "$(realpath --relative-to="${KDHOCKEYAPP_BREAKPAD}" "$(breakPadTarget "${repository_target}")")" >> "${KDHOCKEYAPP_BREAKPAD_README}"
    echo "    ${repository}" >> "${KDHOCKEYAPP_BREAKPAD_README}"
    echo "    commit $(git -C "${repository_target}" rev-parse HEAD)" >> "${KDHOCKEYAPP_BREAKPAD_README}"
    echo "" >> "${KDHOCKEYAPP_BREAKPAD_README}"
done
popd &> /dev/null
rm -fR "${BREAKPAD_WORKDIR}"
