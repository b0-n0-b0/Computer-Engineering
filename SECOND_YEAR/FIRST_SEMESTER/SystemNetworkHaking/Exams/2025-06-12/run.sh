#!/bin/sh
echo $FLAG > flag.txt
LD_LIBRARY_PATH=. ./server
