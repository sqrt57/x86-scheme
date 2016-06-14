; vim: filetype=asm

; Scheme interpreter core functionality
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

macro core_code
{
core_init:
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
        jmp .next

    .ret:
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret

; Parses scheme object from string buffer
; In: 1) Address of buffer
;     2) Length of data in buffer
; Out: EAX - address of obtained scheme object
;      EDX - updated address inside buffer after parsed object
;   If cannot parse returns EAX = EDX = NULL
parse:
        ; ESI - address of next character
        ; [.bufend] - address of end of buffer
        ; [.litstart] - address of start of currently parsed literal
        ; [.newlist] - flag "after start of new list"
        ; [.result] - result scheme object
        .bufend equ ebp-0x04
        .litstart equ ebp-0x08
        .newlist equ ebp-0x0c
        .result equ ebp-0x10

        push ebp
        mov ebp, esp
        sub esp, 0x10
        push esi
        push ebx

        mov esi, [ebp+0x08]
        mov eax, esi
        add eax, [ebp+0x0c]
        mov [.bufend], eax
        xor eax, eax
        mov [.result], eax
        mov [.newlist], eax

    .next:
        mov [.litstart], esi
        cmp esi, [.bufend]
        jae .endofbuf
        lodsb
        call .iswhite
        jz .next

        cmp eax, ';'
        je .comment

        cmp eax, '('
        je .leftbr

        cmp eax, ')'
        je .rightbr

        cmp eax, ''''
        je .quote

        cmp eax, '"'
        je .string

        jmp .literal

    .leftbr:
        write core_leftbr
        jmp .next

    .rightbr:
        write core_rightbr
        jmp .next

    .quote:
        write core_quote
        jmp .next

    .string:
        cmp esi, [.bufend]
        jae .endofbuf
        lodsb
        cmp eax, '"'
        jnz .string
        mov eax, [.litstart]
        inc eax
        mov edx, esi
        sub edx, eax
        dec edx
        push eax
        push edx
        write core_string
        pop edx
        pop eax
        ccall write_string, eax, edx
        write core_string_end
        jmp .next

    .literal:
        cmp esi, [.bufend]
        jae .doneliteralnodec
        lodsb
        cmp eax, '('
        je .doneliteral
        cmp eax, ')'
        je .doneliteral
        cmp eax, ''''
        je .doneliteral
        cmp eax, ';'
        je .doneliteral
        cmp eax, '"'
        je .doneliteral
        call .iswhite
        jz .doneliteral
        jmp .literal
    .doneliteral:
        dec esi
    .doneliteralnodec:
        mov eax, [.litstart]
        mov edx, esi
        sub edx, eax
        push eax
        push edx
        write core_literal
        pop edx
        pop eax
        ccall write_string, eax, edx
        write core_string_end
        jmp .next

    .endofbuf:
        xor eax, eax
        mov edx, eax
        jmp .ret

    .done:
        mov eax, [.result]
        mov edx, esi
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

    .ret:
        pop ebx
        pop esi
        mov esp, ebp
        pop ebp
        ret
}

macro core_rodata
{
def_string core_leftbr, 'Left bracket', 0x0a
def_string core_rightbr, 'Right bracket', 0x0a
def_string core_quote, 'Quotation character', 0x0a
def_string core_string, 'String: "'
def_string core_literal, 'Literal: "'
def_string core_string_end, '"', 0x0a
align 4
empty_list: dd 0
}

macro core_bss
{
}

