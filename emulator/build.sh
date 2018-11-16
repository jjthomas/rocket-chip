#!/bin/bash

riscv64-unknown-elf-gcc -I../riscv-tools/riscv-tests/build/../benchmarks/../env -I../riscv-tools/riscv-tests/build/../benchmarks/common -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -o $1.riscv $1.c ../riscv-tools/riscv-tests/build/../benchmarks/common/syscalls.c ../riscv-tools/riscv-tests/build/../benchmarks/common/crt.S -static -nostdlib -nostartfiles -lm -lgcc -T ../riscv-tools/riscv-tests/build/../benchmarks/common/test.ld
