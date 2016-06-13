; vim: filetype=asm

; Windows system functions
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

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

