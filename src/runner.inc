; vim: filetype=asm

; Scheme interpreter cross-platform runner
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

macro runner_code
{
; Cross-platform entry point
; In: 1) argc - count of command-line arguments
;     2) argv - array of argument adresses
;     3) argl - array of argument lengths
; Out: Result (0 means Ok)
main:
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

        ccall core_init

        xor ebx, ebx
        xor esi, esi

    .next_arg:
        inc esi
        cmp esi, [ebp+8]
        jnb .end_ok

        mov eax, [ebp+12]
        mov eax, [eax + esi*4]
        mov edx, [ebp+16]
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
        push eax
        push edx
        ccall read_file, eax
        test eax, eax
        jz .file_error
        add esp, 8
        ccall exec_string, eax, edx
        jmp .next_arg

    .exec_arg:
        mov ebx, 0
        ccall exec_string, eax, edx
        jmp .next_arg

    .error_arg:
        push eax
        push edx
        ccall write_string, incorrect_arg_str, incorrect_arg_len
        pop edx
        pop eax
        ccall write_string, eax, edx
        ccall write_string, quote_str, quote_len
        ccall write_newline
        mov eax, 1
        jmp .end

    .error_exec_state:
        ccall write_string, expr_expected_str, expr_expected_len
        ccall write_newline
        mov eax, 1
        jmp .end

    .file_error:
        ccall write_string, file_error_str, file_error_len
        pop edx
        pop eax
        ccall write_string, eax, edx
        ccall write_string, quote_str, quote_len
        ccall write_newline
        mov eax, 1
        jmp .end
}

macro runner_rodata
{
def_string incorrect_arg, 'Error: incorrect argument '''
def_string quote, ''''
def_string expr_expected, 'Error: expected scheme expression after ''-e'''
def_string file_error, 'Error reading file '''
}

macro runner_bss
{
}

