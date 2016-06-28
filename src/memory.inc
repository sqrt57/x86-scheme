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
        ; Align esi to 4 bytes
        add esi, 3
        and esi, 0xfffffffc
        mov [eax+block.free], esi
        mov eax, edx

        pop edi
        pop esi
        pop ebx
        mov esp, ebp
        pop ebp
        ret

    .out_of_memory:
        write memory_error_oom
        ccall write_newline
        ccall exit, 1

    .toolarge:
        write memory_error_large
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

; Allocates unsigned number on the heap
; In: 1) Value of unsigned number
; Out: Allocated scheme object
memory_create_unsigned:
        ccall memory_alloc, object_size.unsigned
        mov dword [eax+object.tag], object.tag.unsigned
        mov edx, [esp+0x04]
        mov [eax+unsigned.value], edx
        ret

; Allocates string on the heap
; In: 1) Address of string
;     2) Length of string
; Out: Allocated scheme object
; For convenience null byte is added at the end of string
memory_create_string:
        push ebp
        mov ebp, esp
        push edi
        mov eax, [ebp+0x0c]
        add eax, object_size.string + 1
        ccall memory_alloc, eax
        mov edi, eax
        mov dword [edi+object.tag], object.tag.string
        mov edx, [ebp+0x0c]
        mov [edi+string.length], edx
        lea eax, [edi+string.data]
        ccall memcpy, eax, [ebp+0x08], edx
        lea eax, [edi+string.data]
        add eax, [ebp+0x0c]
        mov byte [eax], 0
        mov eax, edi
        pop edi
        mov esp, ebp
        pop ebp
        ret

; Allocates symbol on the heap
; In: 1) Address of symbol name
;     2) Length of symbol name
; Out: Allocated scheme object
; For convenience null byte is added at the end of symbol name
memory_create_symbol:
        push ebp
        mov ebp, esp
        push edi
        mov eax, [ebp+0x0c]
        add eax, object_size.symbol + 1
        ccall memory_alloc, eax
        mov edi, eax
        mov dword [edi+object.tag], object.tag.symbol
        mov edx, [ebp+0x0c]
        mov [edi+symbol.name_length], edx
        lea eax, [edi+symbol.name_data]
        ccall memcpy, eax, [ebp+0x08], edx
        lea eax, [edi+symbol.name_data]
        add eax, [ebp+0x0c]
        mov byte [eax], 0
        mov dword [edi+symbol.value], 0
        mov dword [edi+symbol.next], 0
        mov eax, edi
        pop edi
        mov esp, ebp
        pop ebp
        ret

; Copy block of memory
; In: 1) Destination address
;     2) Source address
;     3) Size of data
; Out: Destination address
memcpy:
        push ebp
        mov ebp, esp
        push esi
        push edi

        mov edi, [ebp+0x08]
        mov esi, [ebp+0x0c]
        mov ecx, [ebp+0x10]
        rep movsb

        mov eax, [ebp+0x08]
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret
}

macro memory_rodata
{
def_string memory_error_oom, 'Error: out of memory'
def_string memory_error_large, 'Error: too large block of memory requested'
}

macro memory_bss
{
}

macro memory_rwdata
{
        mem_freelist    dd  0
        mem_alloclist   dd  0
}

