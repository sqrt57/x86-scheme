
; enscheme executable for Win32
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

format PE console

section '.code' code executable readable

entry platform_start

include 'unicode.inc'

platform_start:
        
        ;; Retrieve standard handles and store them into variables

        push -11                ; nStdHandle = -11 (standard output)
        call [GetStdHandle]     ; Retrieve standard output handle
        mov [hStdout], eax      ; Store it in hStdout

        push -10                ; nStdHandle = -10 (standard input)
        call [GetStdHandle]     ; Retrieve standard output handle
        mov [hStdin], eax       ; Store it in hStdin

        ;; Parse command line
        
        call [GetCommandLineW]  ; Get command line

        push argc               ; pNumArgs - number of arguments goes to argc
        push eax                ; Address of command line
        call [CommandLineToArgvW]   ; Parse command line
        mov [argv], eax         ; arguments list goes to argv

        mov esi, 0
next_arg:
        ; Get next argument and its length
        mov eax, [argv]         ; Load address of argument list into EAX
        mov eax, [eax+4*esi]    ; Load address of next argument into EAX
        mov edx, eax            ; Copy address of next argument into EDX
        call length16           ; Length of argument into EAX
        push eax                ; Length of argument
        push edx                ; Address of argument

        ; Get length of argument converted to UTF-8
        mov ebx, [esp+4]        ; Copy length to EBX
        mov eax, [esp]          ; Copy address of argument to EAX
        call utf16_to_utf8_length   ; UTF-8 length is in EAX
        push eax                ; Length of UTF-8 result

        ; Convert argument to UTF-8
        call [GetProcessHeap]   ; Get default process heap in EAX

        push ebx                ; dwBytes = enough for UTF-8 string
        push 0                  ; dwFlags = 0
        push eax                ; hHeap = default process heap
        call [HeapAlloc]        ; Allocate buffer for converting
        push eax                ; Address of buffer for result

        mov ebx, [esp+12]       ; Length of source string
        mov eax, [esp+8]        ; Address of source string
        mov edx, [esp+4]        ; Length of UTF-8 result
        mov ecx, [esp]          ; Address of UTF-8 result
        call utf16_to_utf8      ; Do the conversion

        ; Output UTF-16 argument
        mov eax, [esp+12]       ; Length of argument to EAX
        mov edx, [esp+8]        ; Address of argument

        push 0                  ; lpOverlapped = NULL
        push bytes_written      ; lpNumberOfBytesWritten
        shl eax, 1
        push eax                ; nNumberOfBytesToWrite
        push edx                ; lpBuffer
        push [hStdout]          ; hFile = standard output
        call [WriteFile]        ; Print string

        push 0                  ; lpOverlapped = NULL
        push bytes_written      ; lpNumberOfBytesWritten
        push newline_len        ; nNumberOfBytesToWrite
        push newline            ; lpBuffer
        push [hStdout]          ; hFile = standard output
        call [WriteFile]        ; Print string

        ; Output argument converted to UTF-8
        mov eax, [esp+4]        ; Length of argument to EAX
        mov edx, [esp]          ; Address of argument

        push 0                  ; lpOverlapped = NULL
        push bytes_written      ; lpNumberOfBytesWritten
        push eax                ; nNumberOfBytesToWrite
        push edx                ; lpBuffer
        push [hStdout]          ; hFile = standard output
        call [WriteFile]        ; Print string

        push 0                  ; lpOverlapped = NULL
        push bytes_written      ; lpNumberOfBytesWritten
        push newline_len        ; nNumberOfBytesToWrite
        push newline            ; lpBuffer
        push [hStdout]          ; hFile = standard output
        call [WriteFile]        ; Print string

        ; Free memory
        call [GetProcessHeap]   ; Get default process heap in EAX

        mov ebx, [esp]          ; Allocated memory block
        push ebx                ; lpMem
        push 0                  ; dwFlags = 0
        push eax                ; hHeap = default process heap
        call [HeapFree]         ; Free memory block

        add esp, 16

        inc esi
        cmp esi, [argc]
        jb next_arg

        push 0                  ; lpOverlapped = NULL
        push bytes_written      ; lpNumberOfBytesWritten
        push hello_len          ; nNumberOfBytesToWrite
        push hello              ; lpBuffer
        push [hStdout]          ; hFile = standard output
        call [WriteFile]        ; Print string

        push 0                  ; return code
        call [ExitProcess]      ; Terminate program

section '.data' data readable writeable
        hello   db "Hello world!", 10
        hello_len = $ - hello
        newline db 10
        newline_len = $ - newline

section '.bss' data readable writeable
        hStdout         rd  1
        hStdin          rd  1
        bytes_written   rd  1
        argc            rd  1
        argv            rd  1
        
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

