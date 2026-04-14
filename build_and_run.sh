#!/bin/bash

rm /tmp/tui-lock.log
make clean 
make
gtklock -m ./build/tui-lock.so 
cat /tmp/tui-lock.log
