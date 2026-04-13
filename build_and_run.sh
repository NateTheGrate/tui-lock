#!/bin/bash

make clean && make && gtklock -m ./build/libtui-module.so && cat /tmp/tui-module.log
