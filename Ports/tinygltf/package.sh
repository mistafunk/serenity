#!/usr/bin/env -S bash ../.port_include.sh
port='tinygltf'
version='2.8.19'
workdir="tinygltf-${version}"
files=(
    "https://github.com/syoyo/tinygltf/archive/refs/tags/v${version}.tar.gz#9e3f6206c6e922c7482e1b4612b62c5cddb7e053b6690fa20edfa5d97805053b"
)
useconfigure='false'
configopts=()

build() {
    true
}

install() {
    SRC_DIR="${PORT_BUILD_DIR}/${port}-${version}"
    DST_DIR="${DESTDIR}/usr/local/include/tinygltf/"
    mkdir -p "${DST_DIR}"
    cp "${SRC_DIR}/stb_image.h" "${SRC_DIR}/stb_image_write.h" "${SRC_DIR}/json.hpp" "${SRC_DIR}/tiny_gltf.h" "${DST_DIR}"
}
