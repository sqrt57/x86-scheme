; vim: filetype=asm

; Miscellaneous macros
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

macro def_string name, str
{
    name#_str db str
    name#_len = $ - name#_str
}
