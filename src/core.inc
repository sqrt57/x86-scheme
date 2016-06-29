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

cons_cell.car = 0x00
cons_cell.cdr = 0x04

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
        ; [.result] - result scheme object
        .bufend equ ebp-0x04
        .multichar_start equ ebp-0x08
        .multichar_length equ ebp-0x0c
        .newlist equ ebp-0x10
        .result equ ebp-0x14

        push ebp
        mov ebp, esp
        sub esp, 0x14
        push esi
        push ebx

        mov esi, [ebp+0x08]
        mov eax, esi
        add eax, [ebp+0x0c]
        mov [.bufend], eax
        xor eax, eax
        mov [.result], eax
        mov [.newlist], eax

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
        write core_leftbr
        jmp .next

    .rightbr:
        write core_rightbr
        jmp .next

    .string:
        cmp esi, [.bufend]
        jae .endofbuf
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
        jmp .done

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
        jmp .done

    .endofbuf:
        xor eax, eax
        mov edx, eax
        jmp .ret

    .done:
        mov eax, [.result]
        mov edx, esi
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

    .ret:
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
        jmp .done

; Prints scheme object to stdout
; In: 1) Address of scheme object
print:
        push ebp
        mov ebp, esp
        call .inner_print
        ccall write_newline
        mov esp, ebp
        pop ebp
        ret

    .inner_print:
        mov eax, [ebp+0x08]
        mov edx, [eax+object.tag]
        cmp edx, object.tag.unsigned
        je .unsigned
        cmp edx, object.tag.string
        je .string
        cmp edx, object.tag.symbol
        je .symbol
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
        write double_quote
        mov eax, [ebp+0x08]
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
def_string core_leftbr, 'Left bracket', 0x0a
def_string core_rightbr, 'Right bracket', 0x0a
def_string core_string, 'String: "'
def_string core_symbol, 'Symbol: "'
def_string core_number, 'Number: '
def_string double_quote, '"'
def_string core_unknown_object, 'Unknown object'
def_string core_creating_symbol, 'Creating symbol: "'

align0 4
empty_list: dd object.tag.empty_list or object.tag.fixed
core_true:  dd 0
core_false: dd 0
}

macro core_bss
{
}

macro core_rwdata
{
        symbols dd  0
}

