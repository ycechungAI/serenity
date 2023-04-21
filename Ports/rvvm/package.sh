#!/usr/bin/env -S bash ../.port_include.sh
port='rvvm'
version='0.5'
archive_hash=3a1dbb91ad04f068078bc6c6c27cc5792eebc111907cb5a14bde158fe6e757c9
files="https://github.com/LekKit/RVVM/archive/v$version.tar.gz rvvm.tar.gz $archive_hash"
auth_type='sha256'
workdir="RVVM-$version"
depends=('sdl12-compat')

build() {
    run make OS=SerenityOS USE_SDL=1 USE_NET=1 GIT_COMMIT=76796ba all lib
}

install() {
    run make OS=SerenityOS USE_SDL=1 USE_NET=1 GIT_COMMIT=76796ba DESTDIR="${DESTDIR}" install
}
