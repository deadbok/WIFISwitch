#!/bin/bash
# Orinial work by Ivan Grokhotkov.
# https://gist.github.com/igrr/43f8b8c3c4f883f980e5
#
# This version modified by Martin Grønholdt
# https://gist.github.com/deadbok/1e9a3fd78bdb854fd9b2
#
# Print data, rodata, bss sizes in bytes
#
# Usage: 
# OBJDUMP=../xtensa-lx106-elf/bin/xtensa-lx106-elf-objdump ./mem_usage.sh app.out [mem_size] [flash_size]
#
# If you specify total_mem_size, free heap size will also be printed
# For esp8266, total_mem_size is 81920
#

if [ -z "$OBJDUMP" ]; then
	OBJDUMP=xtensa-lx106-elf-objdump
fi
 
objdump=$OBJDUMP
objfile=$1
ram_size=$2
rom_size=$3
used=0
 
function get_usage {
	name=$1
	start_sym="_${name}_start"
	end_sym="_${name}_end"
 
	objdump_cmd="$objdump -t $objfile"	
	addr_start=`$objdump_cmd | grep " $start_sym" | cut -d " " -f1 | tr 'a-f' 'A-F'`
	addr_end=`$objdump_cmd | grep " $end_sym" | cut -d " " -f1 | tr 'a-f' 'A-F'`
	size=`printf "ibase=16\n$addr_end - $addr_start\n" | bc`
	used=$(($used+$size))
}
 
get_usage data
get_usage rodata
get_usage bss
echo -n "Firmware uses $used bytes of RAM"
if [[ ! -z "$ram_size" ]]; then
	echo -n  ", $(($ram_size-$used)) bytes free of $ram_size"
fi
echo .

used=0
get_usage irom0_text
echo -n "Firmware uses $used bytes of ROM"
if [[ ! -z "$rom_size" ]]; then
	echo -n ", $(($rom_size-$used)) bytes free of $rom_size"
fi
echo .
