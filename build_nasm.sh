#!/bin/sh
if [ $# -lt 1 ]
then
  echo "Usage $0 in_file"
  exit 0
fi
f=$(basename -s .asm $1)
rm $f $f.o
nasm -f elf64 $1 -o $f.o
ld $f.o -o $f