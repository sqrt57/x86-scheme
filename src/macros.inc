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

builtin_symbols_list equ

macro def_symbol_unbound name, str
{
local code_start, name_start, name_length
    name:
        dd  object.tag.symbol or object.tag.fixed
        dd  name_length
        dd  builtin_symbols_prev
    name_start:
        db  str
        name_length = $-name_start
        db  0
        align0 4
builtin_symbols_prev = name
}

macro def_symbol name, str, value
{
local code_start, name_start, name_length
    name:
        dd  object.tag.symbol or object.tag.fixed
        dd  name_length
        dd  builtin_symbols_prev
    name_start:
        db  str
        name_length = $-name_start
        db  0
        align0 4
builtin_symbols_prev = name
builtin_symbols_list equ builtin_symbols_list name
name#_symbol_value equ value
}

macro builtin_symbols_bindings
{
binding_list_prev = empty_list
    match params, builtin_symbols_list
    \{
        irps name, params
        \\{
        name\\#_symbol_binding:
            dd  object.tag.cons_cell
            dd  name
            dd  name\\#_symbol_value
        name\\#_symbol_binding_list:
            dd  object.tag.cons_cell
            dd  name\\#_symbol_binding
            dd  binding_list_prev
        binding_list_prev = name\\#_symbol_binding_list
        \\}
    \}
binding_list = binding_list_prev
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
local procedure_object, name
    procedure_object:
        dd  object.tag.native_procedure or object.tag.fixed
        dd  value
def_symbol name, str, procedure_object
}

macro def_special str, value
{
local special_object, name
    special_object:
        dd  object.tag.native_special or object.tag.fixed
        dd  value
def_symbol name, str, special_object
}

macro ensure_procedure_args n
{
local wrong_count, bad_syntax, ok
if n = 0
        test eax, eax
        jnz wrong_count
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je ok
    wrong_count:
        mov ecx, wrong_args_count_str
        ret
    ok:
else
        push edx
        test eax, eax
        jz wrong_count
repeat (n-1)
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je wrong_count
        cmp ecx, object.tag.cons_cell
        jne bad_syntax
        mov edx, [edx+cons_cell.cdr]
end repeat
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je ok
    wrong_count:
        pop edx
        mov ecx, wrong_args_count_str
        ret
    bad_syntax:
        pop edx
        mov ecx, arg_list_error
        ret
    ok:
        pop edx
end if
}

macro ensure_special_args_simple n
{
    ensure_special_args n, wrong_args_count_str, arg_list_error
}

macro ensure_special_args n, count_str, syntax_str
{
local wrong_count, bad_syntax, ok
if n = 0
        mov eax, [eval_argument]
        test eax, eax
        jnz wrong_count
        mov eax, [eval_args]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je ok
    wrong_count:
        mov eax, count_str
        jmp eval.process_error
    ok:
else
        mov eax, [eval_argument]
        test eax, eax
        jz wrong_count
        mov eax, [eval_args]
repeat (n-1)
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je wrong_count
        cmp ecx, object.tag.cons_cell
        jne bad_syntax
        mov eax, [eax+cons_cell.cdr]
end repeat
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je ok
    wrong_count:
        mov eax, count_str
        jmp eval.process_error
    bad_syntax:
        mov eax, syntax_str
        jmp eval.process_error
    ok:
end if
}
