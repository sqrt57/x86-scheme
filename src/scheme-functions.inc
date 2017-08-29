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
        test eax, eax
        jz .done
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        je .first_ok
        mov ecx, arithmetic_error
        ret
    .first_ok:
        mov eax, [eax+number.value]
    .next:
        test edx, edx
        jz .done
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        je .get_car
        cmp ecx, object.tag.empty_list
        je .done
        mov ecx, arg_list_error
        ret
    .get_car:
        push ebx
        mov ebx, [edx+cons_cell.car]
        mov edx, [edx+cons_cell.cdr]
        mov ecx, [ebx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        je .next_ok
        pop ebx
        mov ecx, arithmetic_error
        ret
    .next_ok:
        mov ecx, [ebx+number.value]
        add eax, ecx
        pop ebx
        jmp .next
    .done:
        ccall memory_create_number, eax
        xor ecx, ecx
        mov edx, empty_list
        ret

scheme_sub:
        push esi
        test eax, eax
        jz .done
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        je .first_ok
        mov ecx, arithmetic_error
        ret
    .first_ok:
        xor esi, esi
        mov eax, [eax+number.value]
    .next:
        test edx, edx
        jz .done
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        je .get_car
        cmp ecx, object.tag.empty_list
        je .done
        mov ecx, arg_list_error
        pop esi
        ret
    .get_car:
        mov esi, 1
        push ebx
        mov ebx, [edx+cons_cell.car]
        mov edx, [edx+cons_cell.cdr]
        mov ecx, [ebx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.number
        je .next_ok
        pop ebx
        pop esi
        mov ecx, arithmetic_error
        ret
    .next_ok:
        mov ecx, [ebx+number.value]
        sub eax, ecx
        pop ebx
        jmp .next
    .done:
        test esi, esi
        jnz .done1
        ; Exactly one argument - calculate negative of it
        not eax
        inc eax
    .done1:
        pop esi
        ccall memory_create_number, eax
        xor ecx, ecx
        mov edx, empty_list
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
        ccall memory_create_string, ecx, [eax+symbol.name_length]
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
        ccall memory_create_string_0, ecx
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
