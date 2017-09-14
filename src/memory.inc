; Memory routines and garbage collector
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

memory_init:
        mov eax, [mem_cur_block]
        test eax, eax
        jnz .ret
        call alloc_block
    .ret:
        ret

alloc_block:
        mov eax, [mem_freelist]
        test eax, eax
        jnz .have_free
        ccall malloc, memory_block_size
        test eax, eax
        jz .out_of_memory
        add [mem_occupied], memory_block_size
        jmp .init_block
    .have_free:
        mov edx, [eax+block.next]
        mov [mem_freelist], edx
    .init_block:
        mov dword [eax+block.size], memory_block_size
        mov dword [eax+block.next], 0
        lea edx, [eax+block.start]
        mov [eax+block.free], edx
        mov edx, [mem_cur_block]
        test edx, edx
        jz .first
        mov [edx+block.next], eax
        mov [mem_cur_block], eax
        ret
    .first:
        mov [mem_alloclist], eax
        mov [mem_cur_block], eax
        ret

    .out_of_memory:
        write memory_error_oom
        ccall exit, 1

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

        cmp dword [ebp+8], max_object_size
        ja .toolarge

        mov eax, [mem_cur_block]
        mov edx, [eax+block.free]
        add edx, [ebp+0x08]
        mov ecx, [eax+block.size]
        add ecx, eax
        cmp edx, ecx
        jb .doalloc
        call alloc_block

    .doalloc:
        mov esi, [eax+block.free]
        mov edx, esi
        ; Align ecx to 4 bytes
        mov ecx, [ebp+0x08]
        add ecx, 3
        and ecx, 0xfffffffc
        add esi, ecx
        add [mem_allocated], ecx
        mov [eax+block.free], esi
        mov eax, edx

        pop edi
        pop esi
        pop ebx
        mov esp, ebp
        pop ebp
        ret

    .toolarge:
        write memory_error_large
        ccall exit, 1

; Returns address where next object will be allocated
; Out: Free pointer
memory_free_pointer:
        mov eax, [mem_cur_block]
        mov eax, [eax+block.free]
        ret

; Garbage collect if necessary
memory_check_gc:
        mov eax, [mem_allocated]
        cmp eax, [mem_gc_threshold]
        ja .gc
        ret
    .gc:
        ccall memory_gc
        mov eax, [mem_allocated]
        shl eax, 2
        mov [mem_gc_threshold], eax
        ret

; Forced garbage collect
memory_gc:
        push ebp
        mov ebp, esp

        inc [mem_gc_times]
        mov eax, [mem_allocated]
        cmp eax, [mem_max_allocated]
        jbe .notmax
        mov [mem_max_allocated], eax
    .notmax:

        mov eax, [mem_alloclist]
        mov [gc_oldlist], eax
        xor eax, eax
        mov [mem_allocated], eax
        mov [mem_alloclist], eax
        mov [mem_cur_block], eax

        ccall memory_init
        ccall memory_free_pointer
        mov [gc_process_pointer], eax
        mov eax, [mem_cur_block]
        mov [gc_process_block], eax
        ccall gc_copy_roots
    .process:
        ccall memory_free_pointer
        cmp eax, [gc_process_pointer]
        jz .done
        ccall gc_process
        jmp .process

    .done:
        ccall memory_collect_symbols
        ccall memory_old_to_free

        mov esp, ebp
        pop ebp
        ret

; Processes object at GC_PROCESS_POINTER
; Updates all pointers from this object to their live copies,
; copying them if necessary.
; Updates GC_PROCESS_POINTER
gc_process:
        push ebp
        mov ebp, esp
        push esi

        mov esi, [gc_process_pointer]
        mov eax, [gc_process_block]
        cmp esi, [eax+block.free]
        jb .dispatch
        mov eax, [eax+block.next]
        mov [gc_process_block], eax
        test eax, eax
        jz .process_block_error
        lea esi, [eax+block.start]

    .dispatch:
        ccall get_object_dword_size, esi
        shl eax, 2
        add eax, esi
        mov [gc_process_pointer], eax

        mov ecx, [esi+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je .ret
        cmp ecx, object.tag.cons_cell
        je .cons_cell
        cmp ecx, object.tag.symbol
        je .ret
        cmp ecx, object.tag.string
        je .ret
        cmp ecx, object.tag.number
        je .ret
        cmp ecx, object.tag.bytevector
        je .ret
        cmp ecx, object.tag.environment
        je .environment
        cmp ecx, object.tag.continuation
        je .continuation
        cmp ecx, object.tag.native_procedure
        je .ret
        cmp ecx, object.tag.native_special
        je .ret
        cmp ecx, object.tag.lambda
        je .lambda
        cmp ecx, object.tag.macro
        je .macro
        cmp ecx, object.tag.boolean
        je .ret
        write process_unknown_object
        ccall exit, 1
    .cons_cell:
        ccall gc_get_live_object, [esi+cons_cell.car]
        mov [esi+cons_cell.car], eax
        ccall gc_get_live_object, [esi+cons_cell.cdr]
        mov [esi+cons_cell.cdr], eax
        jmp .ret
    .environment:
        ccall gc_get_live_object, [esi+environment.parent]
        mov [esi+environment.parent], eax
        ccall gc_get_live_object, [esi+environment.bindings]
        mov [esi+environment.bindings], eax
        jmp .ret
    .continuation:
        ccall gc_get_live_object, [esi+continuation.next]
        mov [esi+continuation.next], eax
        ccall gc_get_live_object, [esi+continuation.x]
        mov [esi+continuation.x], eax
        ccall gc_get_live_object, [esi+continuation.y]
        mov [esi+continuation.y], eax
        ccall gc_get_live_object, [esi+continuation.z]
        mov [esi+continuation.z], eax
        jmp .ret
    .lambda:
        ccall gc_get_live_object, [esi+lambda.args]
        mov [esi+lambda.args], eax
        ccall gc_get_live_object, [esi+lambda.body]
        mov [esi+lambda.body], eax
        ccall gc_get_live_object, [esi+lambda.environment]
        mov [esi+lambda.environment], eax
        jmp .ret
    .macro:
        ccall gc_get_live_object, [esi+macro.args]
        mov [esi+macro.args], eax
        ccall gc_get_live_object, [esi+macro.body]
        mov [esi+macro.body], eax
        ccall gc_get_live_object, [esi+macro.environment]
        mov [esi+macro.environment], eax
        jmp .ret

    .ret:
        pop esi
        mov esp, ebp
        pop ebp
        ret

    .process_block_error:
        write gc_process_block_error
        ccall exit, 1

; Copies root objects to live memory
gc_copy_roots:
        push ebp
        mov ebp, esp
        push esi

        ccall gc_get_live_object, [global_env]
        mov [global_env], eax

        mov esi, [vms]
    .next_vm:
        test esi, esi
        jz .done_vms
        ccall gc_get_live_object, [esi+vm.current_continuation]
        mov [esi+vm.current_continuation], eax
        ccall gc_get_live_object, [esi+vm.argument]
        mov [esi+vm.argument], eax
        ccall gc_get_live_object, [esi+vm.args]
        mov [esi+vm.args], eax
        ccall gc_get_live_object, [esi+vm.environment]
        mov [esi+vm.environment], eax
        mov esi, [esi+vm.next]
        jmp .next_vm

    .done_vms:
        pop esi
        mov esp, ebp
        pop ebp
        ret

; Returns object size in dwords, rounding up
; In: 1) Address of object
; Out: Size of object in dwords
get_object_dword_size:
        mov eax, [esp+0x04]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je .empty_list
        cmp ecx, object.tag.cons_cell
        je .cons_cell
        cmp ecx, object.tag.symbol
        je .symbol
        cmp ecx, object.tag.string
        je .string
        cmp ecx, object.tag.number
        je .number
        cmp ecx, object.tag.bytevector
        je .bytevector
        cmp ecx, object.tag.environment
        je .environment
        cmp ecx, object.tag.continuation
        je .continuation
        cmp ecx, object.tag.native_procedure
        je .native_procedure
        cmp ecx, object.tag.native_special
        je .native_special
        cmp ecx, object.tag.lambda
        je .lambda
        cmp ecx, object.tag.macro
        je .macro
        cmp ecx, object.tag.boolean
        je .boolean
        write size_unknown_object
        ccall exit, 1
    .empty_list:
        mov eax, object_size.empty_list shr 2
        ret
    .cons_cell:
        mov eax, object_size.cons_cell shr 2
        ret
    .symbol:
        mov eax, [eax+symbol.name_length]
        ; Add 1 for null byte and 3 for padding
        add eax, object_size.symbol+4
        shr eax, 2
        ret
    .string:
        mov eax, [eax+string.length]
        ; Add 1 for null byte and 3 for padding
        add eax, object_size.string+4
        shr eax, 2
        ret
    .number:
        mov eax, object_size.number shr 2
        ret
    .bytevector:
        mov eax, [eax+bytevector.length]
        ; Add 1 for null byte and 3 for padding
        add eax, object_size.bytevector+4
        shr eax, 2
        ret
    .environment:
        mov eax, object_size.environment shr 2
        ret
    .continuation:
        mov eax, object_size.continuation shr 2
        ret
    .native_procedure:
        mov eax, object_size.native_procedure shr 2
        ret
    .native_special:
        mov eax, object_size.native_special shr 2
        ret
    .lambda:
        mov eax, object_size.lambda shr 2
        ret
    .macro:
        mov eax, object_size.macro shr 2
        ret
    .boolean:
        mov eax, object_size.boolean shr 2
        ret

; Copies old object to live memory.
; If the object is already copied just returns new address.
; In: 1) Address of old object
; Out: Address of new live object
gc_get_live_object:
        push ebp
        mov ebp, esp

        mov eax, [arg1]
        test eax, eax
        jz .ret
        mov ecx, [eax+object.tag]
        test ecx, object.tag.fixed
        jnz .ret
        test ecx, object.tag.gc_copied
        jnz .copied
        sub esp, 0x08
        ccall get_object_dword_size, eax
        mov [local1], eax
        shl eax, 2
        ccall memory_alloc, eax
        mov [local2], eax
        ccall memcpy_dw, eax, [arg1], [local1]
        mov edx, [arg1]
        or dword [edx+object.tag], object.tag.gc_copied
        mov eax, [local2]
        mov [edx+object.gc_copy], eax
        jmp .ret

    .copied:
        mov eax, [eax+object.gc_copy]
    .ret:
        mov esp, ebp
        pop ebp
        ret

; Groups live symbols into a list
memory_collect_symbols:
        push ebp
        mov ebp, esp
        push esi
        push edi

        mov esi, [symbols]
        mov edi, 0
        jmp .start

    .next:
        mov esi, [esi+symbol.next]
    .start:
        test esi, esi
        jz .done

        mov ecx, [esi+object.tag]
        test ecx, object.tag.fixed
        jnz .next
        test ecx, object.tag.gc_copied
        jz .next

        mov eax, [esi+object.gc_copy]
        mov [eax+symbol.next], edi
        mov edi, eax
        jmp .next

    .done:
        mov [symbols], edi
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret

; Moves memory blocks from list GC_OLDLIST to MEM_FREELIST
memory_old_to_free:
    .next:
        mov eax, [gc_oldlist]
        test eax, eax
        jz .done
        mov edx, [eax+block.next]
        mov [gc_oldlist], edx
        mov edx, [mem_freelist]
        mov [eax+block.next], edx
        mov [mem_freelist], eax
        jmp .next
    .done:
        ret

; Allocates number on the heap
; In: 1) Value of number number
; Out: Allocated scheme object
memory_create_number:
        ccall memory_alloc, object_size.number
        mov dword [eax+object.tag], object.tag.number
        mov edx, [esp+0x04]
        mov [eax+number.value], edx
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

; Allocates string without content on the heap
; In: 1) Length of string
; Out: Allocated scheme object
; For convenience null byte is added at the end of string
memory_create_string_0:
        push ebp
        mov ebp, esp
        mov eax, [arg1]
        add eax, object_size.string + 1
        ccall memory_alloc, eax
        mov dword [eax+object.tag], object.tag.string
        mov edx, [arg1]
        mov [eax+string.length], edx
        mov byte [eax+string.data], 0
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
        mov eax, [symbols]
        mov dword [edi+symbol.next], eax
        mov [symbols], edi
        mov eax, edi
        pop edi
        mov esp, ebp
        pop ebp
        ret

; Allocates new cons cell
; Out: Allocated scheme object
memory_create_cons_cell:
        ccall memory_alloc, object_size.cons_cell
        mov dword [eax+object.tag], object.tag.cons_cell
        xor edx, edx
        mov [eax+cons_cell.car], edx
        mov [eax+cons_cell.cdr], edx
        ret

; Allocates new continuation
; In: 1) Code field
; Out: Allocated scheme object
memory_create_continuation:
        ccall memory_alloc, object_size.continuation
        mov dword [eax+object.tag], object.tag.continuation
        mov edx, [esp+0x04]
        mov [eax+continuation.code], edx
        xor edx, edx
        mov [eax+continuation.next], edx
        mov [eax+continuation.x], edx
        mov [eax+continuation.y], edx
        mov [eax+continuation.z], edx
        ret

; Allocates new lambda structure
; Out: Allocated scheme object
memory_create_lambda:
        ccall memory_alloc, object_size.lambda
        mov dword [eax+object.tag], object.tag.lambda
        xor edx, edx
        mov [eax+lambda.args], edx
        mov [eax+lambda.body], edx
        mov [eax+lambda.environment], edx
        ret

; Allocates new macro structure
; Out: Allocated scheme object
memory_create_macro:
        ccall memory_alloc, object_size.macro
        mov dword [eax+object.tag], object.tag.macro
        xor edx, edx
        mov [eax+macro.args], edx
        mov [eax+macro.body], edx
        mov [eax+macro.environment], edx
        ret

; Allocates new environment
; Out: Allocated scheme object
memory_create_environment:
        ccall memory_alloc, object_size.environment
        mov dword [eax+object.tag], object.tag.environment
        mov dword [eax+environment.parent], false
        mov dword [eax+environment.bindings], empty_list
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

; Copy block of memory
; In: 1) Destination address
;     2) Source address
;     3) Size of data in dwords
; Out: Destination address
memcpy_dw:
        push ebp
        mov ebp, esp
        push esi
        push edi

        mov edi, [ebp+0x08]
        mov esi, [ebp+0x0c]
        mov ecx, [ebp+0x10]
        rep movsd

        mov eax, [ebp+0x08]
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret
