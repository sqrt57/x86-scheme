; vim: filetype=asm

; Unicode utilites
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

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
    ; ESI - address of next source char
    ; ECX - number of remaining 16-bit chars
    ; EAX - current char
    ; EDX - length of resulting UTF-8 string
        push esi
        push ecx
        push edx
        mov ecx, ebx
        mov esi, eax
        cld
        xor edx, edx
        xor eax, eax
    .start:
        jcxz .finish
        lodsw
        dec ecx
    .start_loaded:

        ; If code point is in range 00..7F then increase length by one
        cmp eax, 0x7F
        jbe .one

        ; If code point is in range 80..7FF then increase length by two
        cmp eax, 0x7FF
        jbe .two

        ; If code point is in range 800..D7FF or E000..FFFF
        ; then increase length by 3.
        cmp eax, 0xD7FF
        jbe .three
        cmp eax, 0xDFFF
        ja .three

        ; Range D800..DBFF is the second word of a UTF-16 surrogate pair
        ; It is ill-formed, so we ignore it
        cmp eax, 0xDBFF
        jbe .start

        ; Range DC00..DFFF is the first word of a UTF-16 surrogate pair,
        ; so we read next word
        ; If we are at the end of string we ignore ill-formed word
        jcxz .finish
        lodsw
        dec ecx

        ; Second word must be in range D800..DBFF,
        ; then we increase length by 4
        ; Otherwise we ignore previous word and start over with this word
        cmp eax, 0xD7FF
        jbe .start_loaded
        cmp eax, 0xDBFF
        ja .start_loaded

        ; Correct surrogate pair - add 4 to length
        add edx, 4
        jmp .start

    .one:
        inc edx
        jmp .start

    .two:
        add edx, 2
        jmp .start

    .three:
        add edx, 3
        jmp .start

    .finish:
        mov eax, edx
        pop edx
        pop ecx
        pop esi
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
    ; ESI - address of next source char
    ; EDI - address of next result char
    ; ECX - number of remaining 16-bit chars
    ; EAX - current char
    ; EBX - helper for recoding
    ; EDX - number of remaining bytes in result
        push esi
        push edi
        push ecx
        push edx
        push ebx
        mov esi, eax
        mov edi, ecx
        mov ecx, ebx
        cld
        xor eax, eax
    .start:
        jcxz .finish
        lodsw
        dec ecx
    .start_loaded:

        ; If code point is in range 00..7F then output one byte
        cmp eax, 0x7F
        jbe .one

        ; If code point is in range 80..7FF then output two bytes
        cmp eax, 0x7FF
        jbe .two

        ; If code point is in range 800..D7FF or E000..FFFF
        ; then ouput 3 bytes
        cmp eax, 0xD7FF
        jbe .three
        cmp eax, 0xDFFF
        ja .three

        ; Range D800..DBFF is the second word of a UTF-16 surrogate pair
        ; It is ill-formed, so we ignore it
        cmp eax, 0xDBFF
        jbe .start

        ; Range DC00..DFFF is the first word of a UTF-16 surrogate pair,
        ; so we read next word
        ; If we are at the end of string we ignore ill-formed word
        jcxz .finish
        mov ebx, eax        ; Store first word of surrogate pair in EBX
        lodsw
        dec ecx

        ; Second word must be in range D800..DBFF,
        ; then output 4 bytes
        ; Otherwise we ignore previous word and start over with this word
        cmp eax, 0xD7FF
        jbe .start_loaded
        cmp eax, 0xDBFF
        ja .start_loaded

        ; Correct surrogate pair - add 4 to length
        add edx, 4
        jmp .start

    .one:
        cmp edx, 1
        jb .outofmemory
        stosb
        dec edx
        jmp .start

    .two:
        cmp edx, 2
        jb .outofmemory
        stosb
        sub edx, 2
        jmp .start

    .three:
        add edx, 3
        jmp .start

    .outofmemory:
        mov eax, -1
        pop ebx
        pop edx
        pop ecx
        pop edi
        pop esi
        ret

    .finish:
        mov eax, edx
        pop ebx
        pop edx
        mov ecx, edx
        sub ecx, eax
        mov eax, ecx
        pop ecx
        pop edi
        pop esi
        ret

