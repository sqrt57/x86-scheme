
; enscheme executable for 32-bit Linux
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

include 'macros.inc'
include 'proc.inc'

include 'unicode.inc'
include 'core.inc'
include 'runner.inc'
include 'memory.inc'

format ELF executable 3

entry platform_start

segment executable

platform_start:
        ; Clear direction flag for calling conventions
        cld
        
        pop [argc]
        mov [argv], esp

        ccall linux32_init

        ccall main, [argc], [argv], [argl]

        mov ebx, eax
        mov eax, 1
        int 0x80

include 'system-linux32.inc'
unicode_code
core_code
runner_code
memory_code

segment readable
def_string newline, 10
unicode_rodata
core_rodata
runner_rodata
memory_rodata

segment readable writeable
unicode_rwdata
core_rwdata
runner_rwdata
memory_rwdata

segment readable writeable
        argc            rd  1
        argv            rd  1
        argl            rd  1
        cur_brk         rd  1

unicode_bss
core_bss
runner_bss
memory_bss

