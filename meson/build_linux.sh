#!/usr/bin/sh

__script_dir__=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

builddir="$__script_dir__/build_linux"

mesondir="$__script_dir__"

src_root_dir=$(realpath "$__script_dir__/../src")
src_engine_dir=~/'.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer'
game_dir=~/'.steam/steam/steamapps/sourcemods/heist'

cd "$mesondir"

_jobs=6

export CCACHE_COMPILERTYPE='gcc'
export CCACHE_COMPILER=

export CC='/usr/lib/ccache/bin/gcc'
export CC_LD='gold'
export CXX='/usr/lib/ccache/bin/g++'
export CXX_LD='gold'

export CMAKE_C_COMPILER='/usr/bin/gcc'
export CMAKE_C_COMPILER_LAUNCHER='/usr/bin/ccache'
export CMAKE_CXX_COMPILER='/usr/bin/g++'
export CMAKE_CXX_COMPILER_LAUNCHER='/usr/bin/ccache'

export CMAKE_LINKER_TYPE='GOLD'

if [[ -d "$builddir/everything" ]]; then
	if [[ ! -f "$builddir/everything/build.ninja" ]]; then
		rm -rf "$builddir/everything"
	fi
fi

if [[ ! -d "$builddir/everything" ]]; then
	meson setup \
	--cross-file "$__script_dir__/i686-gcc-linux" \
	"$builddir/everything" \
	"$mesondir" \
	--backend='ninja' \
	-Dsrc_root_dir="$src_root_dir" \
	-Dsrc_engine_dir="$src_engine_dir" \
	-Dgame_dir="$game_dir" \
	-Dfunny=true \
	-Dbuild_libs=true \
	-Dbuild_client=true \
	-Dbuild_listen_server=true \
	-Dbuild_dedicated_server=false
	if [[ $? != 0 ]]; then
		exit 1
	fi
fi

if [[ ! -d "$builddir/everything" ]]; then
	echo 'no builddir'
	exit 1
fi

ninja -j $_jobs -C "$builddir/everything" -k 0
_code=$?
if [[ $_code != 0 ]]; then
	exit 1
fi

meson install -C "$builddir/everything" --no-rebuild
_code=$?
if [[ $_code != 0 ]]; then
	exit 1
fi

export CCACHE_COMPILERTYPE='gcc'
export CCACHE_COMPILER=

export CC='/usr/lib/ccache/bin/winegcc'
export CC_LD='gold'
export CXX='/usr/lib/ccache/bin/wineg++'
export CXX_LD='gold'

export CMAKE_C_COMPILER='/usr/bin/gcc'
export CMAKE_C_COMPILER_LAUNCHER='/usr/bin/ccache'
export CMAKE_CXX_COMPILER='/usr/bin/g++'
export CMAKE_CXX_COMPILER_LAUNCHER='/usr/bin/ccache'

export CMAKE_LINKER_TYPE='GOLD'

if [[ -d "$builddir/hammer_dll" ]]; then
	if [[ ! -f "$builddir/hammer_dll/build.ninja" ]]; then
		rm -rf "$builddir/hammer_dll"
	fi
fi

if [[ ! -d "$builddir/hammer_dll" ]]; then
	meson setup \
	--cross-file "$__script_dir__/i686-winegcc-linux" \
	"$builddir/hammer_dll" \
	"$mesondir" \
	--backend='ninja' \
	-Dsrc_root_dir="$src_root_dir" \
	-Dsrc_engine_dir="$src_engine_dir" \
	-Dgame_dir="$game_dir" \
	-Dfunny=false \
	-Dbuild_libs=false \
	-Dbuild_client=true \
	-Dbuild_listen_server=false \
	-Dbuild_dedicated_server=false
	if [[ $? != 0 ]]; then
		exit 1
	fi
fi

if [[ ! -d "$builddir/hammer_dll" ]]; then
	echo 'no builddir'
	exit 1
fi

ninja -j $_jobs -C "$builddir/hammer_dll" -k 0
_code=$?
if [[ $_code == 0 ]]; then
	meson install -C "$builddir/hammer_dll" --no-rebuild
fi

exit 0
