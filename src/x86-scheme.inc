; Overall platform-independent x86-scheme structure
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

builtin_symbols_prev = 0

include 'proc.inc'
include 'macros.inc'
include 'data-struct.inc'

PLATFORM_HEADER

PLATFORM_CODE

include 'main.inc'
include 'parse.inc'
include 'print.inc'
include 'symbols.inc'
include 'eval.inc'
include 'memory.inc'
include 'scheme-functions.inc'
include 'scheme-specials.inc'

PLATFORM_RDATA

include 'messages.inc'
include 'scheme-defines.inc'

PLATFORM_DATA

        symbols                 dd  0
        vms                     dd  0
        mem_freelist            dd  0
        mem_alloclist           dd  0
        mem_cur_block           dd  0
        gc_oldlist              dd  0
        gc_process_block        dd  0
        gc_process_pointer      dd  0
        mem_allocated           dd  0
        mem_occupied            dd  0
        mem_gc_threshold        dd  0x1000
        mem_gc_times            dd  0
        mem_max_allocated       dd  0
        global_env              dd  global_env_init

global_env_init:
        dd  object.tag.environment
        dd  false
        dd  binding_list

builtin_symbols_bindings

PLATFORM_BSS

builtin_symbols = builtin_symbols_prev
