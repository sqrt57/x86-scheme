; Scheme code evaluator
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

; Executes scheme object as code
; In: 1) Scheme expression
; Out: Expression result
eval:
        push ebp
        mov ebp, esp
        sub esp, vm_size
        push esi
        push edi
        push ebx

        eval_vm equ ebp-vm_size
        eval_current_continuation equ eval_vm+vm.current_continuation
        eval_argument equ eval_vm+vm.argument
        eval_args equ eval_vm+vm.args
        eval_environment equ eval_vm+vm.environment

        mov eax, [vms]
        mov [eval_vm+vm.next], eax
        lea eax, [eval_vm]
        mov [vms], eax

        mov dword [eval_args], empty_list
        mov eax, [global_env]
        mov [eval_environment], eax
        mov eax, [ebp+0x08]
        mov [eval_argument], eax
        ccall memory_create_continuation, .return_code
        mov [eval_current_continuation], eax
        ccall memory_create_continuation, .eval_cur_env
        mov edx, [eval_current_continuation]
        mov [eax+continuation.next], edx
        mov [eval_current_continuation], eax

    .next:
        ccall memory_check_gc
        mov eax, [eval_current_continuation]
        jmp dword [eax+continuation.code]

    .process_error:
        mov edx, eax
        xor eax, eax
        jmp .ret

    .eval_cur_env:
        mov eax, [eval_args]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je .eval_many_args_ok
        mov eax, eval_many_args_str
        jmp .process_error
    .eval_many_args_ok:
        mov eax, [eval_argument]
        test eax, eax
        jz .eval_zero_args
        mov edx, [eax+object.tag]
        and edx, object.tag.type_mask
        cmp edx, object.tag.cons_cell
        je .cons_cell
        cmp edx, object.tag.symbol
        je .symbol
        cmp edx, object.tag.number
        je .self
        cmp edx, object.tag.string
        je .self
        cmp edx, object.tag.bytevector
        je .self
        cmp edx, object.tag.boolean
        je .self
        jmp .unknown_type

    .eval_zero_args:
        write eval_zero_args
        xor eax, eax
        jmp .process_error

    .return_code:
        mov eax, [eval_argument]
        xor edx, edx
        jmp .ret

    .ret:
        mov esi, [eval_vm+vm.next]
        mov [vms], esi
        pop ebx
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret

    .unknown_type:
        write eval_unknown
        ccall print, [eval_argument]
        xor eax, eax
        jmp .process_error

    .self:
        ; Retrieve next continuation passing to it the same argument
        mov eax, [eval_current_continuation]
        mov eax, [eax+continuation.next]
        mov [eval_current_continuation], eax
        jmp .next

    .symbol:
        ; Value bound to symbol becomes argument to next continuation
        mov eax, [eval_argument]
        ccall get_symbol_value, [eval_argument], [eval_environment]
        test eax, eax
        jz .unbound_symbol
        mov [eval_argument], eax
        ; Retrieve next continuation
        mov eax, [eval_current_continuation]
        mov eax, [eax+continuation.next]
        mov [eval_current_continuation], eax
        jmp .next
    .unbound_symbol:
        write unbound_symbol
        mov eax, [eval_argument]
        mov ecx, [eax+symbol.name_length]
        lea edx, [eax+symbol.name_data]
        ccall write_string, edx, ecx
        ccall write_newline
        xor eax, eax
        jmp .process_error

    .cons_cell:
        ; Procedure or special form call
        ; Apply procedure or special form to CDR of current cons cell
        ccall memory_create_continuation, .apply_procedure_or_special
        mov edx, [eval_current_continuation]
        mov edx, [edx+continuation.next]
        mov [eax+continuation.next], edx
        ; Store raw argument list in X field
        mov edx, [eval_argument]
        mov edx, [edx+cons_cell.cdr]
        mov [eax+continuation.x], edx
        mov [eval_current_continuation], eax
        ; But before we evaluate CAR of this cons cell
        ccall memory_create_continuation, .eval_cur_env
        mov edx, [eval_current_continuation]
        mov [eax+continuation.next], edx
        mov [eval_current_continuation], eax
        ; Pass CAR of current cons cell as argument
        mov edx, [eval_argument]
        mov edx, [edx+cons_cell.car]
        mov [eval_argument], edx
        jmp .next

    .apply_procedure_or_special:
        mov eax, [eval_args]
        mov eax, [eax+object.tag]
        and eax, object.tag.type_mask
        cmp eax, object.tag.empty_list
        je .operator_many_args_ok
        mov eax, operator_many_args
        jmp .process_error
    .operator_many_args_ok:
        mov eax, [eval_argument]
        test eax, eax
        jz .operator_empty_arg
        mov edx, [eax+object.tag]
        and edx, object.tag.type_mask
        cmp edx, object.tag.native_procedure
        je .process_procedure
        cmp edx, object.tag.lambda
        je .process_procedure
        cmp edx, object.tag.continuation
        je .process_procedure
        cmp edx, object.tag.native_special
        je .process_special
        cmp edx, object.tag.macro
        je .process_macro
        write eval_apply_wrong_type
        xor eax, eax
        jmp .process_error
    .process_procedure:
        mov ebx, eax
        mov edx, [eval_current_continuation]
        mov esi, [edx+continuation.x]
        mov edi, empty_list
        jmp .eval_arg_or_apply
    .process_macro:
        mov ebx, eax
        mov edx, [eval_current_continuation]
        mov esi, empty_list
        mov edi, [edx+continuation.x]
        jmp .do_apply_macro
    .process_special:
        mov edi, eax
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.x]
        test ecx, ecx
        jz .special_no_args
        mov edx, [ecx+object.tag]
        and edx, object.tag.type_mask
        cmp edx, object.tag.empty_list
        jz .special_no_args
        cmp edx, object.tag.cons_cell
        jne .error_args_list
        mov eax, [ecx+cons_cell.car]
        mov [eval_argument], eax
        mov eax, [ecx+cons_cell.cdr]
        mov [eval_args], eax
        jmp dword [edi+native_special.code]
    .special_no_args:
        xor eax, eax
        mov [eval_argument], eax
        mov dword [eval_args], empty_list
        jmp dword [edi+native_special.code]
    .operator_empty_arg:
        write operator_empty_arg
        xor eax, eax
        jmp .process_error

        ; In:
        ; EBX - procedure
        ; ESI - (rest) unevaluated arguments list
        ; EDI - reversed list of evaluated arguments
    .eval_arg_or_apply:
        mov ecx, [esi+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je .do_apply
        cmp ecx, object.tag.cons_cell
        jne .error_args_list
        ccall memory_create_continuation, .process_args
        mov edx, [eval_current_continuation]
        mov ecx, [edx+continuation.next]
        mov [eax+continuation.next], ecx
        ; Store procedure in X
        mov [eax+continuation.x], ebx
        ; Store evaluated arguments list in Y
        mov [eax+continuation.y], edi
        ; Store raw argument list in Z
        mov ecx, [esi+cons_cell.cdr]
        mov [eax+continuation.z], ecx
        mov [eval_current_continuation], eax
        ; But before we evaluate CAR of this cons cell
        ccall memory_create_continuation, .eval_cur_env
        mov edx, [eval_current_continuation]
        mov [eax+continuation.next], edx
        mov [eval_current_continuation], eax
        ; Pass CAR of unevaluated operands list as argument
        mov ecx, [esi+cons_cell.car]
        mov [eval_argument], ecx
        mov dword [eval_args], empty_list
        jmp .next
    .do_apply:
        mov eax, edi
        mov edx, empty_list
        call reverse
        mov edi, eax
        mov ecx, [ebx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.native_procedure
        je .native_procedure_args
        cmp ecx, object.tag.lambda
        je .do_apply_lambda
        cmp ecx, object.tag.continuation
        je .continuation_args
        mov eax, do_apply_unknown_type_str
        jmp .process_error
    .native_procedure_args:
        mov ecx, [edi+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        je .native_procedure_many_args
        xor eax, eax
        mov edx, empty_list
        jmp .do_apply_native_procedure
    .native_procedure_many_args:
        mov edx, [edi+cons_cell.cdr]
        mov eax, [edi+cons_cell.car]
    .do_apply_native_procedure:
        call dword [ebx+native_procedure.code]
        test ecx, ecx
        jnz .native_error
        mov [eval_argument], eax
        mov [eval_args], edx
        mov eax, [eval_current_continuation]
        mov eax, [eax+continuation.next]
        mov [eval_current_continuation], eax
        jmp .next
    .native_error:
        mov eax, ecx
        jmp .process_error

    .apply_continuation:
        mov ebx, [eval_argument]
        mov edi, [eval_args]
        mov ecx, [ebx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.native_procedure
        je .native_procedure_args
        cmp ecx, object.tag.lambda
        je .do_apply_lambda
        cmp ecx, object.tag.continuation
        je .continuation_args
        mov eax, do_apply_unknown_type_str
        jmp .process_error

    .continuation_args:
        mov ecx, [edi+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        je .continuation_many_args
        xor eax, eax
        mov edx, empty_list
        jmp .do_apply_continuation
    .continuation_many_args:
        mov edx, [edi+cons_cell.cdr]
        mov eax, [edi+cons_cell.car]
    .do_apply_continuation:
        mov [eval_current_continuation], ebx
        jmp .next

    .do_apply_lambda:
        ; EBX - procedure
        ; ESI - procedure formal parameters list
        ; EDI - list of evaluated arguments
        ; Save current environment for popping it later
        ccall memory_create_continuation, .pop_environment
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eax+continuation.next], ecx
        mov ecx, [eval_environment]
        mov [eax+continuation.x], ecx
        mov [eval_current_continuation], eax
        ; Create continuation for evaluating lambda body
        ccall memory_create_continuation, .eval_list
        mov ecx, [eval_current_continuation]
        mov [eax+continuation.next], ecx
        mov ecx, [ebx+lambda.body]
        mov [eax+continuation.x], ecx
        mov [eval_current_continuation], eax
        xor eax, eax
        mov [eval_argument], eax
        mov dword [eval_args], empty_list
        ; Create new environment
        ccall memory_create_environment
        mov edx, [ebx+lambda.environment]
        mov [eax+environment.parent], edx
        mov [eval_environment], eax
        ; Set bindings for lambda arguments in new environment
        mov esi, [ebx+lambda.args]
        ccall bind_parameters, esi, edi
        test eax, eax
        jz .lambda_args_count
        mov edx, [eval_environment]
        mov [edx+environment.bindings], eax
        jmp .next
    .lambda_args_count:
        mov eax, lambda_args_count_str
        jmp .process_error

    .do_apply_macro:
        ; EBX - macro
        ; ESI - macro formal parameters list
        ; EDI - list of evaluated arguments
        ; Continuation for evaluating expanded macro
        ccall memory_create_continuation, .eval_cur_env
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        ; Save current environment for popping it later
        ccall memory_create_continuation, .pop_environment
        mov ecx, [eval_current_continuation]
        mov [eax+continuation.next], ecx
        mov ecx, [eval_environment]
        mov [eax+continuation.x], ecx
        mov [eval_current_continuation], eax
        ; Continuation for evaluating macro body
        ccall memory_create_continuation, .eval_list
        mov ecx, [eval_current_continuation]
        mov [eax+continuation.next], ecx
        mov ecx, [ebx+macro.body]
        mov [eax+continuation.x], ecx
        mov [eval_current_continuation], eax
        xor eax, eax
        mov [eval_argument], eax
        mov dword [eval_args], empty_list
        ; Create new environment
        mov esi, [ebx+macro.args]
        ccall memory_create_environment
        mov edx, [ebx+macro.environment]
        mov [eax+environment.parent], edx
        mov [eval_environment], eax
        ; Set bindings for macro arguments in new environment
        ccall bind_parameters, esi, edi
        test eax, eax
        jz .macro_args_count
        mov edx, [eval_environment]
        mov [edx+environment.bindings], eax
        jmp .next
    .macro_args_count:
        mov eax, macro_args_count_str
        jmp .process_error

    .eval_list:
        mov eax, [eval_current_continuation]
        mov edx, [eax+continuation.x]
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je .done_eval_list
        cmp ecx, object.tag.cons_cell
        je .eval_list_car
        mov eax, lambda_body_syntax_str
        jmp .process_error
    .done_eval_list:
        mov eax, [eax+continuation.next]
        mov [eval_current_continuation], eax
        jmp .next
    .eval_list_car:
        ccall memory_create_continuation, .eval_list
        mov edx, [eval_current_continuation]
        mov ecx, [edx+continuation.next]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        mov esi, [edx+continuation.x]
        mov ecx, [esi+cons_cell.cdr]
        mov [eax+continuation.x], ecx
        ccall memory_create_continuation, .eval_cur_env
        mov edx, [eval_current_continuation]
        mov [eax+continuation.next], edx
        mov [eval_current_continuation], eax
        mov eax, [esi+cons_cell.car]
        mov [eval_argument], eax
        mov dword [eval_args], empty_list
        jmp .next

    .pop_environment:
        mov eax, [eval_current_continuation]
        mov ecx, [eax+continuation.x]
        mov [eval_environment], ecx
        mov eax, [eax+continuation.next]
        mov [eval_current_continuation], eax
        jmp .next

    .process_args:
        ; Continuation fields:
        ; X - procedure object
        ; Y - list of evaluated arguments in reverse order
        ; Z - list of rest unevaluated arguments
        mov eax, [eval_args]
        mov eax, [eax+object.tag]
        and eax, object.tag.type_mask
        cmp eax, object.tag.empty_list
        je .process_args_many_args_ok
        write operand_many_args
        xor eax, eax
        jmp .process_error
    .process_args_many_args_ok:
        mov ecx, [eval_argument]
        test ecx, ecx
        jz .process_args_no_arg
        mov edx, [eval_current_continuation]
        mov ebx, [edx+continuation.x]
        mov edi, [edx+continuation.y]
        mov esi, [edx+continuation.z]
        ccall memory_create_cons_cell
        mov ecx, [eval_argument]
        mov [eax+cons_cell.car], ecx
        mov [eax+cons_cell.cdr], edi
        mov edi, eax
        jmp .eval_arg_or_apply
    .empty_args:
        xor edx, edx
        mov [eval_argument], edx
        mov [eval_args], edx
    .process_args_no_arg:
        write operand_empty_arg
        xor eax, eax
        jmp .process_error

    .error_args_list:
        mov eax, eval_error_args_list
        jmp .process_error
