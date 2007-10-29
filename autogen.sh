#!/bin/sh
autoreconf -i -f &&
./configure --enable-maintainer-mode --disable-static --enable-gtk-doc --enable-vivified --enable-ffmpeg --enable-mad $@
