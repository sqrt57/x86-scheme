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
        push ebp
        mov ebp, esp
        push esi

        ccall core_init
        
        xor esi, esi

    .check_next_arg:
        cmp esi, [ebp+8]
        jb .next_arg
        jmp .end

    .next_arg:
        mov eax, [ebp+12]
        mov edx, [ebp+16]
        ccall write_string, [eax + esi*4], [edx + esi*4]
        ccall write_newline
        inc esi
        
        jmp .check_next_arg

    .end:
        mov eax, 0
        pop esi
        mov esp, ebp
        pop ebp
        ret
}

macro runner_data
{
}

macro runner_bss
{
}

