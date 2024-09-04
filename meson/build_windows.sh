#!/usr/bin/sh

__script_dir__=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

builddir_windows="$__script_dir__/build_windows"

src_root_dir=$(realpath "$__script_dir__/../src")
src_engine_dir=~/'.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer'
game_dir=$(realpath "$__script_dir__/../../the-heist-files")

unset CMAKE_CXX_COMPILER_LAUNCHER
unset CMAKE_C_COMPILER_LAUNCHER

if [[ -d "$builddir_windows" ]]; then
	if [[ ! -f "$builddir_windows/build.ninja" ]]; then
		rm -rf "$builddir_windows"
	fi
fi

if [[ ! -d "$builddir_windows" ]]; then
	meson setup --cross-file "$__script_dir__/i686-w64-mingw32" "$builddir_windows" --backend='ninja' -Dsrc_root_dir="$src_root_dir" -Dsrc_engine_dir="$src_engine_dir" -Dgame_dir="$game_dir"
	if [[ $? != 0 ]]; then
		exit 1
	fi
fi

if [[ ! -d "$builddir_windows" ]]; then
	echo 'no builddir'
	exit 1
fi

meson compile -j 8 -C "$builddir_windows" --ninja-args '-k 0'
if [[ $? != 0 ]]; then
	exit 1
fi

meson install -C "$builddir_windows" --no-rebuild --skip-subprojects='webm'
if [[ $? != 0 ]]; then
	exit 1
fi

exit 0
