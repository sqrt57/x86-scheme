; x86-scheme platform-dependent functions for win32
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2018, Dmitry Grigoryev

platform_start:
        ; Clear direction flag for calling conventions
        cld

        ccall win32_init

        ccall main, [argc], [argv], [argl]

        stdcall [ExitProcess], eax


; Initializes platform-specific win32 variables
win32_init:
        push ebp
        mov ebp, esp
        sub esp, 16
        push esi
        push edi

        ; Retrieve standard output handle
        stdcall [GetStdHandle], -11
        mov [hStdout], eax      ; Store it in hStdout

        ; Retrieve standard input handle
        stdcall [GetStdHandle], -10
        mov [hStdin], eax       ; Store it in hStdin

        ; Get and parse command line
        stdcall [GetCommandLineW]
        stdcall [CommandLineToArgvW], eax, argc
        mov [argv16], eax       ; arguments list goes to argv

        ; Initialize heap
        stdcall [GetProcessHeap]
        mov [win32_heap], eax

        ; Allocate space for argv
        mov esi, [argc]
        shl esi, 2
        stdcall [HeapAlloc], [win32_heap], 0, esi
        mov [argv], eax
        stdcall [HeapAlloc], [win32_heap], 0, esi
        mov [argl], eax

        xor esi, esi

    .next_arg:
        ; [ebp-4] - number of 16-bit words in UTF-16 argument
        ; [ebp-8] - address of UTF-16 argument
        ; [ebp-12] - length of UTF-8 argumnet
        ; [ebp-16] - address of UTF-8 argument
        ;; Get next argument and its length
        mov eax, [argv16]       ; Load address of argument list into EAX
        mov edi, [eax+4*esi]    ; Load address of next argument into EDI
        mov [ebp-8], edi        ; Address of argument
        ccall length16, edi     ; Length of argument into EAX
        mov [ebp-4], eax        ; Length of argument

        ;; Get length of argument converted to UTF-8
        ccall utf16_to_utf8_length, [ebp-8], [ebp-4]
                                ; UTF-8 length is in EAX
        mov [ebp-12], eax       ; Length of UTF-8 result

        ;; Convert argument to UTF-8
        ; Allocate buffer for converting
        mov eax, [ebp-12]
        inc eax                 ; For extra terminating null byte
        stdcall [HeapAlloc], [win32_heap], 0, eax
        mov [ebp-16], eax       ; Address of buffer for result

        ; Convert argument
        mov eax, [ebp-12]
        inc eax                 ; For extra terminating null byte
        ccall utf16_to_utf8, [ebp-8], [ebp-4], [ebp-16], eax

        ; Store address of argument in argv
        mov eax, [argv]
        mov edx, [ebp-16]
        mov [eax+4*esi], edx
        ; Store length of argument in argv
        mov eax, [argl]
        mov edx, [ebp-12]
        mov [eax+4*esi], edx

        inc esi
        cmp esi, [argc]
        jb .next_arg

        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret

; Terminate program
; In: 1) exit code
exit:
        mov eax, [esp+4]
        stdcall [ExitProcess], eax


; Writes a string to standard output
; In: 1) Address of string
;     2) Length of string
write_string:
        mov eax, [esp+4]
        mov edx, [esp+8]
        stdcall [WriteFile], [hStdout], eax, edx, bytes_written, 0
        ret

; Writes new line to standard output
write_newline:
        stdcall [WriteFile], [hStdout], newline_str, newline_len, \
            bytes_written, 0
        ret

; Reads a file into memory
; In: 1) File path (null-terminated string)
; Out: EAX - memory address of file data (NULL in case of error)
;      EDX - length of file data
read_file:
        push ebp
        mov ebp, esp
        sub esp, 24

        .handle equ ebp-0x04
        .buffer equ ebp-0x08
        .size_high equ ebp-0x0c
        .size_low equ ebp-0x10
        .bytes_written equ ebp-0x14
        .result equ ebp-0x1c
        ; Locals:
        ; [EBP-4] File handle
        ; [EBP-8] Allocated memory address
        ; [EBP-12] High doubleword of file size
        ; [EBP-16] Low doubleword of file size
        ; [EBP-20] Number of bytes read
        ; [EBP-24] Result of ReadFile

        stdcall [CreateFileA], [ebp+8], 0x80000000, 1, 0, 3, 0, 0
        mov [.handle], eax
        cmp eax, -1
        je .error

        lea edx, [.size_low]
        stdcall [GetFileSizeEx], eax, edx
        test eax, eax
        jz .error

        cmp dword [.size_high], 0
        jnz .error

        stdcall [HeapAlloc], [win32_heap], 0, [ebp-16]
        mov [.buffer], eax
        test eax, eax
        jz .error

        lea edx, [.bytes_written]
        stdcall [ReadFile], [.handle], [.buffer], [.size_low], edx, 0
        mov [.result], eax
        stdcall [CloseHandle], [.handle]

        cmp dword [.result], 0
        je .error

        mov eax, [.buffer]
        mov edx, [.size_low]
        jmp .ret

    .error:
        mov eax, 0

    .ret:
        mov esp, ebp
        pop ebp
        ret

; Clears memory allocated for file
; In: 1) Address of memory allocated for file
finish_file:
        mov eax, [esp+4]
        stdcall [HeapFree], [win32_heap], 0, eax
        ret

; Allocates memory
; In: 1) Size of memory
; Out: Address of allocated memory block
;       NULL if failed
malloc:
        mov eax, [esp+4]
        stdcall [HeapAlloc], [win32_heap], 0, eax
                ret

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
; TODO Debug this. For now works only for 7-bit ASCII.
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
