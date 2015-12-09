
; enscheme executable for Win32
; Copyright (c) 2015, Dmitry Grigoryev

format PE console

section '.code' code executable readable

entry platform_start

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

        mov ebx, 0
next_arg:
        mov eax, [argv]
        mov eax, [eax+4*ebx]
        mov edx, eax
        call length16

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

        inc ebx
        cmp ebx, [argc]
        jb next_arg

        push 0                  ; lpOverlapped = NULL
        push bytes_written      ; lpNumberOfBytesWritten
        push hello_len          ; nNumberOfBytesToWrite
        push hello              ; lpBuffer
        push [hStdout]          ; hFile = standard output
        call [WriteFile]        ; Print string

        push 0                  ; return code
        call [ExitProcess]      ; Terminate program

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
        dd 0

shell32_ilt:
        dd rva CommandLineToArgvW_name
        dd 0

kernel32_iat:
        ExitProcess dd rva ExitProcess_name
        GetCommandLineW dd rva GetCommandLineW_name
        GetStdHandle dd rva GetStdHandle_name
        WriteFile dd rva WriteFile_name
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

