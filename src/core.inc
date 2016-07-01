; vim: filetype=asm

; Scheme interpreter core functionality
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

object.tag = 0x00
object.tag.type_mask = 0xff
object.tag.empty_list = 0x01
object.tag.cons_cell = 0x02
object.tag.symbol = 0x03
object.tag.string = 0x04
object.tag.signed = 0x05
object.tag.unsigned = 0x06
object.tag.bytevector = 0x07

object.tag.fixed = 0x100

object_size.empty_list = 0x04
object_size.cons_cell = 0x0c
object_size.symbol = 0x10
object_size.string = 0x08
object_size.unsigned = 0x08
object_size.signed = 0x08

cons_cell.car = 0x04
cons_cell.cdr = 0x08

symbol.value = 0x04
symbol.next = 0x08
symbol.name_length = 0x0c
symbol.name_data = 0x10

string.length = 0x04
string.data = 0x08

signed.value = 0x04
unsigned.value = 0x04

macro core_code
{
; Executes scheme code
; In: 1) Address of string with scheme code
;     2) Length of string
exec_string:
        push ebp
        mov ebp, esp
        push esi
        push edi

        mov esi, [ebp+0x08]
        mov edi, [ebp+0x0c]
        add edi, esi

    .next:
        mov eax, edi
        sub eax, esi
        ccall parse, esi, eax
        test eax, eax
        jz .ret
        test edx, edx
        jz .ret
        mov esi, edx
        ccall print, eax
        jmp .next

    .ret:
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret

; Parses scheme object from string buffer
; In: 1) Address of buffer
;     2) Length of data in buffer
; Out: EAX - address of obtained scheme object
;      EDX - updated address inside buffer after parsed object
;   If cannot parse returns EAX = EDX = NULL
parse:
        ; ESI - address of next character
        ; [.bufend] - address of end of buffer
        ; [.multichar_start] - address of start of currently parsed literal
        ; [.multichar_length] - length of current literal
        ; [.newlist] - flag "after start of new list"
        ; [.current] - address of current cons cell
        ; [.result] - lexed scheme object
        ; [.list_result] - whole list result
        .bufend equ ebp-0x04
        .multichar_start equ ebp-0x08
        .multichar_length equ ebp-0x0c
        .newlist equ ebp-0x10
        .current equ ebp-0x14
        .result equ ebp-0x1c
        .list_result equ ebp-0x20

        push ebp
        mov ebp, esp
        sub esp, 0x20
        push esi
        push ebx

        mov esi, [ebp+0x08]
        mov eax, esi
        add eax, [ebp+0x0c]
        mov [.bufend], eax
        xor eax, eax
        mov [.result], eax
        mov [.list_result], eax
        mov [.newlist], eax
        mov [.current], eax

    .next:
        mov [.multichar_start], esi
        cmp esi, [.bufend]
        jae .endofbuf
        xor eax, eax
        lodsb
        call .iswhite
        jz .next

        cmp eax, ';'
        je .comment

        cmp eax, '('
        je .leftbr

        cmp eax, ')'
        je .rightbr

        cmp eax, '"'
        je .string

        call .isdigit
        jz .number

        jmp .symbol

    .leftbr:
        mov eax, [.newlist]
        test eax, eax
        jnz .newsublist
        mov dword [.newlist], 1
        mov edx, [.current]
        test edx, edx
        jz .next
        ccall memory_create_cons_cell
        mov edx, [.current]
        mov [.current], eax
        mov ecx, [edx+cons_cell.cdr]
        mov [eax+cons_cell.cdr], ecx
        mov [edx+cons_cell.cdr], eax
        jmp .next

    .newsublist:
        ccall memory_create_cons_cell
        mov edx, [.current]
        mov [.current], eax
        mov [eax+cons_cell.cdr], edx
        test edx, edx
        jz .first_cell
        mov [edx+cons_cell.car], eax
        jmp .next
    .first_cell:
        mov [.list_result], eax
        jmp .next

    .rightbr:
        mov eax, [.newlist]
        xor edx, edx
        mov [.newlist], edx
        test eax, eax
        jnz .empty_list
        mov edx, [.current]
        test edx, edx
        jz .rightbr_error
        mov eax, [edx+cons_cell.cdr]
        mov dword [edx+cons_cell.cdr], empty_list
        mov [.current], eax
        test eax, eax
        jz .done
        jmp .next

    .empty_list:
        mov edx, [.current]
        test edx, edx
        jz .whole_empty_list
        mov dword [edx+cons_cell.car], empty_list
        jmp .next

    .whole_empty_list:
        mov dword [.result], empty_list
        jmp .done

    .string:
        cmp esi, [.bufend]
        jae .string_error
        xor eax, eax
        lodsb
        cmp eax, '"'
        jnz .string
        mov eax, [.multichar_start]
        inc eax
        mov edx, esi
        sub edx, eax
        dec edx
        ccall memory_create_string, eax, edx
        mov [.result], eax
        jmp .process_result

    .number:
        cmp esi, [.bufend]
        jae .done_number_endbuf
        xor eax, eax
        lodsb
        call .isdigit
        jz .number
    .done_number:
        dec esi
    .done_number_endbuf:
        mov eax, [.multichar_start]
        mov edx, esi
        sub edx, eax
        mov [.multichar_start], eax
        mov [.multichar_length], edx
        jmp .parse_number

    .symbol:
        cmp esi, [.bufend]
        jae .done_symbol_endbuf
        xor eax, eax
        lodsb
        cmp eax, '('
        je .done_symbol
        cmp eax, ')'
        je .done_symbol
        cmp eax, ';'
        je .done_symbol
        cmp eax, '"'
        je .done_symbol
        call .iswhite
        jz .done_symbol
        jmp .symbol
    .done_symbol:
        dec esi
    .done_symbol_endbuf:
        mov eax, [.multichar_start]
        mov edx, esi
        sub edx, eax
        ccall get_symbol, eax, edx
        mov [.result], eax
        jmp .process_result

    .rightbr_error:
        write rightbr_error
        jmp .skipbuf

    .leftbr_error:
        write leftbr_error
        jmp .skipbuf

    .string_error:
        write string_error
        jmp .skipbuf

    .endofbuf:
        mov eax, [.newlist]
        test eax, eax
        jnz .leftbr_error
        mov eax, [.current]
        test eax, eax
        jnz .leftbr_error
        jmp .skipbuf

    .skipbuf:
        xor eax, eax
        jmp .ret

    .comment:
        cmp esi, [.bufend]
        jae .endofbuf
        lodsb
        cmp eax, 0x0a
        jz .next
        jmp .comment

    ; Indicates whether character in EAX is whitespace
    ; Return ZF=1 iff it is whitespace
    .iswhite:
        ; We consider 09-0d, 20 as whitespace
        cmp eax, 0x20
        jne .iswhite1
        ret
    .iswhite1:    
        cmp eax, 0x09
        jae .iswhite2
        ret
    .iswhite2:
        cmp eax, 0x0d
        jbe .iswhite3
        ret
    .iswhite3:
        cmp eax, eax
        ret

    ; Indicates whether character in EAX is decimal digit
    ; Return ZF=1 iff it is decimal digit
    .isdigit:
        cmp eax, '0'
        jae .isdigit1
        ret
    .isdigit1:
        cmp eax, '9'
        jbe .isdigit2
        ret
    .isdigit2:
        cmp eax, eax
        ret

    .done:
        mov eax, [.list_result]
        test eax, eax
        jnz .ret
        mov eax, [.result]
        jmp .ret

    .ret:
        mov edx, esi
        pop ebx
        pop esi
        mov esp, ebp
        pop ebp
        ret

    .parse_number:
        push ebx
        push esi
        mov esi, [.multichar_start]
        mov ecx, [.multichar_length]
        xor eax, eax
    .parse_number_digit:
        jcxz .parse_number_end
        movzx ebx, byte [esi]
        sub ebx, '0'
        mov edx, 10
        mul edx
        add eax, ebx
        inc esi
        dec ecx
        jmp .parse_number_digit
    .parse_number_end:
        ccall memory_create_unsigned, eax
        mov [.result], eax
        pop esi
        pop ebx
        jmp .process_result

    .process_result:
        mov eax, [.newlist]
        test eax, eax
        jnz .append_to_newlist
        mov edx, [.current]
        test edx, edx
        jz .done
        ccall memory_create_cons_cell
        mov edx, [.current]
        mov ecx, [edx+cons_cell.cdr]
        mov [eax+cons_cell.cdr], ecx
        mov [edx+cons_cell.cdr], eax
        mov ecx, [.result]
        mov [eax+cons_cell.car], ecx
        mov [.current], eax
        jmp .next

    .append_to_newlist:
        xor eax, eax
        mov [.newlist], eax
        ccall memory_create_cons_cell
        mov edx, [.current]
        mov [.current], eax
        mov ecx, [.result]
        mov [eax+cons_cell.car], ecx
        test edx, edx
        jz .new_whole_list
        mov [edx+cons_cell.car], eax
        mov [eax+cons_cell.cdr], edx
        jmp .next

    .new_whole_list:
        mov [.list_result], eax
        jmp .next

; Prints scheme object to stdout
; In: 1) Address of scheme object
print:
        push ebp
        mov ebp, esp
        mov eax, [ebp+0x08]
        call .inner_print
        ccall write_newline
        mov esp, ebp
        pop ebp
        ret

    .inner_print:
        mov edx, [eax+object.tag]
        and edx, object.tag.type_mask
        cmp edx, object.tag.unsigned
        je .unsigned
        cmp edx, object.tag.string
        je .string
        cmp edx, object.tag.symbol
        je .symbol
        cmp edx, object.tag.empty_list
        je .empty_list
        cmp edx, object.tag.cons_cell
        je .list
        write core_unknown_object
        ret

    .unsigned:
        push ebp
        mov ebp, esp
        .unsigned_buf_end equ ebp
        sub esp, 0x0c
        push edi
        lea edi, [.unsigned_buf_end]
        mov eax, [eax+unsigned.value]
        mov ecx, 10
    .unsigned_next:
        xor edx, edx
        div ecx
        add edx, '0'
        dec edi
        mov byte [edi], dl
        test eax, eax
        jnz .unsigned_next

        lea eax, [.unsigned_buf_end]
        sub eax, edi
        ccall write_string, edi, eax

        pop edi
        mov esp, ebp
        pop ebp
        ret
        
    .string:
        push eax
        write double_quote
        pop eax
        mov ecx, [eax+string.length]
        lea edx, [eax+string.data]
        ccall write_string, edx, ecx
        write double_quote
        ret

    .symbol:
        mov ecx, [eax+symbol.name_length]
        lea edx, [eax+symbol.name_data]
        ccall write_string, edx, ecx
        ret

    .empty_list:
        write empty_list
        ret

    .list:
        push eax
        write leftbr
    .list_next:
        mov eax, [esp]
        mov eax, [eax+cons_cell.car]
        call .inner_print
        pop eax
        mov eax, [eax+cons_cell.cdr]
        test eax, eax
        jz .null_error
        mov edx, [eax+object.tag]
        and edx, object.tag.type_mask
        cmp edx, object.tag.cons_cell
        jnz .finish_list
        push eax
        write space
        jmp .list_next
    .finish_list:
        cmp edx, object.tag.empty_list
        jnz .improper_list
        write rightbr
        ret
    .improper_list:
        push eax
        write space_dot_space
        pop eax
        call .inner_print
        write rightbr
        ret

    .null_error:
        write null_error
        ret

; Find or create symbol by name
; In: 1) Address of name
;     2) Length of name
; Out: Address of symbol
get_symbol:
        push ebp
        mov ebp, esp
        ccall try_find_symbol, [ebp+0x08], [ebp+0x0c]
        test eax, eax
        jnz .done
        write core_creating_symbol
        ccall write_string, [ebp+0x08], [ebp+0x0c]
        write double_quote
        ccall write_newline
        ccall memory_create_symbol, [ebp+0x08], [ebp+0x0c]
    .done:
        mov esp, ebp
        pop ebp
        ret

; Find symbol by name
; In: 1) Address of name
;     2) Length of name
; Out: Address of symbol from collection. Returns null if symbol not found.
try_find_symbol:
        push ebp
        mov ebp, esp
        push esi
        mov esi, [symbols]
    .next:
        test esi, esi
        jz .done
        lea eax, [esi+symbol.name_data]
        mov edx, [esi+symbol.name_length]
        ccall streq, eax, edx, [ebp+0x08], [ebp+0x0c]
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
}

macro core_rodata
{
def_string double_quote, '"'
def_string core_unknown_object, 'Unknown object'
def_string core_creating_symbol, 'Creating symbol: "'
def_string rightbr_error, 'Error: closing bracket without matching ', \
    'opening bracket', 0x0a
def_string leftbr_error, 'Error: bracket open, but not closed', 0x0a
def_string string_error, 'Error: unfinished string literal', 0x0a
def_string empty_list, '()'
def_string leftbr, '('
def_string rightbr, ')'
def_string space, ' '
def_string space_dot_space, ' . '
def_string null_error, ' . *null_reference*)'

align0 4
empty_list: dd object.tag.empty_list or object.tag.fixed
}

macro core_bss
{
}

macro core_rwdata
{
        symbols dd  0
}

