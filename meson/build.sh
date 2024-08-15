#!/usr/bin/sh

builddir=./'build_linux'
src_root_dir=$(realpath ./../'src')
src_engine_dir=~/'.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer'

if [[ -d "$builddir" ]]; then
	if [[ ! -f "$builddir/build.ninja" ]]; then
		rm -rf "$builddir"
	fi
fi

if [[ ! -d "$builddir" ]]; then
	meson setup --cross-file ./'x86-linux-gnu' "$builddir" --backend='ninja' -Dsrc_root_dir="$src_root_dir" -Dsrc_engine_dir="$src_engine_dir"
	if [[ $? != 0 ]]; then
		exit 1
	fi
fi

if [[ ! -d "$builddir" ]]; then
	echo 'no builddir'
	exit 1
fi

meson compile -j 8 -C "$builddir"
if [[ $? != 0 ]]; then
	exit 1
fi

exit 0
