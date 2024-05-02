#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

# This script is custom-tailored for our test setup and may not run everywhere.

bin_path=${1:-}
if [[ -z "$bin_path" ]]; then
    echo "usage: $0 <binary file>"
    exit 1
fi

rm -f grmon_output
script -c "grmon -freq 80 -ftdi -u -jtagcable 1 -e 'reset; wash; load $bin_path; run; reg i0; exit'" grmon_output


if grep -q "Program exited normally" grmon_output; then
    if grep -q "i0 =" grmon_output; then
        ret=$(grep  'i0 =' grmon_output | tr -s ' ' | cut -d ' ' -f 4)
        exit $ret
    fi
fi

exit 1
