#!/usr/bin/sh

__script_dir__=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

builddir_linux="$__script_dir__/build_linux"

src_root_dir=$(realpath "$__script_dir__/../src")
src_engine_dir=~/'.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer'
game_dir=$(realpath "$__script_dir__/../../the-heist-files")

unset CMAKE_CXX_COMPILER_LAUNCHER
unset CMAKE_C_COMPILER_LAUNCHER

if [[ -d "$builddir_linux/client" ]]; then
	if [[ ! -f "$builddir_linux/client/build.ninja" ]]; then
		rm -rf "$builddir_linux/client"
	fi
fi

if [[ -d "$builddir_linux/server" ]]; then
	if [[ ! -f "$builddir_linux/server/build.ninja" ]]; then
		rm -rf "$builddir_linux/server"
	fi
fi

if [[ -d "$builddir_linux/dedicated" ]]; then
	if [[ ! -f "$builddir_linux/dedicated/build.ninja" ]]; then
		rm -rf "$builddir_linux/dedicated"
	fi
fi

if [[ ! -d "$builddir_linux/client" ]]; then
	meson setup --cross-file "$__script_dir__/i686-gcc-linux-gnu" "$builddir_linux/client" --backend='ninja' -Dsrc_root_dir="$src_root_dir" -Dsrc_engine_dir="$src_engine_dir" -Dgame_dir="$game_dir" -Dclient=true -Dserver=false -Ddedicated=false
	if [[ $? != 0 ]]; then
		exit 1
	fi
fi

if [[ ! -d "$builddir_linux/server" ]]; then
	meson setup --cross-file "$__script_dir__/i686-gcc-linux-gnu" "$builddir_linux/server" --backend='ninja' -Dsrc_root_dir="$src_root_dir" -Dsrc_engine_dir="$src_engine_dir" -Dgame_dir="$game_dir" -Dclient=false -Dserver=true -Ddedicated=false
	if [[ $? != 0 ]]; then
		exit 1
	fi
fi

if [[ ! -d "$builddir_linux/dedicated" ]]; then
	meson setup --cross-file "$__script_dir__/i686-gcc-linux-gnu" "$builddir_linux/dedicated" --backend='ninja' -Dsrc_root_dir="$src_root_dir" -Dsrc_engine_dir="$src_engine_dir" -Dgame_dir="$game_dir" -Dclient=false -Dserver=false -Ddedicated=true
	if [[ $? != 0 ]]; then
		exit 1
	fi
fi

if [[ ! -d "$builddir_linux/client" || ! -d "$builddir_linux/server" || ! -d "$builddir_linux/dedicated" ]]; then
	echo 'no builddir'
	exit 1
fi

samu -j 8 -C "$builddir_linux/client" -k 0
if [[ $? != 0 ]]; then
	exit 1
fi

samu -j 8 -C "$builddir_linux/server" -k 0
if [[ $? != 0 ]]; then
	exit 1
fi

samu -j 8 -C "$builddir_linux/dedicated" -k 0
if [[ $? != 0 ]]; then
	exit 1
fi

muon install -C "$builddir_linux/client" --no-rebuild --skip-subprojects='webm'
if [[ $? != 0 ]]; then
	exit 1
fi

muon install -C "$builddir_linux/server" --no-rebuild --skip-subprojects='webm'
if [[ $? != 0 ]]; then
	exit 1
fi

muon install -C "$builddir_linux/dedicated" --no-rebuild --skip-subprojects='webm'
if [[ $? != 0 ]]; then
	exit 1
fi

exit 0
