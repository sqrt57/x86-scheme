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

arg1 equ ebp+0x08
arg2 equ ebp+0x0c
arg3 equ ebp+0x10
arg4 equ ebp+0x14
arg5 equ ebp+0x18

local1 equ ebp-0x04
local2 equ ebp-0x08
local3 equ ebp-0x0c
local4 equ ebp-0x10
local5 equ ebp-0x14
local6 equ ebp-0x18
local7 equ ebp-0x1c
local8 equ ebp-0x20
local9 equ ebp-0x24
local10 equ ebp-0x28

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
