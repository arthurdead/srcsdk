#!/usr/bin/sh

__script_dir__=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

builddir="$__script_dir__/build_windows"

mesondir="$__script_dir__"

src_root_dir=$(realpath "$__script_dir__/../src")

src_engine_target='sdk2013mp'
if [[ "$src_engine_target" == 'sdk2013mp' ]]; then
	src_engine_dir=~/'.steam/steam/steamapps/common/Source SDK Base 2013 Multiplayer'
	src_dedicated_engine_dir=~/'.steam/steam/steamapps/common/Source SDK Base 2013 Dedicated Server'
elif [[ "$src_engine_target" == 'sdk2013sp' ]]; then
	src_engine_dir=~/'.steam/steam/steamapps/common/Source SDK Base 2013 Singleplayer'
	src_dedicated_engine_dir=~/'.steam/steam/steamapps/common/Source SDK Base 2013 Dedicated Server'
else
	echo "unknown engine target"
	exit 1
fi

game_dir=~/'.steam/steam/steamapps/sourcemods/heist'

cd "$mesondir"

_jobs=6

export CCACHE_COMPILERTYPE='gcc'
export CCACHE_COMPILER=

export CC='/usr/lib/ccache/bin/i686-w64-mingw32-gcc'
export CC_LD='bfd'
export CXX='/usr/lib/ccache/bin/i686-w64-mingw32-g++'
export CXX_LD='bfd'

export CMAKE_C_COMPILER='/usr/bin/i686-w64-mingw32-gcc'
export CMAKE_C_COMPILER_LAUNCHER='/usr/bin/ccache'
export CMAKE_CXX_COMPILER='/usr/bin/i686-w64-mingw32-g++'
export CMAKE_CXX_COMPILER_LAUNCHER='/usr/bin/ccache'

export CMAKE_LINKER_TYPE='BFD'

if [[ -d "$builddir/everything" ]]; then
	if [[ ! -f "$builddir/everything/build.ninja" ]]; then
		rm -rf "$builddir/everything"
	fi
fi

if [[ ! -d "$builddir/everything" ]]; then
	meson setup \
	--cross-file "$__script_dir__/i686-mingw-windows" \
	"$builddir/everything" \
	"$mesondir" \
	--backend='ninja' \
	-Dsrc_root_dir="$src_root_dir" \
	-Dsrc_engine_dir="$src_engine_dir" \
	-Dsrc_dedicated_engine_dir="$src_dedicated_engine_dir" \
	-Dsrc_engine_target="$src_engine_target" \
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

rm "$game_dir/bin/"*".dll.a"
rm "$game_dir/bin/tools/"*".dll.a"

exit 0
