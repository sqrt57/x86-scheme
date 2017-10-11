; S-expressions parser
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

; Parses scheme object from string buffer
; In: 1) Address of buffer
;     2) Length of data in buffer
; Out: EAX - address of obtained scheme object
;      EDX - updated address inside buffer after parsed object
;   If cannot parse returns EAX = EDX = NULL
parse:
        ; ESI - address of next character
        ; [.bufend] - address of end of buffer
        ; [.multichar_start] - address of start of currently parsed literal
        ; [.multichar_length] - length of current literal
        ; [.newlist] - flag "after start of new list"
        ; [.afterdot] - flag "after dot"
        ; [.afterdotobj] - flag "after object after dot"
        ; [.current] - address of current cons cell
        ; [.result] - lexed scheme object
        ; [.list_result] - whole list result
        .bufend equ ebp-0x04
        .multichar_start equ ebp-0x08
        .multichar_length equ ebp-0x0c
        .newlist equ ebp-0x10
        .afterdot equ ebp-0x14
        .afterdotobj equ ebp-0x18
        .current equ ebp-0x1c
        .result equ ebp-0x20
        .list_result equ ebp-0x24

        push ebp
        mov ebp, esp
        sub esp, 0x24
        push esi
        push ebx

        mov esi, [ebp+0x08]
        mov eax, esi
        add eax, [ebp+0x0c]
        mov [.bufend], eax
        xor eax, eax
        mov [.result], eax
        mov [.list_result], eax
        mov [.newlist], eax
        mov [.afterdot], eax
        mov [.afterdotobj], eax
        mov [.current], eax

    .next:
        mov [.multichar_start], esi
        cmp esi, [.bufend]
        jae .endofbuf
        xor eax, eax
        lodsb
        call .iswhite
        jz .next

        cmp eax, ';'
        je .comment

        cmp eax, '('
        je .leftbr

        cmp eax, ')'
        je .rightbr

        cmp eax, '"'
        je .string

        call .isdigit
        jz .number

        jmp .symbol

    .leftbr:
        mov eax, [.afterdot]
        test eax, eax
        jnz .error_leftbr_after_dot
        mov eax, [.afterdotobj]
        test eax, eax
        jnz .error_leftbr_after_dotobj
        mov eax, [.newlist]
        test eax, eax
        jnz .newsublist
        mov dword [.newlist], 1
        mov edx, [.current]
        test edx, edx
        jz .next
        ccall memory_create_cons_cell
        mov edx, [.current]
        mov [.current], eax
        mov ecx, [edx+cons_cell.cdr]
        mov [eax+cons_cell.cdr], ecx
        mov [edx+cons_cell.cdr], eax
        jmp .next

    .newsublist:
        ccall memory_create_cons_cell
        mov edx, [.current]
        mov [.current], eax
        mov [eax+cons_cell.cdr], edx
        test edx, edx
        jz .first_cell
        mov [edx+cons_cell.car], eax
        jmp .next
    .first_cell:
        mov [.list_result], eax
        jmp .next

    .rightbr:
        mov eax, [.afterdot]
        test eax, eax
        jnz .error_rightbr_after_dot
        mov eax, [.afterdotobj]
        test eax, eax
        jnz .close_dot_obj
        mov eax, [.newlist]
        xor edx, edx
        mov [.newlist], edx
        test eax, eax
        jnz .empty_list
        mov edx, [.current]
        test edx, edx
        jz .rightbr_error
        mov eax, [edx+cons_cell.cdr]
        mov dword [edx+cons_cell.cdr], empty_list
        mov [.current], eax
        test eax, eax
        jz .done
        jmp .next

    .close_dot_obj:
        mov dword [.afterdotobj], 0
        mov eax, [.current]
        test eax, eax
        jz .done
        jmp .next

    .empty_list:
        mov edx, [.current]
        test edx, edx
        jz .whole_empty_list
        mov dword [edx+cons_cell.car], empty_list
        jmp .next

    .whole_empty_list:
        mov dword [.result], empty_list
        jmp .done

    .string:
        cmp esi, [.bufend]
        jae .string_error
        xor eax, eax
        lodsb
        cmp eax, '"'
        jnz .string
        mov eax, [.multichar_start]
        inc eax
        mov edx, esi
        sub edx, eax
        dec edx
        ccall memory_create_string_from, eax, edx
        mov [.result], eax
        jmp .process_result

    .number:
        cmp esi, [.bufend]
        jae .done_number_endbuf
        xor eax, eax
        lodsb
        call .isdigit
        jz .number
    .done_number:
        dec esi
    .done_number_endbuf:
        mov eax, [.multichar_start]
        mov edx, esi
        sub edx, eax
        mov [.multichar_start], eax
        mov [.multichar_length], edx
        jmp .parse_number

    .symbol:
        cmp esi, [.bufend]
        jae .done_symbol_endbuf
        xor eax, eax
        lodsb
        cmp eax, '('
        je .done_symbol
        cmp eax, ')'
        je .done_symbol
        cmp eax, ';'
        je .done_symbol
        cmp eax, '"'
        je .done_symbol
        call .iswhite
        jz .done_symbol
        jmp .symbol
    .done_symbol:
        dec esi
    .done_symbol_endbuf:
        mov eax, [.multichar_start]
        mov edx, esi
        sub edx, eax
        cmp edx, 1
        jnz .get_symbol
        cmp byte [eax], '.'
        jnz .get_symbol
        jmp .dot
    .get_symbol:
        ccall get_symbol, eax, edx
        mov [.result], eax
        jmp .process_result

    .rightbr_error:
        write rightbr_error
        jmp .skipbuf

    .leftbr_error:
        write leftbr_error
        jmp .skipbuf

    .string_error:
        write string_error
        jmp .skipbuf

    .endofbuf:
        mov eax, [.newlist]
        test eax, eax
        jnz .leftbr_error
        mov eax, [.current]
        test eax, eax
        jnz .leftbr_error
        mov eax, [.afterdot]
        test eax, eax
        jnz .leftbr_error
        mov eax, [.afterdotobj]
        test eax, eax
        jnz .leftbr_error
        jmp .skipbuf

    .skipbuf:
        xor eax, eax
        jmp .ret

    .comment:
        cmp esi, [.bufend]
        jae .endofbuf
        lodsb
        cmp eax, 0x0a
        jz .next
        jmp .comment

    ; Indicates whether character in EAX is whitespace
    ; Return ZF=1 iff it is whitespace
    .iswhite:
        ; We consider 09-0d, 20 as whitespace
        cmp eax, 0x20
        jne .iswhite1
        ret
    .iswhite1:
        cmp eax, 0x09
        jae .iswhite2
        ret
    .iswhite2:
        cmp eax, 0x0d
        jbe .iswhite3
        ret
    .iswhite3:
        cmp eax, eax
        ret

    ; Indicates whether character in EAX is decimal digit
    ; Return ZF=1 iff it is decimal digit
    .isdigit:
        cmp eax, '0'
        jae .isdigit1
        ret
    .isdigit1:
        cmp eax, '9'
        jbe .isdigit2
        ret
    .isdigit2:
        cmp eax, eax
        ret

    .done:
        mov eax, [.list_result]
        test eax, eax
        jnz .ret
        mov eax, [.result]
        jmp .ret

    .ret:
        mov edx, esi
        pop ebx
        pop esi
        mov esp, ebp
        pop ebp
        ret

    .parse_number:
        push ebx
        push esi
        mov esi, [.multichar_start]
        mov ecx, [.multichar_length]
        xor eax, eax
    .parse_number_digit:
        jcxz .parse_number_end
        movzx ebx, byte [esi]
        sub ebx, '0'
        mov edx, 10
        mul edx
        add eax, ebx
        inc esi
        dec ecx
        jmp .parse_number_digit
    .parse_number_end:
        ccall memory_create_number, eax
        mov [.result], eax
        pop esi
        pop ebx
        jmp .process_result

    .process_result:
        mov eax, [.afterdot]
        test eax, eax
        jnz .object_to_cdr
        mov eax, [.afterdotobj]
        test eax, eax
        jnz .error_second_object_after_dot
        mov eax, [.newlist]
        test eax, eax
        jnz .append_to_newlist
        mov edx, [.current]
        test edx, edx
        jz .done
        ccall memory_create_cons_cell
        mov edx, [.current]
        mov ecx, [edx+cons_cell.cdr]
        mov [eax+cons_cell.cdr], ecx
        mov [edx+cons_cell.cdr], eax
        mov ecx, [.result]
        mov [eax+cons_cell.car], ecx
        mov [.current], eax
        jmp .next

    .object_to_cdr:
        mov dword [.afterdot], 0
        mov dword [.afterdotobj], 1
        mov edx, [.current]
        mov ecx, [edx+cons_cell.cdr]
        mov eax, [.result]
        mov [edx+cons_cell.cdr], eax
        mov [.current], ecx
        jmp .next

    .append_to_newlist:
        xor eax, eax
        mov [.newlist], eax
        ccall memory_create_cons_cell
        mov edx, [.current]
        mov [.current], eax
        mov ecx, [.result]
        mov [eax+cons_cell.car], ecx
        test edx, edx
        jz .new_whole_list
        mov [edx+cons_cell.car], eax
        mov [eax+cons_cell.cdr], edx
        jmp .next

    .new_whole_list:
        mov [.list_result], eax
        jmp .next

    .dot:
        mov eax, [.newlist]
        test eax, eax
        jnz .error_dot_after_leftbr
        mov eax, [.afterdot]
        test eax, eax
        jnz .error_dot_after_dot
        mov eax, [.afterdotobj]
        test eax, eax
        jnz .error_dot_after_dot
        mov edx, [.current]
        test edx, edx
        jz .error_dot_outside_list
        mov dword [.afterdot], 1
        jmp .next

    .error_dot_after_leftbr:
        write error_dot_after_leftbr
        jmp .skipbuf

    .error_dot_after_dot:
        write error_dot_after_dot
        jmp .skipbuf

    .error_dot_outside_list:
        write error_dot_outside_list
        jmp .skipbuf

    .error_leftbr_after_dot:
        write error_leftbr_after_dot
        jmp .skipbuf

    .error_leftbr_after_dotobj:
        write error_leftbr_after_dotobj
        jmp .skipbuf

    .error_rightbr_after_dot:
        write error_rightbr_after_dot
        jmp .skipbuf

    .error_second_object_after_dot:
        write error_second_object_after_dot
        jmp .skipbuf
