; Native Scheme functions
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

; Native scheme functions have the following calling conventions.
; Input:
; EAX - first argument, NULL if zero arguments
; EDX - list of rest of arguments, EMPTY_LIST if zero or one argument
; Output:
; ECX - error object, NULL if everything is Ok
;       If error object is non-null other return values are ignored
; EAX - first result, NULL if zero results
; EDX - list of rest of results, EMPTY_LIST if zero or one argument

; Reverse scheme list
reverse:
        ensure_procedure_args 1
        push ebp
        mov ebp, esp
        push esi
        push edi

        mov esi, eax
        mov edi, empty_list
    .next:
        mov ecx, [esi+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        jne .ok
        ccall memory_create_cons_cell
        mov ecx, [esi+cons_cell.car]
        mov [eax+cons_cell.car], ecx
        mov [eax+cons_cell.cdr], edi
        mov edi, eax
        mov esi, [esi+cons_cell.cdr]
        jmp .next
    .ok:
        mov eax, edi
        mov edx, empty_list
        xor ecx, ecx
    .ret:
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret

scheme_add:
        ensure_procedure_args 2
        mov edx, [edx+cons_cell.car]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov ecx, [eax+number.value]
        add ecx, [edx+number.value]
        ccall memory_create_number, ecx
        mov edx, empty_list
        xor ecx, ecx
        ret
    .arithmetic_error:
        mov ecx, arithmetic_error
        ret

scheme_negate:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov ecx, [eax+number.value]
        neg ecx
        ccall memory_create_number, ecx
        mov edx, empty_list
        xor ecx, ecx
        ret
    .arithmetic_error:
        mov ecx, arithmetic_error
        ret

cons:
        ensure_procedure_args 2
        mov edx, [edx+cons_cell.car]
        push eax
        push edx
        ccall memory_create_cons_cell
        pop edx
        pop ecx
        mov [eax+cons_cell.car], ecx
        mov [eax+cons_cell.cdr], edx
        mov edx, empty_list
        xor ecx, ecx
        ret

car:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        jne pair_expected
        mov eax, [eax+cons_cell.car]
        mov edx, empty_list
        xor ecx, ecx
        ret

cdr:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        jne pair_expected
        mov eax, [eax+cons_cell.cdr]
        mov edx, empty_list
        xor ecx, ecx
        ret

set_car:
        ensure_procedure_args 2
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        jne pair_expected
        mov edx, [edx+cons_cell.car]
        mov [eax+cons_cell.car], edx
        mov edx, empty_list
        xor eax, eax
        mov ecx, eax
        ret

set_cdr:
        ensure_procedure_args 2
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        jne pair_expected
        mov edx, [edx+cons_cell.car]
        mov [eax+cons_cell.cdr], edx
        mov edx, empty_list
        xor eax, eax
        mov ecx, eax
        ret

pair_expected:
        mov ecx, pair_expected_str
        ret

pairp:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        je return_true
        jmp return_false

symbolp:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.symbol
        je return_true
        jmp return_false

numberp:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        je return_true
        jmp return_false

stringp:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.string
        je return_true
        jmp return_false

nullp:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je return_true
        jmp return_false

environmentp:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.environment
        je return_true
        jmp return_false

continuationp:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.continuation
        je return_true
        jmp return_false

native_procedurep:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.native_procedure
        je return_true
        jmp return_false

native_specialp:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.native_special
        je return_true
        jmp return_false

lambdap:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.lambda
        je return_true
        jmp return_false

macrop:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.macro
        je return_true
        jmp return_false

booleanp:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.boolean
        je return_true
        jmp return_false

charp:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.char
        je return_true
        jmp return_false

scheme_eq:
        ensure_procedure_args 2
        mov edx, [edx+cons_cell.car]
        cmp eax, edx
        je return_true
        jmp return_false

scheme_not:
        ensure_procedure_args 1
        cmp eax, false
        je return_true
        jmp return_false

return_true:
        mov eax, true
        mov edx, empty_list
        xor ecx, ecx
        ret

return_false:
        mov eax, false
        mov edx, empty_list
        xor ecx, ecx
        ret

arithmetic_equals:
        ensure_procedure_args 2
        mov edx, [edx+cons_cell.car]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov ecx, [eax+number.value]
        cmp ecx, [edx+number.value]
        je .equal
        mov eax, false
        jmp .ret
    .equal:
        mov eax, true
    .ret:
        mov edx, empty_list
        xor ecx, ecx
        ret
    .arithmetic_error:
        mov ecx, arithmetic_error
        ret

arithmetic_less:
        ensure_procedure_args 2
        mov edx, [edx+cons_cell.car]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov ecx, [eax+number.value]
        cmp ecx, [edx+number.value]
        jl .less
        mov eax, false
        jmp .ret
    .less:
        mov eax, true
    .ret:
        mov edx, empty_list
        xor ecx, ecx
        ret
    .arithmetic_error:
        mov ecx, arithmetic_error
        ret

arithmetic_greater:
        ensure_procedure_args 2
        mov edx, [edx+cons_cell.car]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov ecx, [eax+number.value]
        cmp ecx, [edx+number.value]
        jg .greater
        mov eax, false
        jmp .ret
    .greater:
        mov eax, true
    .ret:
        mov edx, empty_list
        xor ecx, ecx
        ret
    .arithmetic_error:
        mov ecx, arithmetic_error
        ret

string_eq:
        ensure_procedure_args 2
        mov edx, [edx+cons_cell.car]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.string
        jne .not_a_string
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.string
        jne .not_a_string
        push ebx
        lea ecx, [eax+string.data]
        lea ebx, [edx+string.data]
        ccall streq, ecx, [eax+string.length], ebx, [edx+string.length]
        pop ebx
        test eax, eax
        jnz .true
        mov eax, false
        jmp .ret
    .true:
        mov eax, true
    .ret:
        mov edx, empty_list
        xor ecx, ecx
        ret
    .not_a_string:
        mov ecx, not_a_string_error
        ret

string_to_symbol:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.string
        jne .not_a_string
        lea ecx, [eax+string.data]
        ccall get_symbol, ecx, [eax+string.length]
        mov edx, empty_list
        xor ecx, ecx
        ret
    .not_a_string:
        mov ecx, not_a_string_error
        ret

symbol_to_string:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.symbol
        jne .not_a_symbol
        lea ecx, [eax+symbol.name_data]
        ccall memory_create_string_from, ecx, [eax+symbol.name_length]
        mov edx, empty_list
        xor ecx, ecx
        ret
    .not_a_symbol:
        mov ecx, not_a_symbol_error
        ret

string_append_2:
        ensure_procedure_args 2
        mov edx, [edx+cons_cell.car]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.string
        jne .not_a_string
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.string
        jne .not_a_string
        push esi
        push edi
        push ebx
        mov esi, eax
        mov ebx, edx
        mov ecx, [eax+string.length]
        add ecx, [edx+string.length]
        ccall memory_create_string, ecx
        mov edi, eax

        ; Copy first string
        pushd [esi+string.length]
        lea ecx, [esi+string.data]
        pushd ecx
        lea ecx, [edi+string.data]
        pushd ecx
        call memcpy
        add esp, 0xc

        ; Copy second string
        pushd [ebx+string.length]
        lea ecx, [ebx+string.data]
        pushd ecx
        lea ecx, [edi+string.data]
        add ecx, [esi+string.length]
        pushd ecx
        call memcpy
        add esp, 0xc

        ; Add null at the end
        lea ecx, [edi+string.data]
        add ecx, [esi+string.length]
        add ecx, [ebx+string.length]
        mov byte [ecx], 0

        mov eax, edi
        add esp, 0xc
    .ret:
        mov edx, empty_list
        xor ecx, ecx
        ret
    .not_a_string:
        mov ecx, not_a_string_error
        ret

string_ref:
        ensure_procedure_args 2
        mov edx, [edx+cons_cell.car]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.string
        jne .not_a_string
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov edx, [edx+number.value]
        mov ecx, [eax+string.length]
        cmp edx, ecx
        jae .out_of_bounds
        movzx eax, byte [eax+string.data+edx]
        ccall memory_create_char, eax
        mov edx, empty_list
        xor ecx, ecx
        ret
    .not_a_string:
        mov ecx, not_a_string_error
        ret
    .arithmetic_error:
        mov ecx, arithmetic_error
        ret
    .out_of_bounds:
        mov ecx, string_ref_out_of_bounds_error
        ret

string_set:
        ensure_procedure_args 3
        push esi
        mov esi, [edx+cons_cell.cdr]
        mov esi, [esi+cons_cell.car]
        mov edx, [edx+cons_cell.car]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.string
        jne .not_a_string
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov ecx, [esi+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.char
        jne .not_a_char
        mov edx, [edx+number.value]
        mov ecx, [eax+string.length]
        cmp edx, ecx
        jae .out_of_bounds
        movzx ecx, byte [esi+char.value]
        mov [eax+string.data+edx], cl
        pop esi
        mov eax, false
        mov edx, empty_list
        xor ecx, ecx
        ret
    .not_a_string:
        pop esi
        mov ecx, not_a_string_error
        ret
    .arithmetic_error:
        pop esi
        mov ecx, arithmetic_error
        ret
    .not_a_char:
        pop esi
        mov ecx, not_a_char_error
        ret
    .out_of_bounds:
        pop esi
        mov ecx, string_set_out_of_bounds_error
        ret

; Arguments: to at from start end
string_copy:
        .to_addr equ local1
        .to_len equ local2
        .at equ local3
        .from_addr equ local4
        .from_len equ local5
        .start equ local6
        .end equ local7
        ; Initialize
        ensure_procedure_args 5
        push ebp
        mov ebp, esp
        sub esp, 0x1c
        push esi
        push edi
        ; Arg 1: to
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.string
        jne .not_a_string
        lea ecx, [eax+string.data]
        mov [.to_addr], ecx
        mov ecx, [eax+string.length]
        mov [.to_len], ecx
        ; Arg 2: at
        mov eax, [edx+cons_cell.car]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov eax, [eax+number.value]
        mov [.at], eax
        mov edx, [edx+cons_cell.cdr]
        ; Arg 3: from
        mov eax, [edx+cons_cell.car]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.string
        jne .not_a_string
        lea ecx, [eax+string.data]
        mov [.from_addr], ecx
        mov ecx, [eax+string.length]
        mov [.from_len], ecx
        mov edx, [edx+cons_cell.cdr]
        ; Arg 4: start
        mov eax, [edx+cons_cell.car]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov eax, [eax+number.value]
        mov [.start], eax
        mov edx, [edx+cons_cell.cdr]
        ; Arg 5: end
        mov eax, [edx+cons_cell.car]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov eax, [eax+number.value]
        mov [.end], eax
        ; Check 0 <= at <= len(to)
        mov eax, [.at]
        cmp eax, 0
        jnge .dest_out_of_bounds
        mov ecx, [.to_len]
        cmp ecx, eax
        jnge .dest_out_of_bounds
        ; Check 0 <= start <= end <= len(from)
        mov eax, [.start]
        cmp eax, 0
        jnge .src_out_of_bounds
        mov ecx, [.end]
        cmp ecx, eax
        jnge .src_out_of_bounds
        mov eax, [.from_len]
        cmp eax, ecx
        jnge .src_out_of_bounds
        ; Check start - end <= len(to) - at
        mov eax, [.end]
        sub eax, [.start]
        mov ecx, [.to_len]
        sub ecx, [.at]
        cmp ecx, eax
        jnge .dest_out_of_bounds
        ; Prepare for copy
        mov esi, [.from_addr]
        add esi, [.start]
        mov edi, [.to_addr]
        add edi, [.at]
        mov ecx, [.end]
        sub ecx, [.start]
        ; Check copy direction
        cmp esi, edi
        jl .back
        ; Copy data forward
        rep movsb
        jmp .success
    .back:
        ; Copy data backwards
        lea esi, [esi+ecx-1]
        lea edi, [edi+ecx-1]
        std
        rep movsb
        cld
    .success:
        ; Successful return
        mov eax, false
        mov edx, empty_list
        xor ecx, ecx
        jmp .ret
    .not_a_string:
        mov ecx, not_a_string_error
        jmp .ret
    .arithmetic_error:
        mov ecx, arithmetic_error
        jmp .ret
    .src_out_of_bounds:
        mov ecx, string_copy_src_out_of_bounds_error
        jmp .ret
    .dest_out_of_bounds:
        mov ecx, string_copy_dest_out_of_bounds_error
        jmp .ret
    .ret:
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret

char_equals:
        ensure_procedure_args 2
        mov edx, [edx+cons_cell.car]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.char
        jne .char_error
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.char
        jne .char_error
        mov ecx, [eax+char.value]
        cmp ecx, [edx+char.value]
        je .equal
        mov eax, false
        jmp .ret
    .equal:
        mov eax, true
    .ret:
        mov edx, empty_list
        xor ecx, ecx
        ret
    .char_error:
        mov ecx, not_a_char_error
        ret

char_to_integer:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.char
        jne .char_error
        ccall memory_create_number, [eax+char.value]
        mov edx, empty_list
        xor ecx, ecx
        ret
    .char_error:
        mov ecx, not_a_char_error
        ret

integer_to_char:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        mov ecx, [eax+number.value]
        cmp ecx, 0xff
        ja .out_of_bounds
        ccall memory_create_char, ecx
        mov edx, empty_list
        xor ecx, ecx
        ret
    .arithmetic_error:
        mov ecx, arithmetic_error
        ret
    .out_of_bounds:
        mov ecx, char_out_of_bounds_error
        ret

global_environment:
        ensure_procedure_args 0
        mov edx, empty_list
        mov eax, [global_env]
        xor ecx, ecx
        ret

create_environment:
        ensure_procedure_args 0
        ccall memory_create_environment
        mov edx, empty_list
        xor ecx, ecx
        ret

environment_parent:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.environment
        jne pair_expected
        mov eax, [eax+environment.parent]
        mov edx, empty_list
        xor ecx, ecx
        ret

environment_bindings:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.environment
        jne environment_expected
        mov eax, [eax+environment.bindings]
        mov edx, empty_list
        xor ecx, ecx
        ret

set_environment_parent:
        ensure_procedure_args 2
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.environment
        jne pair_expected
        mov edx, [edx+cons_cell.car]
        mov [eax+environment.parent], edx
        mov edx, empty_list
        xor eax, eax
        mov ecx, eax
        ret

set_environment_bindings:
        ensure_procedure_args 2
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.environment
        jne pair_expected
        mov edx, [edx+cons_cell.car]
        mov [eax+environment.bindings], edx
        mov edx, empty_list
        xor eax, eax
        mov ecx, eax
        ret

environment_expected:
        mov ecx, environment_expected_str
        ret

mem_allocated_proc:
        ensure_procedure_args 0
        ccall memory_create_number
        mov ecx, [mem_allocated]
        mov [eax+number.value], ecx
        mov edx, empty_list
        xor ecx, ecx
        ret

mem_occupied_proc:
        ensure_procedure_args 0
        ccall memory_create_number
        mov ecx, [mem_occupied]
        mov [eax+number.value], ecx
        mov edx, empty_list
        xor ecx, ecx
        ret

mem_gc_times_proc:
        ensure_procedure_args 0
        ccall memory_create_number
        mov ecx, [mem_gc_times]
        mov [eax+number.value], ecx
        mov edx, empty_list
        xor ecx, ecx
        ret

mem_max_allocated_proc:
        ensure_procedure_args 0
        ccall memory_create_number
        mov ecx, [mem_max_allocated]
        mov [eax+number.value], ecx
        mov edx, empty_list
        xor ecx, ecx
        ret

scheme_write:
        ensure_procedure_args 1
        ccall print, eax
        xor eax, eax
        mov edx, empty_list
        xor ecx, ecx
        ret

scheme_include:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.string
        jne .not_string
        lea ecx, [eax+string.data]
        ccall exec_file, ecx, [eax+string.length]
        xor eax, eax
        mov edx, empty_list
        xor ecx, ecx
        ret
    .not_string:
        mov ecx, include_string_required
        ret

scheme_gc:
        ensure_procedure_args 0
        ccall memory_gc
        xor eax, eax
        mov edx, empty_list
        xor ecx, ecx
        ret

scheme_eval:
        ensure_procedure_args 2
        mov edx, [edx+cons_cell.car]
        push eax
        push edx
        ccall memory_create_continuation, eval.pop_environment
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eax+continuation.next], ecx
        mov ecx, [eval_environment]
        mov [eax+continuation.x], ecx
        mov ecx, [eval_current_continuation]
        mov [ecx+continuation.next], eax
        ccall memory_create_continuation, eval.eval_cur_env
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eax+continuation.next], ecx
        mov ecx, [eval_current_continuation]
        mov [ecx+continuation.next], eax
        pop edx
        mov [eval_environment], edx
        pop eax
        mov edx, empty_list
        xor ecx, ecx
        ret

apply:
        ensure_procedure_args 2
        mov edx, [edx+cons_cell.car]
        push eax
        push edx
        ccall memory_create_continuation, eval.apply_continuation
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eax+continuation.next], ecx
        mov ecx, [eval_current_continuation]
        mov [ecx+continuation.next], eax
        pop edx
        pop eax
        xor ecx, ecx
        ret

error:
        ensure_procedure_args 1
        mov ecx, eax
        ret

make_string:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        jne .arithmetic_error
        ccall memory_create_string_0, [eax+number.value]
        mov edx, empty_list
        xor ecx, ecx
        ret

    .arithmetic_error:
        mov ecx, arithmetic_error
        ret
