#!/bin/sh

mkdir build
rm build/enscheme
fasm-linux/fasm src/enscheme-linux32.asm build/enscheme

