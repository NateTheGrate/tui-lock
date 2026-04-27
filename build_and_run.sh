#!/bin/bash

rm ~/.cache/tui-lock/tui-lock.log
make clean 
make
gtklock -m ./build/tui-lock.so 
cat ~/.cache/tui-lock/tui-lock.log
