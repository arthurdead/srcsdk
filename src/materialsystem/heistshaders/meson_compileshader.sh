#!/usr/bin/sh

__script_dir__=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

src_root_dir=$(realpath "$__script_dir__/../..")

if [[ -z "$WIN64" ]]; then
	WINE64='/usr/bin/wine64'
fi

if [[ -z "$WINESERVER" ]]; then
	WINESERVER='/usr/bin/wineserver'
fi

shadercompiler="$src_root_dir/devtools/bin/ShaderCompile.exe"

#"$WINESERVER" -p

lin_dir="$2"
file="$1"
outdir="$3"

win_dir=$("$WINE64" winepath -w "$lin_dir")
_code=$?
if [[ $_code != 0 ]]; then
	echo 'failed to get windows path'
	exit 1
fi

name=$(basename "$file")

if [[ "$name" =~ [a-zA-Z0-9_]+_ps2x\.fxc ]]; then
	ver='20b'
	type='ps'
elif [[ "$name" =~ [a-zA-Z0-9_]+_ps20b\.fxc ]]; then
	ver='20b'
	type='ps'
elif [[ "$name" =~ [a-zA-Z0-9_]+_ps20\.fxc ]]; then
	ver='20b'
	type='ps'
elif [[ "$name" =~ [a-zA-Z0-9_]+_ps30\.fxc ]]; then
	ver='30'
	type='ps'
elif [[ "$name" =~ [a-zA-Z0-9_]+_vs20\.fxc ]]; then
	ver='20b'
	type='vs'
else
	echo "cannot determine shader version for \"$name\""
	exit 1
fi

"$WINE64" "$shadercompiler" -shaderpath "$win_dir" -types "$type" -ver "$ver" -optimize '3' "$name"
_code=$?
if [[ $_code != 0 ]]; then
	echo 'failed to execute shadercompiler'
	exit 1
fi

name_no_ext="${name%.*}"

mv "$lin_dir/include/$name_no_ext.inc" "$outdir"
_code=$?
if [[ $_code != 0 ]]; then
	echo 'failed to execute move outputs'
	exit 1
fi

mv "$lin_dir/shaders/fxc/$name_no_ext.vcs" "$outdir"
_code=$?
if [[ $_code != 0 ]]; then
	echo 'failed to execute move outputs'
	exit 1
fi

exit 0
