#!/bin/bash

make && gtklock -m ./build/libtui-module.so && cat /tmp/tui-module.log
