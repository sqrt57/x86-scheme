; vim: filetype=asm

; Miscellaneous macros
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

macro def_string name, [s]
{
common
    name#_str:
forward
    db s
common
    name#_len = $ - name#_str
}

macro write name
{
    ccall write_string, name#_str, name#_len
}

