#!/bin/bash -e

TARGET_FILENAME="$1"
TARGET_PATH=$(cd $(dirname "${TARGET_FILENAME}"); pwd)
TARGET=$(basename "${TARGET_FILENAME}")

CXX="${CXX:-g++}"
CXXDIR=$(dirname "${CXX}")
MACHINE=$("${CXX}" -dumpmachine)
READELF="${MACHINE}-readelf"
DUMPSYMS="${DUMPSYMS:-dump_syms}"
ARCHIVE="${TARGET_PATH}/${TARGET}_symbols.zip"
WORKDIR="${TARGET}_symbols"

# locate compiler specific readelf binary
[ "${CXXDIR}" != "." ] && READELF="${CXXDIR}/${READELF}"
which "${READELF}" >/dev/null || READELF=readelf
which "${READELF}" >/dev/null || { echo "Could not find readelf" >&2; exit 1; }

# build libary search path
library_path=$(${CXX} -print-search-dirs ${SYSROOT:+--sysroot=${SYSROOT}} | sed -ne 's/^libraries: =//p')
for rpath in $(${READELF} -d "${TARGET_FILENAME}" | sed -ne 's/.*(RPATH).*\[\(.*\)\]$/\1/p')
do
    library_path="${rpath}:${library_path}"
done

# finds a library in search path
find_library() {
    local IFS=:

    for dir in ${library_path}
    do
        path="${dir}/${library}"
        test -x ${path} || continue
        echo ${path}
        return 0
    done

    return 1
}

# dumps symbols of a binary or library
dump_symbols() {
    local target=$(basename $(realpath "$1"))
    local symfile="${WORKDIR}/${target}.sym"

    ${DUMPSYMS} "$1" > "${symfile}"

    local version=$(sed -ne 's/^MODULE \S\+ \S\+ \(\S\+\) .*/\1/p' ${symfile})

    mkdir -p "${WORKDIR}/symbols/${target}/${version}"
    mv "${symfile}" "${WORKDIR}/symbols/${target}/${version}"
    (cd "${WORKDIR}"; find; zip "${ARCHIVE}" "symbols/${target}/${version}/${target}.sym")
}

# create private working directory
rm -fr "${WORKDIR}"
mkdir -p "${WORKDIR}"

# dump symbols for target
dump_symbols "${TARGET_FILENAME}"

# dump symbols for shared libraries
${READELF} -d "${TARGET_FILENAME}" | sed -ne 's/.*(NEEDED).*\[\(.*\)\]$/\1/p' |
while read library
do
    library=$(find_library "${library}") && dump_symbols "${library}"
done
