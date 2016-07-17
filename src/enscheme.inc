; vim: filetype=asm syntax=fasm

; Scheme interpreter platform-independent core code
;
; This file is part of enscheme project.
; Copyright (c) 2015-2016, Dmitry Grigoryev

builtin_symbols_prev = 0

include 'proc.inc'
include 'macros.inc'

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
object.tag.native_special = 0x0b
object.tag.lambda = 0x0c

object.tag.fixed = 0x100

object_size.empty_list = 0x04
object_size.cons_cell = 0x0c
object_size.symbol = 0x10
object_size.string = 0x08
object_size.number = 0x08
object_size.environment = 0x0c
object_size.continuation = 0x18
object_size.native_procedure = 0x08
object_size.native_special = 0x08
object_size.lambda = 0x10

cons_cell.car = 0x04
cons_cell.cdr = 0x08

symbol.value = 0x04
symbol.next = 0x08
symbol.name_length = 0x0c
symbol.name_data = 0x10

string.length = 0x04
string.data = 0x08

number.value = 0x04

environment.parent = 0x04
environment.bindings = 0x08

continuation.code = 0x04
continuation.next = 0x08
continuation.x = 0x0c
continuation.y = 0x10
continuation.z = 0x14

native_procedure.code = 0x04
native_special.code = 0x04

lambda.args = 0x04
lambda.body = 0x08
lambda.environment = 0x0c

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
        ccall memory_create_signed, eax
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
        cmp edx, object.tag.signed
        je .signed
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
        mov eax, [eax+number.value]
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

    .signed:
        push ebp
        mov ebp, esp
        .signed_buf_end equ ebp
        sub esp, 0x0c
        push edi
        lea edi, [.signed_buf_end]
        mov eax, [eax+number.value]
        test eax, eax
        jns .positive
        not eax
        inc eax
        call .convert_positive
        mov edx, '-'
        dec edi
        mov byte [edi], dl
        jmp .do_print_signed
    .positive:
        call .convert_positive
    .do_print_signed:
        lea eax, [.signed_buf_end]
        sub eax, edi
        ccall write_string, edi, eax
        pop edi
        mov esp, ebp
        pop ebp
        ret

    .convert_positive:
        mov ecx, 10
    .signed_next:
        xor edx, edx
        div ecx
        add edx, '0'
        dec edi
        mov byte [edi], dl
        test eax, eax
        jnz .signed_next
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
        eval_current_continuation equ ebp-0x04
        ; First argument value passed to current continuation
        eval_argument equ ebp-0x08
        ; Rest arguments values passed to current continuation
        eval_args equ ebp-0x0c
        ; First argument goes into eval_argument and rest into eval_args.
        ; Evaluation environment. If NULL then global environment.
        eval_environment equ ebp-0x10

        xor eax, eax
        mov dword [eval_args], empty_list
        mov [eval_environment], eax
        mov eax, [ebp+0x08]
        mov [eval_argument], eax
        ccall memory_create_continuation, .return_code
        mov [eval_current_continuation], eax
        ccall memory_create_continuation, .eval
        mov edx, [eval_current_continuation]
        mov [eax+continuation.next], edx
        mov [eval_current_continuation], eax

    .next:
        mov eax, [eval_current_continuation]
        jmp dword [eax+continuation.code]

    .process_error:
        mov edx, eax
        xor eax, eax
        jmp .ret

    .eval:
        mov eax, [eval_args]
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je .eval_many_args_ok
        mov eax, eval_many_args_str
        jmp .process_error
    .eval_many_args_ok:
        mov eax, [eval_argument]
        test eax, eax
        jz .eval_zero_args
        mov edx, [eax+object.tag]
        and edx, object.tag.type_mask
        jmp dword [.eval_dispatch+edx*4]

    .eval_zero_args:
        write eval_zero_args
        xor eax, eax
        jmp .process_error

    .return_code:
        mov eax, [eval_argument]
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
        ccall print, [eval_argument]
        xor eax, eax
        jmp .process_error

    .self:
        ; Retrieve next continuation passing to it the same argument
        mov eax, [eval_current_continuation]
        mov eax, [eax+continuation.next]
        mov [eval_current_continuation], eax
        jmp .next

    .symbol:
        ; Value bound to symbol becomes argument to next continuation
        mov eax, [eval_argument]
        ccall get_symbol_value, [eval_argument], [eval_environment]
        test eax, eax
        jz .unbound_symbol
        mov [eval_argument], eax
        ; Retrieve next continuation
        mov eax, [eval_current_continuation]
        mov eax, [eax+continuation.next]
        mov [eval_current_continuation], eax
        jmp .next
    .unbound_symbol:
        write unbound_symbol
        mov eax, [eval_argument]
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
        mov edx, [eval_current_continuation]
        mov edx, [edx+continuation.next]
        mov [eax+continuation.next], edx
        ; Store raw argument list in X field
        mov edx, [eval_argument]
        mov edx, [edx+cons_cell.cdr]
        mov [eax+continuation.x], edx
        mov [eval_current_continuation], eax
        ; But before we evaluate CAR of this cons cell
        ccall memory_create_continuation, .eval
        mov edx, [eval_current_continuation]
        mov [eax+continuation.next], edx
        mov [eval_current_continuation], eax
        ; Pass CAR of current cons cell as argument
        mov edx, [eval_argument]
        mov edx, [edx+cons_cell.car]
        mov [eval_argument], edx
        jmp .next

    .apply_procedure_or_special:
        mov eax, [eval_args]
        mov eax, [eax+object.tag]
        and eax, object.tag.type_mask
        cmp eax, object.tag.empty_list
        je .operator_many_args_ok
        write operator_many_args
        xor eax, eax
        jmp .process_error
    .operator_many_args_ok:
        mov eax, [eval_argument]
        test eax, eax
        jz .operator_empty_arg
        mov edx, [eax+object.tag]
        and edx, object.tag.type_mask
        cmp edx, object.tag.native_procedure
        je .process_procedure
        cmp edx, object.tag.lambda
        je .process_procedure
        cmp edx, object.tag.native_special
        je .process_special
        write eval_apply_wrong_type
        xor eax, eax
        jmp .process_error
    .process_procedure:
        mov ebx, eax
        mov edx, [eval_current_continuation]
        mov esi, [edx+continuation.x]
        mov edi, empty_list
        jmp .eval_arg_or_apply
    .process_special:
        mov edi, eax
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.x]
        test ecx, ecx
        jz .special_no_args
        mov edx, [ecx+object.tag]
        and edx, object.tag.type_mask
        cmp edx, object.tag.empty_list
        jz .special_no_args
        cmp edx, object.tag.cons_cell
        jne .error_args_list
        mov eax, [ecx+cons_cell.car]
        mov [eval_argument], eax
        mov eax, [ecx+cons_cell.cdr]
        mov [eval_args], eax
        jmp dword [edi+native_special.code]
    .special_no_args:
        xor eax, eax
        mov [eval_argument], eax
        mov dword [eval_args], empty_list
        jmp dword [edi+native_special.code]
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
        mov edx, [eval_current_continuation]
        mov ecx, [edx+continuation.next]
        mov [eax+continuation.next], ecx
        ; Store procedure in X
        mov [eax+continuation.x], ebx
        ; Store evaluated arguments list in Y
        mov [eax+continuation.y], edi
        ; Store raw argument list in Z
        mov ecx, [esi+cons_cell.cdr]
        mov [eax+continuation.z], ecx
        mov [eval_current_continuation], eax
        ; But before we evaluate CAR of this cons cell
        ccall memory_create_continuation, .eval
        mov edx, [eval_current_continuation]
        mov [eax+continuation.next], edx
        mov [eval_current_continuation], eax
        ; Pass CAR of unevaluated operands list as argument
        mov ecx, [esi+cons_cell.car]
        mov [eval_argument], ecx
        mov dword [eval_args], empty_list
        jmp .next
    .do_apply:
        mov eax, edi
        mov edx, empty_list
        call reverse
        mov edi, eax
        mov ecx, [ebx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.native_procedure
        je .native_procedure_args
        cmp ecx, object.tag.lambda
        je .do_apply_lambda
        mov eax, do_apply_unknown_type_str
        jmp .process_error
    .native_procedure_args:
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        je .native_procedure_many_args
        xor eax, eax
        mov edx, empty_list
        jmp .do_apply_native_procedure
    .native_procedure_many_args:
        mov edx, [eax+cons_cell.cdr]
        mov eax, [eax+cons_cell.car]
    .do_apply_native_procedure:
        call dword [ebx+native_procedure.code]
        test ecx, ecx
        jnz .native_error
        mov [eval_argument], eax
        mov [eval_args], edx
        mov eax, [eval_current_continuation]
        mov eax, [eax+continuation.next]
        mov [eval_current_continuation], eax
        jmp .next
    .native_error:
        mov eax, ecx
        jmp .process_error
    .do_apply_lambda:
        ; EBX - procedure
        ; ESI - procedure formal parameters list
        ; EDI - list of evaluated arguments
        mov esi, [ebx+lambda.args]
        ccall memory_create_environment
        mov edx, [eval_environment]
        mov [eax+environment.parent], edx
        mov [eval_environment], eax
        ccall bind_parameters, esi, edi
        test eax, eax
        jz .lambda_args_count
        mov edx, [eval_environment]
        mov [edx+environment.bindings], eax
        ccall memory_create_continuation, .pop_environment
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        ccall memory_create_continuation, .eval_list
        mov ecx, [eval_current_continuation]
        mov [eax+continuation.next], ecx
        mov ecx, [ebx+lambda.body]
        mov [eax+continuation.x], ecx
        mov [eval_current_continuation], eax
        xor eax, eax
        mov [eval_argument], eax
        mov dword [eval_args], empty_list
        jmp .next
    .lambda_args_count:
        mov eax, lambda_args_count_str
        jmp .process_error

    .eval_list:
        mov eax, [eval_current_continuation]
        mov edx, [eax+continuation.x]
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je .done_eval_list
        cmp ecx, object.tag.cons_cell
        je .eval_list_car
        mov eax, lambda_body_syntax_str
        jmp .process_error
    .done_eval_list:
        mov eax, [eax+continuation.next]
        mov [eval_current_continuation], eax
        jmp .next
    .eval_list_car:
        ccall memory_create_continuation, .eval_list
        mov edx, [eval_current_continuation]
        mov ecx, [edx+continuation.next]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        mov esi, [edx+continuation.x]
        mov ecx, [esi+cons_cell.cdr]
        mov [eax+continuation.x], ecx
        ccall memory_create_continuation, .eval
        mov edx, [eval_current_continuation]
        mov [eax+continuation.next], edx
        mov [eval_current_continuation], eax
        mov eax, [esi+cons_cell.car]
        mov [eval_argument], eax
        mov dword [eval_args], empty_list
        jmp .next

    .pop_environment:
        mov eax, [eval_environment]
        mov eax, [eax+environment.parent]
        mov [eval_environment], eax
        mov eax, [eval_current_continuation]
        mov eax, [eax+continuation.next]
        mov [eval_current_continuation], eax
        jmp .next

    .process_args:
        ; Continuation fields:
        ; X - procedure object
        ; Y - list of evaluated arguments in reverse order
        ; Z - list of rest unevaluated arguments
        mov eax, [eval_args]
        mov eax, [eax+object.tag]
        and eax, object.tag.type_mask
        cmp eax, object.tag.empty_list
        je .process_args_many_args_ok
        write operand_many_args
        xor eax, eax
        jmp .process_error
    .process_args_many_args_ok:
        mov ecx, [eval_argument]
        test ecx, ecx
        jz .process_args_no_arg
        mov edx, [eval_current_continuation]
        mov ebx, [edx+continuation.x]
        mov edi, [edx+continuation.y]
        mov esi, [edx+continuation.z]
        ccall memory_create_cons_cell
        mov ecx, [eval_argument]
        mov [eax+cons_cell.car], ecx
        mov [eax+cons_cell.cdr], edi
        mov edi, eax
        jmp .eval_arg_or_apply
    .empty_args:
        xor edx, edx
        mov [eval_argument], edx
        mov [eval_args], edx
    .process_args_no_arg:
        write operand_empty_arg
        xor eax, eax
        jmp .process_error

    .error_args_list:
        mov eax, eval_error_args_list
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
;     2) Environment, NULL for global environment
; Out: symbol value, NULL if not bound
get_symbol_value:
        push ebp
        mov ebp, esp
        ccall get_symbol_pair, [ebp+0x08], [ebp+0x0c]
        test eax, eax
        jz .global
        mov eax, [eax+cons_cell.cdr]
        jmp .ret
    .global:
        mov eax, [ebp+0x08]
        mov eax, [eax+symbol.value]
    .ret:
        mov esp, ebp
        pop ebp
        ret

; Gets value bound to symbol
; In: 1) Symbol
;     2) Environment, NULL for global environment
; Out: pair from bindings list which contains symbol value in cdr
;       NULL if symbol not boud or bound in global environment
get_symbol_pair:
        push ebp
        mov ebp, esp
        push esi
        push edi
        push ebx
        mov esi, [ebp+0x0c]
        mov edi, [ebp+0x08]
    .next_env:
        test esi, esi
        jz .global_env
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
    .global_env:
        xor eax, eax
    .ret:
        pop ebx
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
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
        ccall memory_alloc, object_size.number
        mov dword [eax+object.tag], object.tag.unsigned
        mov edx, [esp+0x04]
        mov [eax+number.value], edx
        ret

; Allocates signed number on the heap
; In: 1) Value of signed number
; Out: Allocated scheme object
memory_create_signed:
        ccall memory_alloc, object_size.number
        mov dword [eax+object.tag], object.tag.signed
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

; Allocates new environment
; Out: Allocated scheme object
memory_create_environment:
        ccall memory_alloc, object_size.environment
        xor edx, edx
        mov [eax+environment.parent], edx
        mov [eax+environment.bindings], edx
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

; Native scheme functions have the following calling conventions.
; Input:
; EAX - first argument, NULL if zero arguments
; EDX - list of rest of arguments, EMPTY_LIST if zero or one argument
; Output:
; ECX - error object, NULL if everything is Ok
;       If error object is non-null other return values are ignored
; EAX - first result, NULL if zero results
; EDX - list of rest of results, EMPTY_LIST if zero or one argument

; Reverse scheme list
reverse:
        ensure_procedure_args 1
        push ebp
        mov ebp, esp
        push esi
        push edi

        mov esi, eax
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
    .ok:
        mov eax, edi
        mov edx, empty_list
        xor ecx, ecx
    .ret:
        pop edi
        pop esi
        mov esp, ebp
        pop ebp
        ret

scheme_add:
        push edi
        test eax, eax
        jz .done
        mov edi, [eax+object.tag]
        and edi, object.tag.type_mask
        cmp edi, object.tag.unsigned
        je .first_ok
        cmp edi, object.tag.signed
        je .first_ok
        mov ecx, arithmetic_error
        pop edi
        ret
    .first_ok:
        mov eax, [eax+number.value]
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
        pop edi
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
        pop edi
        ret
    .next_ok:
        mov ecx, [ebx+number.value]
        add eax, ecx
        pop ebx
        jmp .next
    .done:
        ccall memory_create_signed, eax
        mov [eax+object.tag], edi
        xor ecx, ecx
        mov edx, empty_list
        pop edi
        ret

scheme_sub:
        push edi
        push esi
        test eax, eax
        jz .done
        mov edi, [eax+object.tag]
        and edi, object.tag.type_mask
        cmp edi, object.tag.unsigned
        je .first_ok
        cmp edi, object.tag.signed
        je .first_ok
        mov ecx, arithmetic_error
        ret
    .first_ok:
        xor esi, esi
        mov eax, [eax+number.value]
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
        pop esi
        pop edi
        ret
    .get_car:
        mov esi, 1
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
        pop esi
        pop edi
        mov ecx, arithmetic_error
        ret
    .next_ok:
        mov ecx, [ebx+number.value]
        sub eax, ecx
        pop ebx
        jmp .next
    .done:
        test esi, esi
        jnz .done1
        ; Exactly one argument - calculate negative of it
        not eax
        inc eax
    .done1:
        pop esi
        ccall memory_create_signed, eax
        mov [eax+object.tag], edi
        pop edi
        xor ecx, ecx
        mov edx, empty_list
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
        mov edx, empty_list
        xor ecx, ecx
        ret
    .empty:
        mov eax, empty_list
        mov edx, empty_list
        xor ecx, ecx
        ret

cons:
        ensure_procedure_args 2
        mov edx, [edx+cons_cell.car]
        push eax
        push edx
        ccall memory_create_cons_cell
        pop edx
        pop ecx
        mov [eax+cons_cell.car], ecx
        mov [eax+cons_cell.cdr], edx
        mov edx, empty_list
        xor ecx, ecx
        ret

car:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        jne pair_expected
        mov eax, [eax+cons_cell.car]
        mov edx, empty_list
        xor ecx, ecx
        ret

cdr:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        jne pair_expected
        mov eax, [eax+cons_cell.cdr]
        mov edx, empty_list
        xor ecx, ecx
        ret

pair_expected:
        mov ecx, pair_expected_str
        ret

number_to_signed:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.signed
        je .ret
        cmp ecx, object.tag.unsigned
        je .convert
        mov ecx, arithmetic_error
        ret
    .convert:
        ccall memory_create_signed, [eax+number.value]
    .ret:
        mov edx, empty_list
        xor ecx, ecx
        ret

number_to_unsigned:
        ensure_procedure_args 1
        mov ecx, [eax+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.unsigned
        je .ret
        cmp ecx, object.tag.signed
        je .convert
        mov ecx, arithmetic_error
        ret
    .convert:
        ccall memory_create_unsigned, [eax+number.value]
    .ret:
        mov edx, empty_list
        xor ecx, ecx
        ret

scheme_eq:
        ensure_procedure_args 2
        mov edx, [edx+cons_cell.car]
        cmp eax, edx
        je .equal
        mov eax, false
        jmp .ret
    .equal:
        mov eax, true
    .ret:
        mov edx, empty_list
        xor ecx, ecx
        ret

; Native special forms have the following API.
; They are working inside stack frame of eval.
; They have access to the following variables (addressed base on EBP,
; so EBP must be preserved when accessing them):
; [eval_argument] - first argument, NULL if zero arguments
; [eval_args] - list of rest of arguments, EMPTY_LIST if zero
;               or one arguments
; [eval_current_continuation] - current continuation, which contains
;               ".apply_procedure_or_special" continuation.
;               Code must at least update current continuation to
;               NEXT field of current continuation
; [eval_environment] - current environment, NULL if global environment
;
; Code may read these variables and update their values.
; When finished, code must jump to "eval.next"
; If an error occured, then code puts error object in EAX
; and jumps to "eval.process_error"

quote:
        ensure_special_args 1, quote_args_str, special_bad_syntax
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eval_current_continuation], ecx
        jmp eval.next

scheme_if:
        ensure_special_args 3, if_args_str, special_bad_syntax
        ccall memory_create_continuation, if_continuation
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        mov edx, [eval_args]
        mov ecx, [edx+cons_cell.car]
        mov [eax+continuation.x], ecx
        mov edx, [edx+cons_cell.cdr]
        mov ecx, [edx+cons_cell.car]
        mov [eax+continuation.y], ecx
        ccall memory_create_continuation, eval.eval
        mov edx, [eval_current_continuation]
        mov [eax+continuation.next], edx
        mov [eval_current_continuation], eax
        mov dword [eval_args], empty_list
        jmp eval.next

if_continuation:
        ensure_special_args_simple 1
        ccall memory_create_continuation, eval.eval
        mov edx, [eval_current_continuation]
        mov ecx, [edx+continuation.next]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        mov ecx, [eval_argument]
        cmp ecx, false
        je .false
        mov ecx, [edx+continuation.x]
        mov [eval_argument], ecx
        jmp eval.next
    .false:
        mov ecx, [edx+continuation.y]
        mov [eval_argument], ecx
        jmp eval.next

lambda:
        mov eax, [eval_argument]
        test eax, eax
        jz .error_no_args
        mov edx, [eval_args]
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.cons_cell
        jne .body_syntax_error
        mov edx, [eval_current_continuation]
        mov edx, [edx+continuation.next]
        mov [eval_current_continuation], edx
        ccall memory_create_lambda
        mov edx, [eval_argument]
        mov [eax+lambda.args], edx
        mov edx, [eval_args]
        mov [eax+lambda.body], edx
        mov edx, [eval_environment]
        mov [eax+lambda.environment], edx
        mov [eval_argument], eax
        mov dword [eval_args], empty_list
        jmp eval.next
    .error_no_args:
        mov eax, lambda_no_args_str
        jmp eval.process_error
    .body_syntax_error:
        mov eax, lambda_body_syntax_error_str
        jmp eval.process_error

scheme_define:
        ensure_special_args 2, define_args_count, define_bad_syntax
        mov ecx, [eval_argument]
        mov ecx, [ecx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.symbol
        je .ok
        mov eax, define_symbol_required
        jmp eval.process_error
    .ok:
        ccall memory_create_continuation, define_continuation
        mov ecx, [eval_current_continuation]
        mov ecx, [ecx+continuation.next]
        mov [eax+continuation.next], ecx
        mov ecx, [eval_argument]
        mov [eax+continuation.x], ecx
        mov [eval_current_continuation], eax
        ccall memory_create_continuation, eval.eval
        mov ecx, [eval_current_continuation]
        mov [eax+continuation.next], ecx
        mov [eval_current_continuation], eax
        mov eax, [eval_args]
        mov eax, [eax+cons_cell.car]
        mov [eval_argument], eax
        mov dword [eval_args], empty_list
        jmp eval.next

define_continuation:
        ensure_special_args_simple 1
        push esi
        push edi
        mov edi, [eval_current_continuation]
        mov ecx, [edi+continuation.next]
        mov [eval_current_continuation], ecx
        mov edi, [edi+continuation.x]
        mov esi, [eval_environment]
        test esi, esi
        jz .global
        mov edx, [esi+environment.bindings]
    .next_binding:
        mov ecx, [edx+object.tag]
        and ecx, object.tag.type_mask
        cmp ecx, object.tag.empty_list
        je .not_found
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
    .already_bound:
        mov eax, define_already_bound
        pop edi
        pop esi
        jmp eval.process_error
    .not_found:
        ccall memory_create_cons_cell
        mov [eax+cons_cell.car], edi
        mov ecx, [eval_argument]
        mov [eax+cons_cell.cdr], ecx
        mov edi, eax
        ccall memory_create_cons_cell
        mov [eax+cons_cell.car], edi
        mov ecx, [esi+environment.bindings]
        mov [eax+cons_cell.cdr], ecx
        mov [esi+environment.bindings], eax
        pop edi
        pop esi
        jmp eval.next
    .global:
        mov eax, [edi+symbol.value]
        test eax, eax
        jnz .already_bound
        mov ecx, [eval_argument]
        mov [edi+symbol.value], ecx
        pop edi
        pop esi
        jmp eval.next
    .error:
        mov eax, environment_error
        pop edi
        pop esi
        jmp eval.process_error

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
def_string exec_error_with_arg, 'Execution aborted with the following error: '
def_string environment_bindings, \
    'Eval error: error in environment bindings', 0x0a

align0 4
empty_list dd object.tag.empty_list or object.tag.fixed
def_proc '+', scheme_add
def_proc '-', scheme_sub
def_proc 'reverse', reverse
def_proc 'list', create_list
def_proc 'cons', cons
def_proc 'car', car
def_proc 'cdr', cdr
def_proc 'number->signed', number_to_signed
def_proc 'number->unsigned', number_to_unsigned
def_proc 'eq?', scheme_eq
def_special 'quote', quote
def_special 'if', scheme_if
def_special 'lambda', lambda
def_special 'define', scheme_define
def_symbol true, '#t', true
def_symbol false, '#f', false
def_scheme_string arithmetic_error, 'non-number in arithmetic operation'
def_scheme_string arg_list_error, 'argument list is not proper list'
def_scheme_string wrong_args_count_str, 'incorrect number of arguments'
def_scheme_string special_bad_syntax, 'syntax error in special form call'
def_scheme_string pair_expected_str, 'pair expected'
def_scheme_string quote_args_str, 'wrong number of  arguments to quote'
def_scheme_string eval_error_args_list, 'syntax error in argument list'
def_scheme_string eval_many_args_str, 'cannot eval many args'
def_scheme_string lambda_no_args_str, 'syntax error in lambda arguments list'
def_scheme_string lambda_body_syntax_error_str, 'syntax error in lambda body'
def_scheme_string do_apply_unknown_type_str, 'cannot apply operator ', \
    'of unknown type'
def_scheme_string lambda_args_count_str, 'wrong number of arguments to lambda'
def_scheme_string lambda_body_syntax_str, 'lambda body must be proper list'
def_scheme_string define_args_count, 'wrong number of arguments to define'
def_scheme_string define_bad_syntax, 'bad syntax of define'
def_scheme_string if_args_str, '3 arguments expected for "if"'
def_scheme_string define_symbol_required, \
        'first argument to define must be a symbol'
def_scheme_string define_already_bound, 'define: symbol already bound'
def_scheme_string environment_error, 'environment structure corrupted'

PLATFORM_DATA

        symbols         dd  0
        mem_freelist    dd  0
        mem_alloclist   dd  0

PLATFORM_BSS

builtin_symbols = builtin_symbols_prev

