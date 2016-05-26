; vim: filetype=asm

; Unicode utilites
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

macro unicode_code
{
; Calculates number of 16-bit characters in null-terminated UTF-16 string
; In: 1) address of null-terminated UTF-16 string
; Out: length of the string
; Preserves all other registers
length16:
        mov eax, [esp+4]
        mov ecx, 0
        jmp .start
    .next:
        inc ecx
    .start:
        cmp word [eax+ecx*2], 0
        jnz .next
        mov eax, ecx
        ret

; Calculates amount memory needed for converting UTF-16 string to UTF-8 string
; In: 1) address of UTF-16 string
;     2) number of 16-bit characters in the string
; Out: length of resulting UTF-8 string not including
;       extra terminating null byte
utf16_to_utf8_length:
    ; ESI - address of next source char
    ; ECX - number of remaining 16-bit chars
    ; EAX - current char
    ; EDX - length of resulting UTF-8 string
        push esi
        mov esi, [esp+8]
        mov ecx, [esp+12]
        cld
        xor eax, eax
        mov edx, eax
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
        pop esi
        ret

; Converts UTF-16 string to UTF-8 string
; In: 1) address of UTF-16 string
;     2) number of 16-bit characters in the string
;     3) address of memory for result
;     4) length of memory for result (including terminating null byte)
; Out: Length of UTF-8 string excluding terminating null byte
;            If not enough memory for output returns -1.
utf16_to_utf8:
    ; ESI - address of next source char
    ; EDI - address of next result char
    ; ECX - number of remaining 16-bit chars
    ; EAX - current char
    ; EBX - helper for recoding
    ; EDX - number of remaining bytes in result
        push ebp
        mov ebp, esp
        push esi
        push edi
        push ebx
        mov esi, [ebp+8]
        mov ecx, [ebp+12]
        mov edi, [ebp+16]
        mov edx, [ebp+20]
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
        cmp edx, 4
        jb .outofmemory
        jmp .four

    .finish:
        ; Extra null-terminating byte
        cmp edx, 1
        jb .outofmemory
        xor eax, eax
        stosb

        mov eax, [ebp+20]
        sub eax, edx
    .ret:
        pop ebx
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret

    .one:
        cmp edx, 1
        jb .outofmemory
        stosb
        dec edx
        jmp .start

    .two:
        cmp edx, 2
        jb .outofmemory
        mov ebx, eax
        shr eax, 6
        or eax, 11000000b
        stosb
        mov eax, ebx
        and eax, 00111111b
        or eax, 10000000b
        stosb
        sub edx, 2
        jmp .start

    .three:
        cmp edx, 3
        jb .outofmemory
        mov ebx, eax
        shr eax, 12
        or eax, 11100000b
        stosb
        mov eax, ebx
        shr eax, 6
        and eax, 00111111b
        or eax, 10000000b
        mov eax, ebx
        and eax, 00111111b
        or eax, 10000000b
        stosb
        sub edx, 3
        jmp .start

    .outofmemory:
        mov eax, -1
        jmp .ret

    .four:
        push eax            ; Put away second word
        push ebx            ; Put away first word

        mov eax, ebx
        shr eax, 6
        and eax, 00001111b
        inc eax
        mov ebx, eax
        shr eax, 2
        or eax, 11110000b
        stosb

        mov eax, ebx
        and eax, 00000011b
        shl eax, 4
        mov ebx, [esp]
        shr ebx, 2
        and ebx, 00001111b
        or eax, ebx
        or eax, 10000000b
        stosb

        mov eax, [esp]
        and eax, 00000011b
        shl eax, 4
        mov ebx, [esp+4]
        shr ebx, 6
        and ebx, 00001111b
        or eax, ebx
        or eax, 10000000b
        stosb

        mov eax, [esp+4]
        and eax, 00111111b
        or eax, 10000000b
        stosb

        add esp, 8
        sub edx, 4
        jmp .start
}

macro unicode_data
{
}

macro unicode_bss
{
}

