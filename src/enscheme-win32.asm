
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

section '.bss' data readable writeable
        hStdout         rd  1
        bytes_written   rd  1
        
section '.idata' import data readable writable

        dd rva kernel32_ilt, 0, -1, rva kernel32_name, rva kernel32_iat
        dd 0, 0, 0, 0, 0

kernel32_ilt:
        dd rva ExitProcess_name
        dd rva GetStdHandle_name
        dd rva WriteFile_name
        dd 0

kernel32_iat:
        ExitProcess dd rva ExitProcess_name
        GetStdHandle dd rva GetStdHandle_name
        WriteFile dd rva WriteFile_name
        dd 0
    
        kernel32_name db 'kernel32.dll', 0
        align 2
        ExitProcess_name db 0, 0, "ExitProcess", 0
        align 2
        GetStdHandle_name db 0, 0, "GetStdHandle", 0
        align 2
        WriteFile_name db 0, 0, "WriteFile", 0

