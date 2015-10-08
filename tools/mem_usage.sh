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
# OBJDUMP=../xtensa-lx106-elf/bin/xtensa-lx106-elf-objdump ./mem_usage.sh app.out [total_mem_size]
#
# If you specify total_mem_size, free heap size will also be printed
# For esp8266, total_mem_size is 81920
#

if [ -z "$OBJDUMP" ]; then
	OBJDUMP=xtensa-lx106-elf-objdump
fi
 
objdump=$OBJDUMP
app_out=$1
total_size=$2
used=0
 
function print_usage {
	name=$1
	start_sym="_${name}_start"
	end_sym="_${name}_end"
 
	objdump_cmd="$objdump -t $app_out"	
	addr_start=`$objdump_cmd | grep " $start_sym" | cut -d " " -f1 | tr 'a-f' 'A-F'`
	addr_end=`$objdump_cmd | grep " $end_sym" | cut -d " " -f1 | tr 'a-f' 'A-F'`
	size=`printf "ibase=16\n$addr_end - $addr_start\n" | bc`
	used=$(($used+$size))
	echo $name: $size
}
 
print_usage data
print_usage rodata
print_usage bss
echo total: $used
if [[ ! -z "$total_size" ]]; then
	echo free: $(($total_size-$used))
fi
