
; enscheme executable for Win32
; Copyright (c) 2015, Dmitry Grigoryev

format PE console

section '.code' code executable readable

entry platform_start

platform_start:
        push 0                  ; return code
        call [ExitProcess]      ; Terminate program
        
section '.idata' import data readable writable

        dd rva kernel32_ilt, 0, -1, rva kernel32_name, rva kernel32_iat
        dd 0, 0, 0, 0, 0

kernel32_ilt:
        dd rva ExitProcess_name
        dd 0

kernel32_iat:
        ExitProcess dd rva ExitProcess_name
        dd 0
    
        kernel32_name db 'kernel32.dll', 0
        align 2
        ExitProcess_name db 0, 0, "ExitProcess", 0

