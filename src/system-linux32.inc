; vim: filetype=asm

; 32-bit Linux system functions
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

; Initializes platform-specific win32 variables
linux32_init:
        push ebp
        mov ebp, esp

        mov esp, ebp
        pop ebp
        ret

; Writes a string to standard output
; In: 1) Address of string
;     2) Length of string
write_string:
        mov eax, 4
        mov ebx, 1
        mov ecx, [esp+4]
        mov edx, [esp+8]
        int 80h
        ret

; Writes new line to standard output
write_newline:
        mov eax, 4
        mov ebx, 1
        mov ecx, newline_str
        mov edx, newline_len
        int 80h
        ret

; Reads a file into memory
; In: 1) File path (null-terminated string)
; Out: EAX - memory address of file data (NULL in case of error)
;      EDX - length of file data
read_file:
        ret

