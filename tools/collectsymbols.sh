#!/bin/bash -e

TARGET=$1

CXX=${CXX:-g++}
CXXDIR=$(dirname ${CXX})
MACHINE=$(${CXX} -dumpmachine)
READELF=${MACHINE}-readelf
DUMPSYMS=${DUMPSYMS:-dump_syms}
ARCHIVE=${TARGET}_symbols.zip
WORKDIR=${TARGET}_symbols

# locate compiler specific readelf binary
[ "${CXXDIR}" != "." ] && READELF="${CXXDIR}/${READELF}"
which "${READELF}" >/dev/null || READELF=readelf
which "${READELF}" >/dev/null || { echo "Could not find readelf" >&2; exit 1; }

# build libary search path
library_path=$(${CXX} -print-search-dirs ${SYSROOT:+--sysroot=${SYSROOT}} | sed -ne 's/^libraries: =//p')
for rpath in $(${READELF} -d "${TARGET}" | sed -ne 's/.*(RPATH).*\[\(.*\)\]$/\1/p')
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
    (cd "${WORKDIR}"; find; zip "../${ARCHIVE}" "symbols/${target}/${version}/${target}.sym")
}

# create private working directory
rm -fr "${TARGET}_symbols"
mkdir -p "${TARGET}_symbols"

# dump symbols for target
dump_symbols "${TARGET}"

# dump symbols for shared libraries
${READELF} -d "${TARGET}" | sed -ne 's/.*(NEEDED).*\[\(.*\)\]$/\1/p' |
while read library
do
    library=$(find_library "${library}") && dump_symbols "${library}"
done
