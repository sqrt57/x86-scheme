; vim: filetype=asm

; Scheme interpreter core functionality
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

macro core_code
{
core_init:
        ret

; Executes scheme code
; In: 1) Address of string with scheme code
;     2) Length of string
exec_string:
        ccall write_string, msg_exec_str, msg_exec_len
        mov eax, [esp+4]
        mov edx, [esp+8]
        ccall write_string, eax, edx
        ccall write_newline
        ret
}

macro core_data
{
def_string msg_exec, 'Executing: '
}

macro core_bss
{
}

