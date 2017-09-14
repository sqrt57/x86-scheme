; String messages
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

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
def_string bytevector_repr, ''
def_string environment_repr, '#<Environment>'
def_string continuation_repr, '#<Procedure:continuation>'
def_string native_procedure_repr, '#<Procedure:native>'
def_string native_special_repr, '#<Special:native>'
def_string lambda_repr, '#<Procedure:lambda>'
def_string macro_repr, '#<Special:macro>'
def_string leftbr, '('
def_string rightbr, ')'
def_string true, '#t'
def_string false, '#f'
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
def_string operator_empty_arg, 'Eval error: no value returned for operator', \
    0x0a
def_string operand_many_args, 'Eval error: cannot use multiple values ', \
    'as operand', 0x0a
def_string operand_empty_arg, 'Eval error: no value returned for operand', \
    0x0a
def_string exec_error_with_arg, 'Execution aborted with the following error: '
def_string environment_bindings, \
    'Eval error: error in environment bindings', 0x0a
def_string size_unknown_object, 'Internal error: get_object_dword_size: ', \
    'unknown object', 0x0a
def_string gc_process_block_error, 'Internal error: gc_process: ', \
    'next block is empty', 0x0a
def_string process_unknown_object, 'Internal error: gc_process: ', \
    'unknown object', 0x0a
