; Symbols and symbol binding routines
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

; Find or create symbol by name
; In: 1) Address of name
;     2) Length of name
; Out: Address of symbol
get_symbol:
        push ebp
        mov ebp, esp
        ccall try_find_symbol, [symbols], [ebp+0x08], [ebp+0x0c]
        test eax, eax
        jnz .done
        ccall try_find_symbol, builtin_symbols, [ebp+0x08], [ebp+0x0c]
        test eax, eax
        jnz .done
        ccall memory_create_symbol, [ebp+0x08], [ebp+0x0c]
    .done:
        mov esp, ebp
        pop ebp
        ret

; Find symbol by name
; In: 1) Symbols list
;     2) Address of name
;     3) Length of name
; Out: Address of symbol from collection. Returns null if symbol not found.
try_find_symbol:
        push ebp
        mov ebp, esp
        push esi
        mov esi, [ebp+0x08]
    .next:
        test esi, esi
        jz .done
        lea eax, [esi+symbol.name_data]
        mov edx, [esi+symbol.name_length]
        ccall streq, eax, edx, [ebp+0x0c], [ebp+0x10]
        test eax, eax
        jnz .done
        mov esi, [esi+symbol.next]
        jmp .next
    .done:
        mov eax, esi
        pop esi
        mov esp, ebp
        pop ebp
        ret

; Compares two strings for equality
; In: 1) Address of string 1
;     2) Length of string 1
;     3) Address of string 2
;     4) Length of string 2
; Out: EAX=1 if strings are equal, 0 otherwise
streq:
        push ebp
        mov ebp, esp
        push esi
        push edi
        mov ecx, [ebp+0x0c]
        mov edx, [ebp+0x14]
        cmp ecx, edx
        jne .noteq

        mov esi, [ebp+0x08]
        mov edi, [ebp+0x10]
    .next:
        jcxz .eq
        lodsb
        cmp al, byte [edi]
        jne .noteq
        inc edi
        dec ecx
        jmp .next
    .eq:
        mov eax, 1
        jmp .ret
    .noteq:
        xor eax, eax
    .ret:
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret

; Matches formal parameters to operands and creates
; bindings list
; In: 1) formal parameters list
;     2) operands list
; Out: bindings list, NULL if numbers don't match
bind_parameters:
        push ebp
        mov ebp, esp
        push esi
        push edi
        push ebx
        mov esi, [ebp+0x08]
        mov edi, [ebp+0x0c]
        mov ecx, [esi+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je .zero_formals
        cmp ecx, object.tag.symbol
        je .rest_args
        cmp ecx, object.tag.cons_cell
        je .next_formal
        jmp .error
    .zero_formals:
        mov ecx, [edi+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        jne .error
        mov eax, empty_list
        jmp .ret
    .rest_args:
        ccall memory_create_cons_cell
        mov [eax+cons_cell.car], esi
        mov [eax+cons_cell.cdr], edi
        mov ebx, eax
        ccall memory_create_cons_cell
        mov [eax+cons_cell.car], ebx
        mov dword [eax+cons_cell.cdr], empty_list
        mov ebx, eax
        jmp .ret
    .next_formal:
        mov ecx, [edi+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        jne .error
        ccall memory_create_cons_cell
        mov ecx, [esi+cons_cell.car]
        mov [eax+cons_cell.car], ecx
        mov ecx, [edi+cons_cell.car]
        mov [eax+cons_cell.cdr], ecx
        mov ebx, eax
        ccall memory_create_cons_cell
        mov [eax+cons_cell.car], ebx
        mov ebx, eax
        ccall bind_parameters, [esi+cons_cell.cdr], [edi+cons_cell.cdr]
        test eax, eax
        jz .error
        mov [ebx+cons_cell.cdr], eax
        mov eax, ebx
        jmp .ret
    .error:
        xor eax, eax
        jmp .ret
    .ret:
        pop ebx
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret

; Gets value bound to symbol
; In: 1) Symbol
;     2) Environment
; Out: symbol value, NULL if not bound
get_symbol_value:
        push ebp
        mov ebp, esp
        ccall get_symbol_pair, [ebp+0x08], [ebp+0x0c]
        test eax, eax
        jz .not_bound
        mov eax, [eax+cons_cell.cdr]
    .not_bound:
    .ret:
        mov esp, ebp
        pop ebp
        ret

; Gets cons cell with symbol binding
; In: 1) Symbol
;     2) Environment
; Out: pair from bindings list which contains symbol value in cdr
;       NULL if symbol not bound
get_symbol_pair:
        push ebp
        mov ebp, esp
        push esi
        push edi
        push ebx
        mov esi, [ebp+0x0c]
        mov edi, [ebp+0x08]
    .next_env:
        mov ecx, [esi+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.environment
        jne .not_found
        mov edx, [esi+environment.bindings]
        mov esi, [esi+environment.parent]
    .next_binding:
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je .next_env
        cmp ecx, object.tag.cons_cell
        jne .error
        mov eax, [edx+cons_cell.car]
        mov edx, [edx+cons_cell.cdr]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        jne .error
        cmp edi, [eax+cons_cell.car]
        jne .next_binding
        jmp .ret
    .error:
        write environment_bindings
        xor eax, eax
        jmp .ret
    .not_found:
        xor eax, eax
    .ret:
        pop ebx
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret
