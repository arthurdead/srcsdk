#!/usr/bin/sh

__script_dir__=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

builddir_linux="$__script_dir__/build_linux"

src_root_dir=$(realpath "$__script_dir__/../src")
src_engine_dir=~/'.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer'
game_dir=$(realpath "$__script_dir__/../../the-heist-files")

export CCACHE_COMPILERTYPE='gcc'
export CCACHE_COMPILER=

export CC='/usr/lib/ccache/bin/gcc'
export CXX='/usr/lib/ccache/bin/g++'

export CMAKE_C_COMPILER='/usr/bin/gcc'
export CMAKE_C_COMPILER_LAUNCHER='/usr/bin/ccache'
export CMAKE_CXX_COMPILER='/usr/bin/g++'
export CMAKE_CXX_COMPILER_LAUNCHER='/usr/bin/ccache'

export CMAKE_LINKER_TYPE='GOLD'

if [[ -d "$builddir_linux/libs" ]]; then
	if [[ ! -f "$builddir_linux/libs/build.ninja" ]]; then
		rm -rf "$builddir_linux/libs"
	fi
fi

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

if [[ ! -d "$builddir_linux/libs" ]]; then
	meson setup --cross-file "$__script_dir__/i686-gcc-linux-gnu" "$builddir_linux/libs" --backend='ninja' -Dsrc_root_dir="$src_root_dir" -Dsrc_engine_dir="$src_engine_dir" -Dgame_dir="$game_dir" -Dbuild_libs=true -Dbuild_client=false -Dbuild_listen_server=false -Dbuild_dedicated_server=false
	if [[ $? != 0 ]]; then
		exit 1
	fi
fi

if [[ ! -d "$builddir_linux/client" ]]; then
	meson setup --cross-file "$__script_dir__/i686-gcc-linux-gnu" "$builddir_linux/client" --backend='ninja' -Dsrc_root_dir="$src_root_dir" -Dsrc_engine_dir="$src_engine_dir" -Dgame_dir="$game_dir" -Dbuild_libs=false -Dbuild_client=true -Dbuild_listen_server=false -Dbuild_dedicated_server=false
	if [[ $? != 0 ]]; then
		exit 1
	fi
fi

if [[ ! -d "$builddir_linux/server" ]]; then
	meson setup --cross-file "$__script_dir__/i686-gcc-linux-gnu" "$builddir_linux/server" --backend='ninja' -Dsrc_root_dir="$src_root_dir" -Dsrc_engine_dir="$src_engine_dir" -Dgame_dir="$game_dir" -Dbuild_libs=false -Dbuild_client=false -Dbuild_listen_server=true -Dbuild_dedicated_server=false
	if [[ $? != 0 ]]; then
		exit 1
	fi
fi

if [[ ! -d "$builddir_linux/libs" || ! -d "$builddir_linux/client" || ! -d "$builddir_linux/server" ]]; then
	echo 'no builddir'
	exit 1
fi

_jobs=6

samu -j $_jobs -C "$builddir_linux/libs" -k 0
if [[ $? != 0 ]]; then
	exit 1
fi

meson install -C "$builddir_linux/libs" --no-rebuild --skip-subprojects='webm'
if [[ $? != 0 ]]; then
	exit 1
fi

samu -j $_jobs -C "$builddir_linux/client" -k 0
if [[ $? != 0 ]]; then
	exit 1
fi

meson install -C "$builddir_linux/client" --no-rebuild --skip-subprojects='webm'
if [[ $? != 0 ]]; then
	exit 1
fi

samu -j $_jobs -C "$builddir_linux/server" -k 0
if [[ $? != 0 ]]; then
	exit 1
fi

meson install -C "$builddir_linux/server" --no-rebuild --skip-subprojects='webm'
if [[ $? != 0 ]]; then
	exit 1
fi

exit 0
