#!/usr/bin/sh

__script_dir__=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

builddir_linux="$__script_dir__/build_linux"

src_root_dir=$(realpath "$__script_dir__/../src")
src_engine_dir=~/'.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer'
game_dir=~/'.steam/steam/steamapps/sourcemods/heist'

export CCACHE_COMPILERTYPE='gcc'
export CCACHE_COMPILER=

export CC='/usr/lib/ccache/bin/gcc'
export CXX='/usr/lib/ccache/bin/g++'

export CMAKE_C_COMPILER='/usr/bin/gcc'
export CMAKE_C_COMPILER_LAUNCHER='/usr/bin/ccache'
export CMAKE_CXX_COMPILER='/usr/bin/g++'
export CMAKE_CXX_COMPILER_LAUNCHER='/usr/bin/ccache'

export CMAKE_LINKER_TYPE='GOLD'

if [[ -d "$builddir_linux/everything" ]]; then
	if [[ ! -f "$builddir_linux/everything/build.ninja" ]]; then
		rm -rf "$builddir_linux/everything"
	fi
fi

if [[ ! -d "$builddir_linux/everything" ]]; then
	meson setup --cross-file "$__script_dir__/i686-gcc-linux-gnu" "$builddir_linux/everything" --backend='ninja' -Dsrc_root_dir="$src_root_dir" -Dsrc_engine_dir="$src_engine_dir" -Dgame_dir="$game_dir" -Dbuild_libs=true -Dbuild_client=true -Dbuild_listen_server=true -Dbuild_dedicated_server=false
	if [[ $? != 0 ]]; then
		exit 1
	fi
fi

if [[ ! -d "$builddir_linux/everything" ]]; then
	echo 'no builddir'
	exit 1
fi

_jobs=6

samu -j $_jobs -C "$builddir_linux/everything" -k 0
if [[ $? != 0 ]]; then
	exit 1
fi

meson install -C "$builddir_linux/everything" --no-rebuild
if [[ $? != 0 ]]; then
	exit 1
fi

exit 0
