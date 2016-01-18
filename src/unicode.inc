; vim: filetype=asm

; Unicode utilites
;
; This file is part of enscheme project.
; Copyright (c) 2015, Dmitry Grigoryev

; Calculates number of 16-bit characters in null-terminated UTF-16 string
; In: EAX - address of null-terminated UTF-16 string
; Out: EAX - length of the string
; Preserves all other registers
length16:
        push ecx
        mov ecx, 0
        jmp .start
    .next:
        inc ecx
    .start:
        cmp word [eax+ecx*2], 0
        jnz .next
        mov eax, ecx
        pop ecx
        ret

; Calculates amount memory needed for converting UTF-16 string to UTF-8 string
; In: EAX - address of UTF-16 string
;     EBX - number of 16-bit characters in the string
; Out: EAX - length of resulting UTF-8 string including extra terminating
;            null byte
; Preserves all other registers including EBX
utf16_to_utf8_length:
    ; ECX is used as a counter for current 16-bit char
    ; EDX is used to load current char
    ; EDI contains length of resulting UTF-8 string
        push ecx
        push edx
        push edi
        xor edi, edi
        xor ecx, ecx
    .start:
        movzx edx, word [eax+ecx*2]
    .start_loaded:

        ; If code point is in range 00..7F then increase length by one
        cmp edx, 0x7F
        jbe .one

        ; If code point is in range 80..7FF then increase length by two
        cmp edx, 0x7FF
        jbe .two

        ; If code point is in range 800..D7FF or E000..FFFF
        ; then increase length by 3.
        cmp edx, 0xD7FF
        jbe .three
        cmp edx, 0xDFFF
        ja .three

        ; Range D800..DBFF is the second word of a UTF-16 surrogate pair
        ; It is ill-formed, so we ignore it
        cmp edx, 0xDBFF
        jbe .next

        ; Range DC00..DFFF is the first word of a UTF-16 surrogate pair,
        ; so we read next word
        ; If we are at the end of string we ignore ill-formed word
        inc ecx
        cmp ecx, ebx
        jz .finish
        movzx edx, word [eax+ecx*2]

        ; Second word must be in range D800..DBFF,
        ; then we increase length by 4
        ; Otherwise we ignore previous word and start over with this word
        cmp edx, 0xD7FF
        jbe .start_loaded
        cmp edx, 0xDBFF
        ja .start_loaded

        ; Correct surrogate pair - add 4 to length
        add edi, 4
        inc ecx
        cmp ecx, ebx
        jnz .start
        jmp .finish

    .one:
        inc edi
        inc ecx
        cmp ecx, ebx
        jnz .start
        jmp .finish

    .two:
        add edi, 2
        inc ecx
        cmp ecx, ebx
        jnz .start
        jmp .finish

    .three:
        add edi, 3
        inc ecx
        cmp ecx, ebx
        jnz .start
        jmp .finish

    .next:
        inc ecx
        cmp ecx, ebx
        jnz .start
        jmp .finish

    .finish:
        mov eax, edi
        pop edi
        pop edx
        pop ecx
        ret

; Converts UTF-16 string to UTF-8 string
; In: EAX - address of UTF-16 string
;     EBX - number of 16-bit characters in the string
;     ECX - address of memory for result
;     EDX - length of memory for result
; Out: EAX - length of UTF-8 string excluding terminating null byte
;            If not enough memory for output returns -1.
; Preserves all other registers
utf16_to_utf8:
        ret

