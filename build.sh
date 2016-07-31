#!/bin/sh

mkdir build
rm build/x86-scheme
rm build/x86-scheme.exe
fasm-linux/fasm src/x86-scheme-linux32.asm build/x86-scheme
fasm-linux/fasm src/x86-scheme-win32.asm build/x86-scheme.exe

