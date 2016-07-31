; vim: filetype=asm syntax=fasm

; x86-scheme executable for 32-bit Linux
;
; This file is part of x86-scheme project.
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

macro PLATFORM_HEADER
{
format ELF executable 3
}

macro PLATFORM_CODE
{
entry platform_start

segment executable

platform_start:
        ; Clear direction flag for calling conventions
        cld

        pop [argc]
        mov [argv], esp

        ccall linux32_init

        ccall main, [argc], [argv], [argl]

        mov ebx, eax
        mov eax, 1
        int 0x80

; Initializes platform-specific linux variables
linux32_init:
        push ebp
        mov ebp, esp
        push esi

        ccall linux_syscall, 45, 0
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

; Terminates program
; In: 1) exit code
exit:
        mov eax, 1
        mov ebx, [esp+4]
        int 0x80

; Allocates memory using brk
; In: 1) Size of memory
; Out: Address of allocated memory block
;      Returns NULL on error
brk_alloc:
malloc:
        mov eax, [cur_brk]
        add eax, [esp+4]
        ccall linux_syscall, 45, eax
        mov edx, [cur_brk]
        cmp edx, eax
        je .error
        mov [cur_brk], eax
        mov eax, edx
        ret
    .error:
        xor eax, eax
        ret

; Writes a string to standard output
; In: 1) Address of string
;     2) Length of string
write_string:
        push ebp
        mov ebp, esp
        ccall linux_syscall, 4, 1, [ebp+8], [ebp+12]
        mov esp, ebp
        pop ebp
        ret

; Writes new line to standard output
write_newline:
        ccall linux_syscall, 4, 1, newline_str, newline_len
        ret

; mmap2 syscall
; In: 1) address for the new mapping
;     2) length of the mapping
;     3) desired memory protection
;     4) flags
;     5) file descriptor
;     6) offset into file (specified in 4096-bytes pages)
; Out: On success return address of the mapped area
;      On error returns -ERRORCODE, which has range -133..-1
mmap2:
        push ebp
        mov ebp, esp
        ccall linux_syscall, 192, [ebp+8], [ebp+12], [ebp+16],\
                [ebp+20], [ebp+24], [ebp+28]
        mov esp, ebp
        pop ebp
        ret

; Linux syscall
; In: 1) Syscall number
; 2-7) 6 syscall parameters
; Out: syscall result
;      On error returns -ERRORCODE, which has range -133..-1
linux_syscall:
        push ebp
        mov ebp, esp
        syscall_before
        mov eax, [ebp+8]
        mov ebx, [ebp+12]
        mov ecx, [ebp+16]
        mov edx, [ebp+20]
        mov esi, [ebp+24]
        mov edi, [ebp+28]
        mov ebp, [ebp+32]
        int 0x80
        syscall_after
        mov esp, ebp
        pop ebp
        ret

stat64.size equ 0x2c

; Reads a file into memory
; In: 1) File path (null-terminated string)
; Out: EAX - memory address of file data (NULL in case of error)
;      EDX - length of file data
read_file:
        push ebp
        mov ebp, esp
        sub esp, 0x6c
        .handle equ ebp-0x04
        .buffer equ ebp-0x08
        .size equ ebp-0x0c
        .statbuf equ ebp-0x6c

        ; Open file
        ccall linux_syscall, 5, [ebp+0x08], 0, 0
        ; Test if result < 0
        test eax, eax
        js .error
        mov [.handle], eax

        ; stat64 syscall
        lea eax, [.statbuf]
        ccall linux_syscall, 197, [.handle], eax
        test eax, eax
        jnz .errorclose

        ; Check that file size fits into 32 bits
        mov eax, [.statbuf+stat64.size+4]
        test eax, eax
        jnz .errorclose

        ; Check that file size fits into 31 bits
        mov eax, [.statbuf+stat64.size]
        add eax, 4
        test eax, eax
        js .errorclose

        mov [.size], eax

        ; Allocate memory
        ccall linux_syscall, 192, 0, [.size], 0x3, 0x0022, -1, 0
        cmp eax, 0xffffe000
        jae .errorclose
        mov [.buffer], eax
        mov edx, [.size]
        mov [eax], edx

        ; Read contents
        add eax, 4
        ccall linux_syscall, 3, [.handle], eax, [.statbuf+stat64.size]
        cmp eax, [.statbuf+stat64.size]
        jne .errorfree

        ccall linux_syscall, 6, [.handle]
        mov eax, [.buffer]
        add eax, 4
        mov edx, [.statbuf+stat64.size]
        jmp .ret

    .errorfree:
        ccall linux_syscall, 91, [.buffer], [.size]
    .errorclose:
        ccall linux_syscall, 6, [.handle]
    .error:
        xor eax, eax
    .ret:
        mov esp, ebp
        pop ebp
        ret


; Clears memory allocated for file
; In: 1) Address of memory allocated for file
finish_file:
        mov eax, [esp+4]
        sub eax, 4
        mov edx, [eax]
        ccall linux_syscall, 91, eax, edx
        ret
}

macro PLATFORM_RDATA
{
segment readable
}

macro PLATFORM_DATA
{
segment readable writeable
}

macro PLATFORM_BSS
{
segment readable writeable
        argc            rd  1
        argv            rd  1
        argl            rd  1
        cur_brk         rd  1
}

include 'x86-scheme.inc'

