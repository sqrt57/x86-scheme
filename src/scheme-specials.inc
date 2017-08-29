; Native Scheme special forms
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

; Native special forms have the following API.
; They are working inside stack frame of eval.
; They have access to the following variables (addressed base on EBP,
; so EBP must be preserved when accessing them):
; [eval_argument] - first argument, NULL if zero arguments
; [eval_args] - list of rest of arguments, EMPTY_LIST if zero
;               or one arguments
; [eval_current_continuation] - current continuation, which contains
;               ".apply_procedure_or_special" continuation.
;               Code must at least update current continuation to
;               NEXT field of current continuation
; [eval_environment] - current environment, global_env is the global environment
;
; Code may read these variables and update their values.
; When finished, code must jump to "eval.next"
; If an error occured, then code puts error object in EAX
; and jumps to "eval.process_error"

quote:
        ensure_special_args 1, quote_args_str, special_bad_syntax
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eval_current_continuation], ecx
        jmp eval.next

scheme_if:
        ensure_special_args 3, if_args_str, special_bad_syntax
        ccall memory_create_continuation, if_continuation
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        mov edx, [eval_args]
        mov ecx, [edx+cons_cell.car]
        mov [eax+continuation.x], ecx
        mov edx, [edx+cons_cell.cdr]
        mov ecx, [edx+cons_cell.car]
        mov [eax+continuation.y], ecx
        ccall memory_create_continuation, eval.eval_cur_env
        mov edx, [eval_current_continuation]
        mov [eax+continuation.next], edx
        mov [eval_current_continuation], eax
        mov dword [eval_args], empty_list
        jmp eval.next

if_continuation:
        ensure_special_args_simple 1
        ccall memory_create_continuation, eval.eval_cur_env
        mov edx, [eval_current_continuation]
        mov ecx, [edx+continuation.next]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        mov ecx, [eval_argument]
        cmp ecx, false
        je .false
        mov ecx, [edx+continuation.x]
        mov [eval_argument], ecx
        jmp eval.next
    .false:
        mov ecx, [edx+continuation.y]
        mov [eval_argument], ecx
        jmp eval.next

lambda:
        mov eax, [eval_argument]
        test eax, eax
        jz .error_no_args
        mov edx, [eval_args]
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        jne .body_syntax_error
        mov edx, [eval_current_continuation]
        mov edx, [edx+continuation.next]
        mov [eval_current_continuation], edx
        ccall memory_create_lambda
        mov edx, [eval_argument]
        mov [eax+lambda.args], edx
        mov edx, [eval_args]
        mov [eax+lambda.body], edx
        mov edx, [eval_environment]
        mov [eax+lambda.environment], edx
        mov [eval_argument], eax
        mov dword [eval_args], empty_list
        jmp eval.next
    .error_no_args:
        mov eax, lambda_no_args_str
        jmp eval.process_error
    .body_syntax_error:
        mov eax, lambda_body_syntax_error_str
        jmp eval.process_error

scheme_macro:
        mov eax, [eval_argument]
        test eax, eax
        jz .error_no_args
        mov edx, [eval_args]
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        jne .body_syntax_error
        mov edx, [eval_current_continuation]
        mov edx, [edx+continuation.next]
        mov [eval_current_continuation], edx
        ccall memory_create_macro
        mov edx, [eval_argument]
        mov [eax+macro.args], edx
        mov edx, [eval_args]
        mov [eax+macro.body], edx
        mov edx, [eval_environment]
        mov [eax+macro.environment], edx
        mov [eval_argument], eax
        mov dword [eval_args], empty_list
        jmp eval.next
    .error_no_args:
        mov eax, macro_no_args_str
        jmp eval.process_error
    .body_syntax_error:
        mov eax, macro_body_syntax_error_str
        jmp eval.process_error

scheme_define:
        ensure_special_args 2, define_args_count, define_bad_syntax
        mov ecx, [eval_argument]
        mov ecx, [ecx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.symbol
        je .ok
        mov eax, define_symbol_required
        jmp eval.process_error
    .ok:
        ccall memory_create_continuation, define_continuation
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eax+continuation.next], ecx
        mov ecx, [eval_argument]
        mov [eax+continuation.x], ecx
        mov [eval_current_continuation], eax
        ccall memory_create_continuation, eval.eval_cur_env
        mov ecx, [eval_current_continuation]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        mov eax, [eval_args]
        mov eax, [eax+cons_cell.car]
        mov [eval_argument], eax
        mov dword [eval_args], empty_list
        jmp eval.next

define_continuation:
        ensure_special_args_simple 1
        push esi
        push edi
        mov edi, [eval_current_continuation]
        mov ecx, [edi+continuation.next]
        mov [eval_current_continuation], ecx
        mov edi, [edi+continuation.x]
        mov esi, [eval_environment]
        test esi, esi
        jz .error
        mov edx, [esi+environment.bindings]
    .next_binding:
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je .not_found
        cmp ecx, object.tag.cons_cell
        jne .error
        mov eax, [edx+cons_cell.car]
        mov edx, [edx+cons_cell.cdr]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        jne .error
        cmp edi, [eax+cons_cell.car]
        jne .next_binding
        mov ecx, [eval_argument]
        mov [eax+cons_cell.cdr], ecx
    .ret:
        xor eax, eax
        mov [eval_argument], eax
        pop edi
        pop esi
        jmp eval.next
    .not_found:
        ccall memory_create_cons_cell
        mov [eax+cons_cell.car], edi
        mov ecx, [eval_argument]
        mov [eax+cons_cell.cdr], ecx
        mov edi, eax
        ccall memory_create_cons_cell
        mov [eax+cons_cell.car], edi
        mov ecx, [esi+environment.bindings]
        mov [eax+cons_cell.cdr], ecx
        mov [esi+environment.bindings], eax
        jmp .ret
    .error:
        mov eax, environment_error
        pop edi
        pop esi
        jmp eval.process_error

scheme_set:
        ensure_special_args 2, set_args_count, set_bad_syntax
        mov ecx, [eval_argument]
        mov ecx, [ecx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.symbol
        je .ok
        mov eax, define_symbol_required
        jmp eval.process_error
    .ok:
        ccall memory_create_continuation, set_continuation
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eax+continuation.next], ecx
        mov ecx, [eval_argument]
        mov [eax+continuation.x], ecx
        mov [eval_current_continuation], eax
        ccall memory_create_continuation, eval.eval_cur_env
        mov ecx, [eval_current_continuation]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        mov eax, [eval_args]
        mov eax, [eax+cons_cell.car]
        mov [eval_argument], eax
        mov dword [eval_args], empty_list
        jmp eval.next

set_continuation:
        ensure_special_args_simple 1
        push esi
        push edi
        mov edi, [eval_current_continuation]
        mov ecx, [edi+continuation.next]
        mov [eval_current_continuation], ecx
        mov edi, [edi+continuation.x]
        mov esi, [eval_environment]
        ccall get_symbol_pair, edi, esi
        test eax, eax
        jz .not_bound
        mov ecx, [eval_argument]
        mov [eax+cons_cell.cdr], ecx
    .ret:
        xor eax, eax
        mov [eval_argument], eax
        pop edi
        pop esi
        jmp eval.next
    .not_bound:
        mov eax, set_not_bound
        pop edi
        pop esi
        jmp eval.process_error

scheme_eval_current_environment:
        ensure_special_args_simple 1
        ccall memory_create_continuation, eval.eval_cur_env
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        ccall memory_create_continuation, eval.eval_cur_env
        mov ecx, [eval_current_continuation]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        jmp eval.next

call_cc:
        ensure_special_args_simple 1
        ccall memory_create_continuation, call_cc_continuation
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        ccall memory_create_continuation, eval.eval_cur_env
        mov ecx, [eval_current_continuation]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        jmp eval.next

call_cc_continuation:
        ensure_special_args_simple 1
        ccall memory_create_cons_cell
        mov edx, [eval_current_continuation]
        mov edx, [edx+continuation.next]
        mov [eax+cons_cell.car], edx
        mov dword [eax+cons_cell.cdr], empty_list
        mov edi, eax
        mov esi, empty_list
        mov ebx, [eval_argument]
        jmp eval.do_apply

begin:
        mov eax, [eval_argument]
        test eax, eax
        jz .zero_args

        ccall memory_create_cons_cell
        mov ecx, [eval_argument]
        mov [eax+cons_cell.car], ecx
        mov ecx, [eval_args]
        mov [eax+cons_cell.cdr], ecx
        push eax

        ccall memory_create_continuation, eval.eval_list
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eax+continuation.next], ecx
        popd [eax+continuation.x]
        mov [eval_current_continuation], eax
        xor eax, eax
        mov [eval_argument], eax
        mov dword [eval_args], empty_list
        jmp eval.next

    .zero_args:
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eval_current_continuation], ecx
        jmp eval.next

current_environment:
        ensure_special_args_simple 0
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eval_current_continuation], ecx
        mov eax, [eval_environment]
        mov [eval_argument], eax
        jmp eval.next
