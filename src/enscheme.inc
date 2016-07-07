; vim: filetype=asm syntax=fasm

; Scheme interpreter platform-independent core code
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

include 'proc.inc'
include 'macros.inc'

builtin_symbols_prev = 0

object.tag = 0x00
object.tag_bits = 4
object.tag_ceiling = 1 shl object.tag_bits
object.tag.type_mask = (1 shl object.tag_bits) - 1
object.tag.empty_list = 0x01
object.tag.cons_cell = 0x02
object.tag.symbol = 0x03
object.tag.string = 0x04
object.tag.signed = 0x05
object.tag.unsigned = 0x06
object.tag.bytevector = 0x07
object.tag.environment = 0x08
; Continuation, has 4 additional fields
object.tag.continuation = 0x09
object.tag.native_procedure = 0x0a

object.tag.fixed = 0x100

object_size.empty_list = 0x04
object_size.cons_cell = 0x0c
object_size.symbol = 0x10
object_size.string = 0x08
object_size.signed = 0x08
object_size.unsigned = 0x08
object_size.environment = 0x0c
object_size.continuation = 0x18
ovject_size.native_procedure = 0x08

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

environment.parent = 0x04
environment.bindings = 0x08

continuation.code = 0x04
continuation.next = 0x08
continuation.x = 0x0c
continuation.y = 0x10
continuation.z = 0x14

native_procedure.code = 0x04

memory_block_size = 1024*1024
max_object_size = 4*1024
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

PLATFORM_HEADER

PLATFORM_CODE

; Calculates length of 8-bit string
; In: 1) address of null-terminated UTF-8 string
; Out: length of the string
; Preserves all other registers
length:
        mov eax, [esp+4]
        mov ecx, 0
        jmp .start
    .next:
        inc ecx
    .start:
        cmp byte [eax+ecx], 0
        jnz .next
        mov eax, ecx
        ret

; def_string input, '(+ "s")'

; Cross-platform entry point
; In: 1) argc - count of command-line arguments
;     2) argv - array of argument adresses
;     3) argl - array of argument lengths
; Out: Result (0 means Ok)
main:
        .argc equ ebp+0x08
        .argv equ ebp+0x0c
        .argl equ ebp+0x10

        ; ccall parse, input_str, input_len
        ; ccall eval, eax
        ; ccall print, eax

        ; EBX stores current state:
        ;   0: Normal state
        ;   1: After '-e', executes next argument as string
        ;   2: After '--', execute all remaining arguments as files
        ; ESI stores index of current argument
        ;   We skip 0-th argument because it is executable name
        push ebp
        mov ebp, esp
        push ebx
        push esi

        xor ebx, ebx
        xor esi, esi

    .next_arg:
        inc esi
        cmp esi, [.argc]
        jnb .end_ok

        mov eax, [.argv]
        mov eax, [eax + esi*4]
        mov edx, [.argl]
        mov edx, [edx + esi*4]

        cmp ebx, 1
        jz .exec_arg

        cmp ebx, 2
        jz .exec_file

        cmp byte [eax], '-'
        jnz .exec_file

        cmp edx, 2
        jne .error_arg

        cmp byte [eax+1], 'e'
        je .next_expr

        cmp byte [eax+1], '-'
        je .options_done

        jmp .error_arg

    .next_expr:
        mov ebx, 1
        jmp .next_arg

    .options_done:
        mov ebx, 2
        jmp .next_arg

    .end_ok:
        cmp ebx, 1
        jz .error_exec_state
        xor eax, eax
    .end:
        pop esi
        pop ebx
        mov esp, ebp
        pop ebp
        ret

    .exec_file:
        push eax
        push edx
        ccall read_file, eax
        test eax, eax
        jz .file_error
        add esp, 8
        push eax
        ccall exec_string, eax, edx
        pop eax
        ccall finish_file, eax
        jmp .next_arg

    .exec_arg:
        mov ebx, 0
        ccall exec_string, eax, edx
        jmp .next_arg

    .error_arg:
        push eax
        push edx
        write incorrect_arg
        pop edx
        pop eax
        ccall write_string, eax, edx
        write double_quote
        ccall write_newline
        mov eax, 1
        jmp .end

    .error_exec_state:
        write expr_expected
        mov eax, 1
        jmp .end

    .file_error:
        ccall write_string, file_error_str, file_error_len
        pop edx
        pop eax
        ccall write_string, eax, edx
        write double_quote
        ccall write_newline
        mov eax, 1
        jmp .end

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
        ccall eval, eax
        test eax, eax
        jz .error
        ccall print, eax
        jmp .next

    .error:
        push edx
        write exec_error_with_arg
        pop edx
        ccall print, edx
        jmp .ret

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
        ; [.afterdot] - flag "after dot"
        ; [.afterdotobj] - flag "after object after dot"
        ; [.current] - address of current cons cell
        ; [.result] - lexed scheme object
        ; [.list_result] - whole list result
        .bufend equ ebp-0x04
        .multichar_start equ ebp-0x08
        .multichar_length equ ebp-0x0c
        .newlist equ ebp-0x10
        .afterdot equ ebp-0x14
        .afterdotobj equ ebp-0x18
        .current equ ebp-0x1c
        .result equ ebp-0x20
        .list_result equ ebp-0x24

        push ebp
        mov ebp, esp
        sub esp, 0x24
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
        mov [.afterdot], eax
        mov [.afterdotobj], eax
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
        mov eax, [.afterdot]
        test eax, eax
        jnz .error_leftbr_after_dot
        mov eax, [.afterdotobj]
        test eax, eax
        jnz .error_leftbr_after_dotobj
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
        mov eax, [.afterdot]
        test eax, eax
        jnz .error_rightbr_after_dot
        mov eax, [.afterdotobj]
        test eax, eax
        jnz .close_dot_obj
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

    .close_dot_obj:
        mov dword [.afterdotobj], 0
        mov eax, [.current]
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
        cmp edx, 1
        jnz .get_symbol
        cmp byte [eax], '.'
        jnz .get_symbol
        jmp .dot
    .get_symbol:
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
        mov eax, [.afterdot]
        test eax, eax
        jnz .leftbr_error
        mov eax, [.afterdotobj]
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
        mov eax, [.afterdot]
        test eax, eax
        jnz .object_to_cdr
        mov eax, [.afterdotobj]
        test eax, eax
        jnz .error_second_object_after_dot
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

    .object_to_cdr:
        mov dword [.afterdot], 0
        mov dword [.afterdotobj], 1
        mov edx, [.current]
        mov ecx, [edx+cons_cell.cdr]
        mov eax, [.result]
        mov [edx+cons_cell.cdr], eax
        mov [.current], ecx
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

    .dot:
        mov eax, [.newlist]
        test eax, eax
        jnz .error_dot_after_leftbr
        mov eax, [.afterdot]
        test eax, eax
        jnz .error_dot_after_dot
        mov eax, [.afterdotobj]
        test eax, eax
        jnz .error_dot_after_dot
        mov edx, [.current]
        test edx, edx
        jz .error_dot_outside_list
        mov dword [.afterdot], 1
        jmp .next

    .error_dot_after_leftbr:
        write error_dot_after_leftbr
        jmp .skipbuf

    .error_dot_after_dot:
        write error_dot_after_dot
        jmp .skipbuf

    .error_dot_outside_list:
        write error_dot_outside_list
        jmp .skipbuf

    .error_leftbr_after_dot:
        write error_leftbr_after_dot
        jmp .skipbuf

    .error_leftbr_after_dotobj:
        write error_leftbr_after_dotobj
        jmp .skipbuf

    .error_rightbr_after_dot:
        write error_rightbr_after_dot
        jmp .skipbuf

    .error_second_object_after_dot:
        write error_second_object_after_dot
        jmp .skipbuf

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
        test eax, eax
        jz .null
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
        jz .improper_list
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

    .null:
        write null_ref
        ret

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

; Executes scheme object as code
; In: 1) Scheme expression
; Out: Expression result
eval:
        push ebp
        mov ebp, esp
        sub esp, 0x10
        push esi
        push edi
        push ebx

        ; Continuation to execute
        .current_continuation equ ebp-0x04
        ; First argument value passed to current continuation
        .argument equ ebp-0x08
        ; Rest arguments values passed to current continuation
        .args equ ebp-0x0c
        ; First argument goes into .argument and rest into .args.
        ; Evaluation environment. If NULL then global environment.
        .environment equ ebp-0x10

        xor eax, eax
        mov [.args], eax
        mov [.environment], eax
        mov eax, [ebp+0x08]
        mov [.argument], eax
        ccall memory_create_continuation, .return_code
        mov [.current_continuation], eax
        ccall memory_create_continuation, .eval
        mov edx, [.current_continuation]
        mov [eax+continuation.next], edx
        mov [.current_continuation], eax

    .next:
        mov eax, [.current_continuation]
        jmp dword [eax+continuation.code]

    .process_error:
        mov edx, eax
        xor eax, eax
        jmp .ret

    .eval:
        mov eax, [.args]
        test eax, eax
        jnz .eval_many_args
        mov eax, [.argument]
        test eax, eax
        jz .eval_zero_args
        mov edx, [eax+object.tag]
        and edx, object.tag.type_mask
        jmp dword [.eval_dispatch+edx*4]

    .eval_zero_args:
        write eval_zero_args
        xor eax, eax
        jmp .process_error

    .eval_many_args:
        write eval_many_args
        xor eax, eax
        jmp .process_error

    .return_code:
        mov eax, [.argument]
        jmp .ret
        
    .ret:
        pop ebx
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret

    .unknown_type:
        write eval_unknown
        ccall print, [.argument]
        xor eax, eax
        jmp .process_error

    .self:
        ; Retrieve next continuation passing to it the same argument
        mov eax, [.current_continuation]
        mov eax, [eax+continuation.next]
        mov [.current_continuation], eax
        jmp .next

    .symbol:
        ; Value bound to symbol becomes argument to next continuation
        mov eax, [.argument]
        mov edx, [eax+symbol.value]
        test edx, edx
        jz .unbound_symbol
        mov [.argument], edx
        ; Retrieve next continuation
        mov eax, [.current_continuation]
        mov eax, [eax+continuation.next]
        mov [.current_continuation], eax
        jmp .next
    .unbound_symbol:
        write unbound_symbol
        mov eax, [.argument]
        mov ecx, [eax+symbol.name_length]
        lea edx, [eax+symbol.name_data]
        ccall write_string, edx, ecx
        ccall write_newline
        xor eax, eax
        jmp .process_error

    .cons_cell:
        ; Procedure or special form call
        ; Apply procedure or special form to CDR of current cons cell
        ccall memory_create_continuation, .apply_procedure_or_special
        mov edx, [.current_continuation]
        mov edx, [edx+continuation.next]
        mov [eax+continuation.next], edx
        ; Store raw argument list in X field
        mov edx, [.argument]
        mov edx, [edx+cons_cell.cdr]
        mov [eax+continuation.x], edx
        mov [.current_continuation], eax
        ; But before we evaluate CAR of this cons cell
        ccall memory_create_continuation, .eval
        mov edx, [.current_continuation]
        mov [eax+continuation.next], edx
        mov [.current_continuation], eax
        ; Pass CAR of current cons cell as argument
        mov edx, [.argument]
        mov edx, [edx+cons_cell.car]
        mov [.argument], edx
        jmp .next

        db '.apply_procedure_or_special', 0
        align0 4
    .apply_procedure_or_special:
        ; TODO process special forms
        mov eax, [.args]
        test eax, eax
        jz .operator_many_args_ok
        mov eax, [eax+object.tag]
        and eax, object.tag.type_mask
        cmp eax, object.tag.empty_list
        je .operator_many_args_ok
        write operator_many_args
        xor eax, eax
        jmp .process_error
    .operator_many_args_ok:
        mov eax, [.argument]
        test eax, eax
        jz .operator_empty_arg
        mov edx, [eax+object.tag]
        and edx, object.tag.type_mask
        cmp edx, object.tag.native_procedure
        je .process_procedure
        write eval_apply_wrong_type
        xor eax, eax
        jmp .process_error
    .process_procedure:
        mov ebx, eax
        mov edx, [.current_continuation]
        mov esi, [edx+continuation.x]
        mov edi, empty_list
        jmp .eval_arg_or_apply
    .operator_empty_arg:
        write operator_empty_arg
        xor eax, eax
        jmp .process_error

        ; In:
        ; EBX - procedure
        ; ESI - (rest) unevaluated arguments list
        ; EDI - reversed list of evaluated arguments
    .eval_arg_or_apply:
        test esi, esi
        jz .do_apply
        mov ecx, [esi+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je .do_apply
        cmp ecx, object.tag.cons_cell
        jne .error_args_list
        ccall memory_create_continuation, .process_args
        mov edx, [.current_continuation]
        mov ecx, [edx+continuation.next]
        mov [eax+continuation.next], ecx
        ; Store procedure in X
        mov [eax+continuation.x], ebx
        ; Store evaluated arguments list in Y
        mov [eax+continuation.y], edi
        ; Store raw argument list in Z
        mov ecx, [esi+cons_cell.cdr]
        mov [eax+continuation.z], ecx
        mov [.current_continuation], eax
        ; But before we evaluate CAR of this cons cell
        ccall memory_create_continuation, .eval
        mov edx, [.current_continuation]
        mov [eax+continuation.next], edx
        mov [.current_continuation], eax
        ; Pass CAR of unevaluated operands list as argument
        mov ecx, [esi+cons_cell.car]
        mov [.argument], ecx
        mov dword [.args], 0
        jmp .next
        db '.do_apply', 0
        align0 4
    .do_apply:
        mov eax, edi
        xor edx, edx
        call reverse
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        je .do_apply_many_args
        xor eax, eax
        xor edx, edx
        jmp .do_apply_select
    .do_apply_many_args:
        mov edx, [eax+cons_cell.cdr]
        mov eax, [eax+cons_cell.car]
    .do_apply_select:
        mov ecx, [ebx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.native_procedure
        je .do_apply_native_procedure
        write do_apply_unknown_type
        xor eax, eax
        jmp .process_error
    .do_apply_native_procedure:
        call dword [ebx+native_procedure.code]
        test ecx, ecx
        jnz .native_error
        mov [.argument], eax
        mov [.args], edx
        mov eax, [.current_continuation]
        mov eax, [eax+continuation.next]
        mov [.current_continuation], eax
        jmp .next
    .native_error:
        mov eax, ecx
        jmp .process_error
        
        db '.process_args', 0
        align0 4
    .process_args:
        ; Continuation fields:
        ; X - procedure object
        ; Y - list of evaluated arguments in reverse order
        ; Z - list of rest unevaluated arguments
        mov eax, [.args]
        test eax, eax
        jz .process_args_many_args_ok
        mov eax, [eax+object.tag]
        and eax, object.tag.type_mask
        cmp eax, object.tag.empty_list
        je .process_args_many_args_ok
        write operand_many_args
        xor eax, eax
        jmp .process_error
    .process_args_many_args_ok:
        mov ecx, [.argument]
        test ecx, ecx
        jz .process_args_no_arg
        mov edx, [.current_continuation]
        mov ebx, [edx+continuation.x]
        mov edi, [edx+continuation.y]
        mov esi, [edx+continuation.z]
        ccall memory_create_cons_cell
        mov ecx, [.argument]
        mov [eax+cons_cell.car], ecx
        mov [eax+cons_cell.cdr], edi
        mov edi, eax
        jmp .eval_arg_or_apply
    .empty_args:
        xor edx, edx
        mov [.argument], edx
        mov [.args], edx
    .process_args_no_arg:
        write operand_empty_arg
        xor eax, eax
        jmp .process_error

    .error_args_list:
        write eval_error_args_list
        xor eax, eax
        jmp .process_error
         
    align0 4
    .eval_dispatch:
        dd  .unknown_type   ; Zero tag - incorrect object
        dd  .unknown_type   ; Empty list
        dd  .cons_cell
        dd  .symbol
        dd  .self           ; String
        dd  .self           ; Signed number
        dd  .self           ; Unsigned number
        dd  .self           ; Bytevector
        dd  .unknown_type   ; Environment
        dd  .unknown_type   ; Continuation
        dd  .unknown_type   ; Native procedure
        ; Pad table to object.tag_ceiling entries
        dd  object.tag_ceiling-($-.eval_dispatch)/4  dup(.unknown_type)

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

        mov eax, [mem_alloclist]
        test eax, eax
        jnz .next

        call .alloc_block

    .next:
        call .check_block
        jb .doalloc
        call .alloc_block

    .doalloc:
        mov esi, [eax+block.free]
        mov edx, esi
        add esi, [ebp+0x08]
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
        ccall exit, 1

    .toolarge:
        write memory_error_large
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
        add edx, [ebp+0x08]
        mov ecx, [eax+block.size]
        add ecx, eax
        cmp edx, ecx
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

hello_world:
        write hello_world
        mov eax, some_result
        xor edx, edx
        ret

; Reverse scheme list
reverse:
        push ebp
        mov ebp, esp
        push esi
        push edi

        test edx, edx
        jz .start
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je .start
        jmp .error
    .start:
        mov esi, eax
        test esi, esi
        jz .error
        mov edi, empty_list
    .next:
        mov ecx, [esi+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        jne .ok
        ccall memory_create_cons_cell
        mov ecx, [esi+cons_cell.car]
        mov [eax+cons_cell.car], ecx
        mov [eax+cons_cell.cdr], edi
        mov edi, eax
        mov esi, [esi+cons_cell.cdr]
        jmp .next

    .error:
        mov ecx, wrong_args_count
        jmp .ret
    .ok:
        mov eax, edi
        xor edx, edx
        xor ecx, ecx
    .ret:
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret

        db 'scheme_add', 0
        align0 4
scheme_add:
        test eax, eax
        jz .done
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.unsigned
        je .first_ok
        cmp ecx, object.tag.signed
        je .first_ok
        mov ecx, arithmetic_error
        ret
    .first_ok:
        mov eax, [eax+unsigned.value]
    .next:
        test edx, edx
        jz .done
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        je .get_car
        cmp ecx, object.tag.empty_list
        je .done
        mov ecx, arg_list_error
        ret
    .get_car:
        push ebx
        mov ebx, [edx+cons_cell.car]
        mov edx, [edx+cons_cell.cdr]
        mov ecx, [ebx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.unsigned
        je .next_ok
        cmp ecx, object.tag.signed
        je .next_ok
        pop ebx
        mov ecx, arithmetic_error
        ret
    .next_ok:
        mov ecx, [ebx+unsigned.value]
        add eax, ecx
        pop ebx
        jmp .next
    .done:
        ccall memory_create_unsigned, eax
        xor ecx, ecx
        xor edx, edx
        ret

scheme_sub:
        test eax, eax
        jz .done
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.unsigned
        je .first_ok
        cmp ecx, object.tag.signed
        je .first_ok
        mov ecx, arithmetic_error
        ret
    .first_ok:
        mov eax, [eax+unsigned.value]
    .next:
        test edx, edx
        jz .done
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        je .get_car
        cmp ecx, object.tag.empty_list
        je .done
        mov ecx, arg_list_error
        ret
    .get_car:
        push ebx
        mov ebx, [edx+cons_cell.car]
        mov edx, [edx+cons_cell.cdr]
        mov ecx, [ebx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.unsigned
        je .next_ok
        cmp ecx, object.tag.signed
        je .next_ok
        pop ebx
        mov ecx, arithmetic_error
        ret
    .next_ok:
        mov ecx, [ebx+unsigned.value]
        sub eax, ecx
        pop ebx
        jmp .next
    .done:
        ccall memory_create_unsigned, eax
        xor ecx, ecx
        xor edx, edx
        ret

create_list:
        test eax, eax
        jz .empty
        test edx, edx
        jnz .next
        mov edx, empty_list
    .next:
        push eax
        push edx
        ccall memory_create_cons_cell
        pop edx
        pop ecx
        mov [eax+cons_cell.car], ecx
        mov [eax+cons_cell.cdr], edx
        xor edx, edx
        xor ecx, ecx
        ret
    .empty:
        mov eax, empty_list
        xor edx, edx
        xor ecx, ecx
        ret

cons:
        xor edx, edx
        xor ecx, ecx
        ret
        test eax, eax
        jz .wrong_args_count
    .wrong_args_count:
        mov ecx, wrong_args_count
        ret

PLATFORM_RDATA

def_string newline, 0x0a
def_string incorrect_arg, 'Error: incorrect command-line argument "'
def_string expr_expected, 'Error: expected scheme expression ', \
    'after "-e" in command-line arguments', 0x0a
def_string file_error, 'Error reading file "'
def_string double_quote, '"'
def_string core_unknown_object, 'Unknown object'
def_string rightbr_error, 'Parse error: closing bracket without matching ', \
    'opening bracket', 0x0a
def_string leftbr_error, 'Parse error: bracket open, but not closed', 0x0a
def_string string_error, 'Parse error: unfinished string literal', 0x0a
def_string empty_list, '()'
def_string leftbr, '('
def_string rightbr, ')'
def_string space, ' '
def_string space_dot_space, ' . '
def_string null_ref, '*NULL_REFERENCE*'
def_string error_dot_after_leftbr, 'Parse error: dot after left bracket', 0x0a
def_string error_dot_after_dot, 'Parse error: dot after dot', 0x0a
def_string error_dot_outside_list, 'Parse error: dot outside list', 0x0a
def_string error_leftbr_after_dot, 'Parse error: list nested inside ', \
    'dotted cdr not supported', 0x0a
def_string error_leftbr_after_dotobj, 'Parse error: right bracket expected ', \
    'after dot and scheme object', 0x0a
def_string error_rightbr_after_dot, 'Parse error: scheme object expected ', \
    'after dot', 0x0a
def_string error_second_object_after_dot, 'Parse error: scheme object ', \
    'expected after dot', 0x0a
def_string memory_error_oom, 'Memory error: out of memory', 0x0a
def_string memory_error_large, 'Memory error: too large block of memory ', \
    'requested', 0x0a
def_string eval_zero_args, 'Eval error: required one argument to eval', 0x0a
def_string eval_many_args, 'Eval error: cannot eval many args', 0x0a
def_string eval_unknown, 'Eval error: cannot execute expression: '
def_string unbound_symbol, 'Eval error: unbound symbol: '
def_string eval_apply_wrong_type, 'Eval error: cannot apply object ', \
    'because of wrong type', 0x0a
def_string operator_many_args, 'Eval error: cannot apply multiple values ', \
    'as operator', 0x0a
def_string operator_empty_arg, 'Eval error: no value returned for operator', \
    0x0a
def_string operand_many_args, 'Eval error: cannot use multiple values ', \
    'as operand', 0x0a
def_string operand_empty_arg, 'Eval error: no value returned for operand', \
    0x0a
def_string do_apply_unknown_type, 'Eval error: cannot apply operator ', \
    'of unknown type', 0x0a
def_string exec_error_with_arg, 'Execution aborted with the following error: '
def_string eval_error_args_list, 'Syntax error in argument list', 0x0a
def_string hello_world, 'Hello, world!', 0x0a

align0 4
empty_list: dd object.tag.empty_list or object.tag.fixed
def_proc 'hello', hello_world
def_proc '+', scheme_add
def_proc '-', scheme_sub
def_proc 'reverse', reverse
def_proc 'list', create_list
def_proc 'cons', cons
def_symbol 'qwe', qwerty
def_symbol 'asd', qwerty
def_scheme_string qwerty, 'Qwerty'
def_scheme_string some_result, 'Some result'
def_scheme_string arithmetic_error, 'non-number in arithmetic operation'
def_scheme_string arg_list_error, 'argument list is not proper list'
def_scheme_string wrong_args_count, 'incorrect number of arguments'
builtin_symbols = builtin_symbols_prev

PLATFORM_DATA

        symbols         dd  0
        mem_freelist    dd  0
        mem_alloclist   dd  0

PLATFORM_BSS

