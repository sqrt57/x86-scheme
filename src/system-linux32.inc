; vim: filetype=asm

; 32-bit Linux system functions
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

macro syscall_before
{
        push ebx
        push esi
        push edi
        push ebp
}

macro syscall_after
{
        pop ebp
        pop edi
        pop esi
        pop ebx
}

; Initializes platform-specific win32 variables
linux32_init:
        push ebp
        mov ebp, esp
        push esi

        syscall_before
        mov eax, 45
        mov ebx, 0
        int 0x80
        syscall_after
        mov [cur_brk], eax

        mov eax, [argc]
        shl eax, 4
        ccall brk_alloc, eax
        mov [argl], eax

        xor esi, esi
    .next_arg:
        mov edx, [argv]
        ccall length, [edx+esi*4]
        mov edx, [argl]
        mov [edx+esi*4], eax

        inc esi
        cmp esi, [argc]
        jb .next_arg

        pop esi
        mov esp, ebp
        pop ebp
        ret

; Allocates memory using brk
; In: 1) Size of memory
; Out: Address of allocated memory block
;      Returns NULL on error
brk_alloc:
        push ebp
        mov ebp, esp
        syscall_before
        mov eax, 45
        mov ebx, [cur_brk]
        add ebx, [ebp+8]
        int 0x80
        mov edx, [cur_brk]
        cmp edx, eax
        je .error
        mov [cur_brk], eax
        mov eax, edx
        syscall_after
        mov esp, ebp
        pop ebp
        ret
    .error:
        xor eax, eax
        syscall_after
        mov esp, ebp
        pop ebp
        ret

; Writes a string to standard output
; In: 1) Address of string
;     2) Length of string
write_string:
        push ebp
        mov ebp, esp
        syscall_before
        mov eax, 4
        mov ebx, 1
        mov ecx, [ebp+8]
        mov edx, [ebp+12]
        int 0x80
        syscall_after
        mov esp, ebp
        pop ebp
        ret

; Writes new line to standard output
write_newline:
        push ebp
        mov ebp, esp
        syscall_before
        mov eax, 4
        mov ebx, 1
        mov ecx, newline_str
        mov edx, newline_len
        int 0x80
        syscall_after
        mov esp, ebp
        pop ebp
        ret

; Reads a file into memory
; In: 1) File path (null-terminated string)
; Out: EAX - memory address of file data (NULL in case of error)
;      EDX - length of file data
read_file:
        mov eax, 0
        ret

