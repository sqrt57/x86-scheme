#!/bin/sh

mkdir build
rm build/enscheme
rm build/enscheme.exe
fasm-linux/fasm src/enscheme-linux32.asm build/enscheme
fasm-linux/fasm src/enscheme-win32.asm build/enscheme.exe

