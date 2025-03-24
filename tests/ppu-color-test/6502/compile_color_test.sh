#!/bin/sh
ca65 color_test.s -g -o color_test.o
ld65 -C nrom.cfg -o color_test.nes color_test.o
