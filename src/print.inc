; S-expressions printer
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

; Prints scheme object to stdout
; In: 1) Address of scheme object
print:
        push ebp
        mov ebp, esp
        mov eax, [ebp+0x08]
        call .inner_print
        ccall write_newline
        mov esp, ebp
        pop ebp
        ret

    .inner_print:
        test eax, eax
        jz .null
        mov edx, [eax+object.tag]
        and edx, object.tag.type_mask
        cmp edx, object.tag.number
        je .number
        cmp edx, object.tag.string
        je .string
        cmp edx, object.tag.symbol
        je .symbol
        cmp edx, object.tag.empty_list
        je .empty_list
        cmp edx, object.tag.cons_cell
        je .list
        cmp edx, object.tag.bytevector
        je .bytevector
        cmp edx, object.tag.environment
        je .environment
        cmp edx, object.tag.continuation
        je .continuation
        cmp edx, object.tag.native_procedure
        je .native_procedure
        cmp edx, object.tag.native_special
        je .native_special
        cmp edx, object.tag.lambda
        je .lambda
        cmp edx, object.tag.macro
        je .macro
        cmp edx, object.tag.boolean
        je .boolean
        write core_unknown_object
        ret

    .number:
        push ebp
        mov ebp, esp
        .number_buf_end equ ebp
        sub esp, 0x0c
        push edi
        lea edi, [.number_buf_end]
        mov eax, [eax+number.value]
        test eax, eax
        jns .positive
        not eax
        inc eax
        call .convert_positive
        mov edx, '-'
        dec edi
        mov byte [edi], dl
        jmp .do_print_number
    .positive:
        call .convert_positive
    .do_print_number:
        lea eax, [.number_buf_end]
        sub eax, edi
        ccall write_string, edi, eax
        pop edi
        mov esp, ebp
        pop ebp
        ret

    .convert_positive:
        mov ecx, 10
    .number_next:
        xor edx, edx
        div ecx
        add edx, '0'
        dec edi
        mov byte [edi], dl
        test eax, eax
        jnz .number_next
        ret

    .string:
        push eax
        write double_quote
        pop eax
        mov ecx, [eax+string.length]
        lea edx, [eax+string.data]
        ccall write_string, edx, ecx
        write double_quote
        ret

    .symbol:
        mov ecx, [eax+symbol.name_length]
        lea edx, [eax+symbol.name_data]
        ccall write_string, edx, ecx
        ret


    .bytevector:
        write bytevector_repr
        ret

    .environment:
        write environment_repr
        ret

    .continuation:
        write continuation_repr
        ret

    .native_procedure:
        write native_procedure_repr
        ret

    .native_special:
        write native_special_repr
        ret

    .lambda:
        write lambda_repr
        ret

    .macro:
        write macro_repr
        ret

    .empty_list:
        write empty_list
        ret

    .boolean:
        mov ecx, [eax+boolean.value]
        test ecx, ecx
        jnz .true
        write false
        ret
    .true:
        write true
        ret

    .list:
        push eax
        write leftbr
    .list_next:
        mov eax, [esp]
        mov eax, [eax+cons_cell.car]
        call .inner_print
        pop eax
        mov eax, [eax+cons_cell.cdr]
        test eax, eax
        jz .improper_list
        mov edx, [eax+object.tag]
        and edx, object.tag.type_mask
        cmp edx, object.tag.cons_cell
        jnz .finish_list
        push eax
        write space
        jmp .list_next
    .finish_list:
        cmp edx, object.tag.empty_list
        jnz .improper_list
        write rightbr
        ret
    .improper_list:
        push eax
        write space_dot_space
        pop eax
        call .inner_print
        write rightbr
        ret

    .null:
        write null_ref
        ret
