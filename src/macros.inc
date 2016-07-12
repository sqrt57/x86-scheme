; vim: filetype=asm syntax=fasm

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
    db 0
}

macro write name
{
    ccall write_string, name#_str, name#_len
}

macro align0 n
{
    db (($+n-1)/n*n - $) dup(0)
}

builtin_symbols_prev = 0

macro def_symbol str, value
{
local code_start, name_start, name_length, symbol_start
    symbol_start:
        dd  object.tag.symbol or object.tag.fixed
        dd  value
        dd  builtin_symbols_prev
        builtin_symbols_prev = symbol_start
        dd  name_length
    name_start:
        db  str
        name_length = $-name_start
        db  0
        align0 4
}

macro def_scheme_string name, [value]
{
common
local length, string_start
    name:
        dd  object.tag.string or object.tag.fixed
        dd  length
    string_start:
forward
        db  value
common
        length = $-string_start
        db  0
        align0 4
}

macro def_proc str, value
{
local procedure_object
    procedure_object:
        dd  object.tag.native_procedure or object.tag.fixed
        dd  value
def_symbol str, procedure_object
}

macro def_special str, value
{
local special_object
    special_object:
        dd  object.tag.native_special or object.tag.fixed
        dd  value
def_symbol str, special_object
}

