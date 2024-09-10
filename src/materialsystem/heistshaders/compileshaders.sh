#!/usr/bin/sh

__script_dir__=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

src_root_dir=$(realpath "$__script_dir__/../..")

if [[ -z "$WIN64" ]]; then
	WINE64='/usr/bin/wine64'
fi

if [[ -z "$WINESERVER" ]]; then
	WINESERVER='/usr/bin/wineserver'
fi

gameshaders_dir="$src_root_dir/materialsystem/heistshaders"

stdshaders_dir="$src_root_dir/materialsystem/stdshaders"
shadercompiler="$src_root_dir/devtools/bin/ShaderCompile.exe"

"$WINESERVER" -p

_gameshaders_dir_win=$("$WINE64" winepath -w "$gameshaders_dir")
_stdshaders_dir_win=$("$WINE64" winepath -w "$stdshaders_dir")

while read line; do
	name=$(echo "$line")
	
	if [[ -z "$name" || "$name" == '#*' ]]; then
		continue
	fi

	if [[ "$name" =~ [a-zA-Z0-9_]+_ps2x\.fxc ]]; then
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
		continue
	fi

	"$WINE64" "$shadercompiler" -shaderpath "$_stdshaders_dir_win" -types "$type" -ver "$ver" -optimize '3' "$name"
done < "$__script_dir__/stdshaders.txt"
