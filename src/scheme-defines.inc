; Definitions going to global Scheme environment
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

align0 4

empty_list dd object.tag.empty_list or object.tag.fixed

def_proc '+2', scheme_add
def_proc '-1', scheme_negate
def_proc 'reverse', reverse
def_proc 'cons', cons
def_proc 'car', car
def_proc 'cdr', cdr
def_proc 'set-car!', set_car
def_proc 'set-cdr!', set_cdr
def_proc 'pair?', pairp
def_proc 'symbol?', symbolp
def_proc 'number?', numberp
def_proc 'string?', stringp
def_proc 'null?', nullp
def_proc 'environment?', environmentp
def_proc 'continuation?', continuationp
def_proc 'native-procedure?', native_procedurep
def_proc 'native-special?', native_specialp
def_proc 'lambda?', lambdap
def_proc 'macro?', macrop
def_proc 'eq?', scheme_eq
def_proc 'not', scheme_not
def_proc '=', arithmetic_equals
def_proc '<', arithmetic_less
def_proc '>', arithmetic_greater
def_proc 'string=?', string_eq
def_proc 'string->symbol', string_to_symbol
def_proc 'symbol->string', symbol_to_string
def_proc 'string-append-2', string_append_2
def_proc 'global-environment', global_environment
def_proc 'create-environment', create_environment
def_proc 'environment-parent', environment_parent
def_proc 'environment-bindings', environment_bindings
def_proc 'set-environment-parent!', set_environment_parent
def_proc 'set-environment-bindings!', set_environment_bindings
def_proc 'mem-allocated', mem_allocated_proc
def_proc 'mem-occupied', mem_occupied_proc
def_proc 'mem-gc-times', mem_gc_times_proc
def_proc 'mem-max-allocated', mem_max_allocated_proc
def_proc 'write', scheme_write
def_proc 'include', scheme_include
def_proc 'gc', scheme_gc
def_proc 'eval', scheme_eval
def_proc 'apply', apply
def_proc 'error', error

def_special 'quote', quote
def_special 'if', scheme_if
def_special 'lambda', lambda
def_special 'macro', scheme_macro
def_special 'builtin-define', scheme_define
def_special 'set!', scheme_set
def_special 'eval-current-environment', scheme_eval_current_environment
def_special 'call-with-current-continuation', call_cc
def_special 'call/cc', call_cc
def_special 'begin', begin
def_special 'current-environment', current_environment

def_symbol true, '#t', true
def_symbol false, '#f', false
def_symbol_unbound symbol_linux, 'linux32'
def_symbol_unbound symbol_win32, 'win32'
def_symbol symbol_platform, '*platform*', current_platform

def_scheme_string arithmetic_error, 'non-number in arithmetic operation'
def_scheme_string not_a_string_error, 'non-string in string operation'
def_scheme_string not_a_symbol_error, 'non-symbol in symbol operation'
def_scheme_string arg_list_error, 'argument list is not proper list'
def_scheme_string wrong_args_count_str, 'incorrect number of arguments'
def_scheme_string special_bad_syntax, 'syntax error in special form call'
def_scheme_string pair_expected_str, 'pair expected'
def_scheme_string environment_expected_str, 'environment expected'
def_scheme_string quote_args_str, 'wrong number of  arguments to quote'
def_scheme_string eval_error_args_list, 'syntax error in argument list'
def_scheme_string eval_many_args_str, 'cannot eval many args'
def_scheme_string lambda_no_args_str, 'syntax error in lambda arguments list'
def_scheme_string lambda_body_syntax_error_str, 'syntax error in lambda body'
def_scheme_string macro_no_args_str, 'syntax error in macro arguments list'
def_scheme_string macro_body_syntax_error_str, 'syntax error in macro body'
def_scheme_string do_apply_unknown_type_str, 'cannot apply operator ', \
    'of unknown type'
def_scheme_string lambda_args_count_str, 'wrong number of arguments to lambda'
def_scheme_string lambda_body_syntax_str, 'lambda body must be proper list'
def_scheme_string macro_args_count_str, 'wrong number of arguments to macro'
def_scheme_string macro_body_syntax_str, 'macro body must be proper list'
def_scheme_string define_args_count, 'wrong number of arguments to define'
def_scheme_string define_bad_syntax, 'bad syntax of define'
def_scheme_string set_args_count, 'wrong number of arguments to set'
def_scheme_string set_bad_syntax, 'bad syntax of set'
def_scheme_string if_args_str, '3 arguments expected for "if"'
def_scheme_string define_symbol_required, \
        'first argument to define must be a symbol'
def_scheme_string environment_error, 'environment structure corrupted'
def_scheme_string set_not_bound, 'set! error: symbol not bound'
def_scheme_string operator_many_args, 'cannot apply multiple values ', \
    'as operator'
def_scheme_string include_string_required, 'include requires string argument'
