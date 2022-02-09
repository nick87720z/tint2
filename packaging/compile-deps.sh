#!/bin/sh

set -x -e

require_cmds() {
    for c in "$@"; do
        type "$c" >/dev/null 2>&1 && continue ||:
        failed_deps=1
        printf '%s\n' "$c is required, but not found."
    done
    [ -z "${failed_deps}" ] || exit 1
}

set_modver() {
    varname="$1"
    pkgname="$2"
    eval "$varname=$(pkg-config --modversion $pkgname)"
    if [ $? -ne 0 ]; then
        failed_deps=1
    fi
}

two_digits() {
    echo "$@" | cut -d. -f 1-2
}

PS4='\e[99;33m${BASH_SOURCE}:$LINENO\e[0m '
export PKG_CONFIG_PATH=""
DEPS=$(realpath './tint2-deps')

#~ sudo apt-get install ninja-build python2.7 wget
#~ sudo apt-get build-dep libx11-6 libxrender1 libcairo2 libglib2.0-0 libpango-1.0-0 libimlib2 librsvg2-2

require_cmds ninja python2 wget pkg-config

set_modver X11          x11
set_modver XRENDER      xrender
set_modver XCOMPOSITE   xcomposite
set_modver XDAMAGE      xdamage
set_modver XEXT         xext
set_modver XRANDR       xrandr
set_modver XINERAMA     xinerama
set_modver IMLIB        imlib2
set_modver GLIB         glib-2.0
set_modver CAIRO        cairo
set_modver PANGO        pango
set_modver PIXBUF       gdk-pixbuf-2.0
set_modver RSVG         librsvg-2.0
[ "${failed_deps}" ] && exit 1 ||:

mkdir -p $DEPS/src

download_and_build() {
    URL="$1"
    shift
    ARCHIVE="$(basename "$URL")"
    NAME="$(echo "$ARCHIVE" | sed 's/\.tar.*$//g')"

    cd "$DEPS/src"
    [ -f "$ARCHIVE" ] || wget "$URL" -O "$ARCHIVE"
    rm -rf "$NAME"
    tar xf "$ARCHIVE"
    cd "$NAME"

    export PKG_CONFIG_PATH="$DEPS/lib/pkgconfig"
    export PATH="$DEPS/bin:$PATH"
    export CFLAGS="-O0 -fno-common -fno-omit-frame-pointer -rdynamic -fsanitize=address -g"
    export LDFLAGS="-Wl,--no-as-needed -Wl,-z,defs -O0 -fno-common -fno-omit-frame-pointer -rdynamic -fsanitize=address -fuse-ld=gold -g -ldl -lasan"

    if [ -x './configure' ]
    then
        ./configure "--prefix=$DEPS" "$@"
        make -j
        make install
    elif [ -f 'meson.build' ]
    then
        mkdir build
        cd build
        meson "--prefix=$DEPS" "$@" ..
        ninja install
    else
        echo "unknown build method"
        exit 1
    fi
}

download_and_build "https://www.x.org/archive/individual/lib/libX11-$X11.tar.gz" --enable-static=no
download_and_build "https://www.x.org/archive//individual/lib/libXrender-$XRENDER.tar.gz" --enable-static=no
download_and_build "https://www.x.org/archive//individual/lib/libXcomposite-$XCOMPOSITE.tar.gz" --enable-static=no
download_and_build "https://www.x.org/archive//individual/lib/libXdamage-$XDAMAGE.tar.gz" --enable-static=no
download_and_build "https://www.x.org/archive//individual/lib/libXext-$XEXT.tar.gz" --enable-static=no
download_and_build "https://www.x.org/archive//individual/lib/libXrandr-$XRANDR.tar.gz" --enable-static=no
download_and_build "https://www.x.org/archive//individual/lib/libXinerama-$XINERAMA.tar.gz" --enable-static=no
download_and_build "https://downloads.sourceforge.net/enlightenment/imlib2-$IMLIB.tar.bz2" --enable-static=no
download_and_build "https://ftp.gnome.org/pub/gnome/sources/glib/$(two_digits "$GLIB")/glib-$GLIB.tar.xz" --enable-debug=yes
download_and_build "https://ftp.gnome.org/pub/gnome/sources/gdk-pixbuf/$(two_digits "$PIXBUF")/gdk-pixbuf-$PIXBUF.tar.xz"
download_and_build "https://cairographics.org/snapshots/cairo-$CAIRO.tar.xz"
download_and_build "https://ftp.gnome.org/pub/gnome/sources/pango/$(two_digits "$PANGO")/pango-$PANGO.tar.xz"
download_and_build "https://ftp.gnome.org/pub/gnome/sources/librsvg/$(two_digits "$RSVG")/librsvg-$RSVG.tar.xz" --enable-pixbuf-loader
