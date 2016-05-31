
; enscheme executable for 32-bit Linux
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

include 'macros.inc'
include 'proc.inc'

include 'unicode.inc'
include 'core.inc'
include 'runner.inc'

format ELF executable 3

entry platform_start

segment readable executable

platform_start:
        ; Clear direction flag for calling conventions
        cld

        ccall linux32_init

        ccall main, [argc], [argv], [argl]

        mov ebx, eax
        mov eax, 1
        int 80h

include 'system-linux32.inc'
unicode_code
core_code
runner_code

segment readable writeable
def_string newline, 10

unicode_data
core_data
runner_data

segment writeable
        bytes_written   rd  1
        argc            rd  1
        argv            rd  1
        argl            rd  1

unicode_bss
core_bss
runner_bss
