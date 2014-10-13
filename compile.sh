#!/bin/sh

# set according to each environment
AS=/opt/gcc/ppc64/ppc64-linux-gnu/bin/as
CC=/opt/gcc/ppc64/bin/ppc64-linux-gnu-gcc
OBJDUMP=/opt/gcc/ppc64/ppc64-linux-gnu/bin/objdump

if [ $1 = "-d" ]; then
	dump=$($OBJDUMP -dz $2 2> /dev/null)
	if [ "$dump" = "" ]; then
		echo "error"
		exit
	fi
	n=$(echo "$dump" | grep -n main | cut -d ":" -f 1)
	dump=$(echo "$dump" | sed -n "$[ $n+1 ],\$p" | cut -c 7-17)
	for t in $dump; do
		/bin/echo -n "\x$t"
	done
	echo
else
	$CC -c -o ${1%.c}.o $1
fi
