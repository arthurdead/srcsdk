#!/usr/bin/sh

__script_dir__=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

mesondir="$__script_dir__"

cd "$mesondir"

"$mesondir/build_linux.sh"
_code=$?
if [[ $_code != 0 ]]; then
	exit 1
fi

"$mesondir/build_windows.sh"
_code=$?
if [[ $_code != 0 ]]; then
	exit 1
fi

exit 0
