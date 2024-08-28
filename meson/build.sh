#!/usr/bin/sh

__script_dir__=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

builddir_linux="$__script_dir__/build_linux"

src_root_dir=$(realpath "$__script_dir__/../src")
src_engine_dir=~/'.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer'
game_dir=$(realpath "$__script_dir__/../../the-heist-files")

unset CMAKE_CXX_COMPILER_LAUNCHER
unset CMAKE_C_COMPILER_LAUNCHER

if [[ -d "$builddir_linux" ]]; then
	if [[ ! -f "$builddir_linux/build.ninja" ]]; then
		rm -rf "$builddir_linux"
	fi
fi

if [[ ! -d "$builddir_linux" ]]; then
	meson setup --cross-file "$__script_dir__/i686-gcc-linux-gnu" "$builddir_linux" --backend='ninja' -Dsrc_root_dir="$src_root_dir" -Dsrc_engine_dir="$src_engine_dir" -Dgame_dir="$game_dir"
	if [[ $? != 0 ]]; then
		exit 1
	fi
fi

if [[ ! -d "$builddir_linux" ]]; then
	echo 'no builddir'
	exit 1
fi

meson compile -j 8 -C "$builddir_linux" --ninja-args '-k 0'
if [[ $? != 0 ]]; then
	exit 1
fi

meson install -C "$builddir_linux" --no-rebuild --skip-subprojects='webm'
if [[ $? != 0 ]]; then
	exit 1
fi

exit 0
