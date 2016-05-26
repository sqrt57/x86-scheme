
; enscheme executable for Win32
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

include 'macros.inc'
include 'proc.inc'

include 'unicode.inc'
include 'core.inc'
include 'runner.inc'

format PE console

section '.code' code executable readable

entry platform_start

platform_start:
        ; Clear direction flag for calling conventions
        cld

        ccall win32_init

        ccall main, [argc], [argv], [argl]

        ; Initialize interpeter
        ccall core_init

        ; Terminate program
        stdcall [ExitProcess], eax

include 'system-win32.inc'
unicode_code
core_code
runner_code

section '.data' data readable writeable
def_string newline, 10

unicode_data
core_data
runner_data

section '.bss' data readable writeable
        hStdout         rd  1
        hStdin          rd  1
        bytes_written   rd  1
        argc            rd  1
        argv16          rd  1
        argv            rd  1
        argl            rd  1
        win32_heap      rd  1

unicode_bss
core_bss
runner_bss
        
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
        dd rva CreateFileA_name
        dd rva CloseHandle_name
        dd rva GetFileSizeEx_name
        dd rva GetLastError_name
        dd rva ReadFile_name
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
        CreateFileA dd rva CreateFileA_name
        CloseHandle dd rva CloseHandle_name
        GetFileSizeEx dd rva GetFileSizeEx_name
        GetLastError dd rva GetLastError_name
        ReadFile dd rva ReadFile_name
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
        align 2
        CreateFileA_name db 0, 0, "CreateFileA", 0
        align 2
        CloseHandle_name db 0, 0, "CloseHandle", 0
        align 2
        GetFileSizeEx_name db 0, 0, "GetFileSizeEx", 0
        align 2
        GetLastError_name db 0, 0, "GetLastError", 0
        align 2
        ReadFile_name db 0, 0, "ReadFile", 0

