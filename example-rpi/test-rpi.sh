#!/bin/bash

##
## Programs
##
LDF=../../linux-load-file/ldf.elf
GRA=../linux-get-release-addr/get-release-addr.elf
MOD_PAIO=../linux-mod-paio/mod-paio.ko
MEM_PAIO=../linux-mem-paio/mem-paio.elf
SEV=../linux-sev/sev.elf

boot_addr=0x30000000

[ ! -z "$1" ] && file=$1 || { echo "need a filename" && exit; }
[ ! -z "$2" ] && cpu=$2 || cpu=3

echo file=$file
echo cpu=$cpu

function install_dtbo() {
    make -C . rpi-install
}

set -x

##
## Load binary file
##
sudo ${LDF} ${boot_addr} ${file}

##
## Find release address
##
release_addr=$( ${GRA} ${cpu} | grep "^CPU\[" | awk '{print $2}' )
echo release_addr=$release_addr

## Force set release address
release_addr=0xF0
echo release_addr=$release_addr

##
## CPU-release (by mod-paio)
##
# sudo rmmod mod-paio; sudo insmod ${MOD_PAIO}; sudo echo "wq ${release_addr} ${boot_addr} 1" > /proc/paio

##
## CPU-release (by /dev/mem)
##
# sudo ${MEM_PAIO} WQ ${release_addr} ${boot_addr}

##
## CPU-release (by /dev/uio*)
##
install_dtbo
sudo ${MEM_PAIO} --dev /dev/uio3 WQ ${release_addr} ${boot_addr}

##
## SEV
##
sudo ${SEV}

