; Top-level interpreter driver
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

; Cross-platform entry point
; In: 1) argc - count of command-line arguments
;     2) argv - array of argument adresses
;     3) argl - array of argument lengths
; Out: Result (0 means Ok)
main:
        .argc equ ebp+0x08
        .argv equ ebp+0x0c
        .argl equ ebp+0x10

        ; EBX stores current state:
        ;   0: Normal state
        ;   1: After '-e', executes next argument as string
        ;   2: After '--', execute all remaining arguments as files
        ; ESI stores index of current argument
        ;   We skip 0-th argument because it is executable name
        push ebp
        mov ebp, esp
        push ebx
        push esi


        ccall memory_init

        xor ebx, ebx
        xor esi, esi

    .next_arg:
        inc esi
        cmp esi, [.argc]
        jnb .end_ok

        mov eax, [.argv]
        mov eax, [eax + esi*4]
        mov edx, [.argl]
        mov edx, [edx + esi*4]

        cmp ebx, 1
        jz .exec_arg

        cmp ebx, 2
        jz .exec_file

        cmp byte [eax], '-'
        jnz .exec_file

        cmp edx, 2
        jne .error_arg

        cmp byte [eax+1], 'e'
        je .next_expr

        cmp byte [eax+1], '-'
        je .options_done

        jmp .error_arg

    .next_expr:
        mov ebx, 1
        jmp .next_arg

    .options_done:
        mov ebx, 2
        jmp .next_arg

    .end_ok:
        cmp ebx, 1
        jz .error_exec_state
        xor eax, eax
    .end:
        pop esi
        pop ebx
        mov esp, ebp
        pop ebp
        ret

    .exec_file:
        ccall exec_file, eax, edx
        jmp .next_arg

    .exec_arg:
        mov ebx, 0
        ccall exec_string, eax, edx
        jmp .next_arg

    .error_arg:
        push eax
        push edx
        write incorrect_arg
        pop edx
        pop eax
        ccall write_string, eax, edx
        write double_quote
        ccall write_newline
        mov eax, 1
        jmp .end

    .error_exec_state:
        write expr_expected
        mov eax, 1
        jmp .end

exec_file:
        push ebp
        mov ebp, esp
        ccall read_file, [ebp+0x08]
        test eax, eax
        jz .file_error
        push eax
        ccall exec_string, eax, edx
        pop eax
        ccall finish_file, eax
        mov eax, 0
        mov esp, ebp
        pop ebp
        ret

    .file_error:
        write file_error
        ccall write_string, [ebp+0x08], [ebp+0x0c]
        write double_quote
        ccall write_newline
        mov eax, 1
        mov esp, ebp
        pop ebp
        ret

; Executes scheme code
; In: 1) Address of string with scheme code
;     2) Length of string
exec_string:
        push ebp
        mov ebp, esp
        push esi
        push edi

        mov esi, [ebp+0x08]
        mov edi, [ebp+0x0c]
        add edi, esi

    .next:
        mov eax, edi
        sub eax, esi
        ccall parse, esi, eax
        test eax, eax
        jz .ret
        test edx, edx
        jz .ret
        mov esi, edx
        ccall eval, eax
        test edx, edx
        jnz .error
        jmp .next

    .error:
        push edx
        write exec_error_with_arg
        pop edx
        ccall print, edx
        jmp .ret

    .ret:
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret
