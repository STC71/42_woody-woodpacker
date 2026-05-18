#!/bin/bash
dd if=/bin/ls of=trunc_elf bs=1 count=60 > /dev/null 2>&1
./woody_woodpacker trunc_elf 2>&1
