; vim: filetype=asm

; Macros for calling procedures
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

;-------------------------------------------
; We use CDECL calling conventions, particularly:
; 1) All arguments are passed on stack
; 2) Arguments are pushed from right to left
; 3) Stack is cleared by caller
; 4) EAX, ECX, EDX are scratch registers and are saved by caller
; 5) EBX, ESP, EBP, ESI, EDI are saved by callee
; 6) Result is returned in EAX and EDX if needed
; 7) Direction flag must be cleared on call and return
; 8) Other arithmetic flags are scratch (must not be saved by callee)
; 
; x87, MMX, XMM, YMM, ZMM register usage is not specified

macro ccall proc, [arg]
{
    common
        size@ccall = 0
    if ~ arg eq
    reverse
        pushd arg
        size@ccall = size@ccall + 4
    common
    end if
        call proc
        if size@ccall
            add esp, size@ccall
        end if
}

macro stdcall proc, [arg]
{
    common
    if ~ arg eq
    reverse
        pushd arg
    common
    end if
    call proc
}


