
; enscheme executable for Win32
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

include 'proc.inc'

include 'unicode.inc'
include 'core.inc'

format PE console

section '.code' code executable readable

entry platform_start

platform_start:
        ; Clear direction flag for calling conventions
        cld
        ; Allocate stack space for locals
        mov ebp, esp
        sub esp, 16
        
        ;; Retrieve standard handles and store them into variables

        ; nStdHandle = -11 (standard output)
        ; Retrieve standard output handle
        stdcall [GetStdHandle], -11
        mov [hStdout], eax      ; Store it in hStdout

        ; nStdHandle = -10 (standard input)
        ; Retrieve standard input handle
        stdcall [GetStdHandle], -10
        mov [hStdin], eax       ; Store it in hStdin

        ;; Initialize interpeter
        ccall core_init

        ; Get command line
        stdcall [GetCommandLineW]

        ; Parse command line
        stdcall [CommandLineToArgvW], eax, argc
        mov [argv], eax         ; arguments list goes to argv

        mov esi, 0

next_arg:
        ; [ebp-4] - number of 16-bit words in UTF-16 argument
        ; [ebp-8] - address of UTF-16 argument
        ; [ebp-12] - length of UTF-8 argumnet
        ; [ebp-16] - address of UTF-8 argument
        ;; Get next argument and its length
        mov eax, [argv]         ; Load address of argument list into EAX
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
        stdcall [GetProcessHeap]
        stdcall [HeapAlloc], eax, 0, [ebp-12]
        mov [ebp-16], eax       ; Address of buffer for result

        ; Convert argument
        ccall utf16_to_utf8, [ebp-8], [ebp-4], [ebp-16], [ebp-12]

        ; Print UTF-16 string
        mov eax, [ebp-4]
        shl eax, 1
        stdcall [WriteFile], [hStdout], [ebp-8], eax, \
            bytes_written, 0

        ; Print newline
        stdcall [WriteFile], [hStdout], newline, newline_len, \
            bytes_written, 0

        ; Print string converted to UTF-8
        stdcall [WriteFile], [hStdout], [ebp-16], [ebp-12], \
            bytes_written, 0

        ; Print newline
        stdcall [WriteFile], [hStdout], newline, newline_len, \
            bytes_written, 0

        ; Free memory
        stdcall [GetProcessHeap]
        stdcall [HeapFree], eax, 0, [ebp-16]

        inc esi
        cmp esi, [argc]
        jb next_arg

        ; Print newline
        stdcall [WriteFile], [hStdout], hello, hello_len, \
            bytes_written, 0

        ; Terminate program
        stdcall [ExitProcess], 0

unicode_code
core_code

section '.data' data readable writeable
        hello   db "Hello world!", 10
        hello_len = $ - hello
        newline db 10
        newline_len = $ - newline

unicode_data
core_data

section '.bss' data readable writeable
        hStdout         rd  1
        hStdin          rd  1
        bytes_written   rd  1
        argc            rd  1
        argv            rd  1

unicode_bss
core_bss
        
section '.idata' import data readable writable

        dd rva kernel32_ilt, 0, -1, rva kernel32_name, rva kernel32_iat
        dd rva shell32_ilt, 0, -1, rva shell32_name, rva shell32_iat
        dd 0, 0, 0, 0, 0

kernel32_ilt:
        dd rva ExitProcess_name
        dd rva GetCommandLineW_name
        dd rva GetStdHandle_name
        dd rva WriteFile_name
        dd rva GetProcessHeap_name
        dd rva HeapAlloc_name
        dd rva HeapFree_name
        dd 0

shell32_ilt:
        dd rva CommandLineToArgvW_name
        dd 0

kernel32_iat:
        ExitProcess dd rva ExitProcess_name
        GetCommandLineW dd rva GetCommandLineW_name
        GetStdHandle dd rva GetStdHandle_name
        WriteFile dd rva WriteFile_name
        GetProcessHeap dd rva GetProcessHeap_name
        HeapAlloc dd rva HeapAlloc_name
        HeapFree dd rva HeapFree_name
        dd 0

shell32_iat:
        CommandLineToArgvW dd rva CommandLineToArgvW_name
        dd 0
    
        kernel32_name db 'kernel32.dll', 0
        shell32_name db 'shell32.dll', 0
        align 2
        ExitProcess_name db 0, 0, "ExitProcess", 0
        align 2
        GetCommandLineW_name db 0, 0, "GetCommandLineW", 0
        align 2
        CommandLineToArgvW_name db 0, 0, "CommandLineToArgvW", 0
        align 2
        GetStdHandle_name db 0, 0, "GetStdHandle", 0
        align 2
        WriteFile_name db 0, 0, "WriteFile", 0
        align 2
        GetProcessHeap_name db 0, 0, "GetProcessHeap", 0
        align 2
        HeapAlloc_name db 0, 0, "HeapAlloc", 0
        align 2
        HeapFree_name db 0, 0, "HeapFree", 0

