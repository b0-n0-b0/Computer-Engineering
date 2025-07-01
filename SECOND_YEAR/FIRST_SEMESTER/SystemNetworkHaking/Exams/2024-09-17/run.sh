#!/bin/sh
if [ -n "$FLAG" ]; then
	echo $FLAG > flag.txt
fi
./server
