; x86-scheme executable for 32-bit Linux
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2018, Dmitry Grigoryev

macro PLATFORM_HEADER
{
format ELF executable 3
}

macro PLATFORM_CODE
{
entry platform_start

segment executable

include 'system-linux32.inc'
}

macro PLATFORM_RDATA
{
segment readable
}

macro PLATFORM_DATA
{
segment readable writeable
}

macro PLATFORM_BSS
{
segment readable writeable
        argc            rd  1
        argv            rd  1
        argl            rd  1
        cur_brk         rd  1
}

current_platform = symbol_win32

include 'x86-scheme.inc'
