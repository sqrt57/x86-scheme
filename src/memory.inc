; vim: filetype=asm

; Memory allocation routines
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

memory_block_size = 1024*1024
block.size = 0x00
block.next = 0x04
block.free = 0x08
block.start = 0x0c

; Memory is allocated in blocks of size memory_block_size
; Block structure:
; 0x00 Block size
; 0x04 Next block address
; 0x08 Address of free memory inside block
; 0x0c Start of data

macro memory_code
{
; Initializes heap to empty
memory_init:
        mov [mem_freelist], 0
        mov [mem_alloclist], 0
        ret

; Allocates memory from heap
; In: 1) Size of memory block
; Out: Address of memory block
;       On error aborts process
memory_alloc:
        push ebp
        mov ebp, esp
        push ebx
        push esi
        push edi

        cmp dword [ebp+8], memory_block_size-block.start
        ja .toolarge

        mov eax, [mem_alloclist]
        test eax, eax
        jnz .next

        call .alloc_block

    .next:
        call .check_block
        jb .doalloc
        call .alloc_block
        
    .doalloc:
        mov [eax+block.free], esi
        mov eax, edx

        pop edi
        pop esi
        pop ebx
        mov esp, ebp
        pop ebp
        ret

    .out_of_memory:
        ccall write_string, memory_error_oom_str, memory_error_oom_len
        ccall write_newline
        ccall exit, 1

    .toolarge:
        ccall write_string, memory_error_large_str, memory_error_large_len
        ccall write_newline
        ccall exit, 1

    .alloc_block:
        ccall malloc, memory_block_size
        test eax, eax
        jz .out_of_memory
        mov dword [eax+block.size], memory_block_size
        mov edx, [mem_alloclist]
        mov [eax+block.next], edx
        lea edx, [eax+block.start]
        mov [eax+block.free], edx
        mov [mem_alloclist], eax
        ret

    .check_block:
        mov edx, [eax+block.free]
        mov ecx, [eax+block.size]
        mov esi, edx
        add esi, [ebp+8]
        cmp esi, ecx
        ret
}

macro memory_rodata
{
def_string memory_error_oom, 'Error: out of memory'
def_string memory_error_large, 'Error: too large block of memory requested'
}

macro memory_bss
{
        mem_freelist rd 1
        mem_alloclist rd 1
}

