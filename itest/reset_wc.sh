#!/bin/bash

function usage()
{
	echo "Usage: ./reset_wc.sh [wc path] [wc size in G/M]"
}

if [[ $# -ne 2 ]]; then
	usage
	exit -1
fi

if [[ ! -e "$1" ]]; then
	echo "$1 does not exist."
	exit -1
fi

if [[ x"${2:${#2}-1}" == x"G" || x"${2:${#2}-1}" == x"g" ]]; then
        let wc_size_in_m=${2:0:${#2}-1}*1024
elif [[ x"${2:${#2}-1}" == x"M" || x"${2:${#2}-1}" == x"m" ]]; then
        let wc_size_in_m=${2:0:${#2}-1}
else
        usage
        exit -1
fi

let header_size_in_m=wc_size_in_m*1024*1024/4096*32/1024/1024+8
let trailer_size_in_m=4
let seek_size_in_m=wc_size_in_m-trailer_size_in_m

dd if=/dev/zero of=$1 bs=1M count=$header_size_in_m
dd if=/dev/zero of=$1 bs=1M seek=$seek_size_in_m count=$trailer_size_in_m

echo "wc reset successfully."
exit 0