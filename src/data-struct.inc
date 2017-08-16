; Interpreter data structures
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

object.tag = 0x00
object.gc_copy = 0x04
object.tag_bits = 4
object.tag_ceiling = 1 shl object.tag_bits
object.tag.type_mask = (1 shl object.tag_bits) - 1
object.tag.empty_list = 0x01
object.tag.cons_cell = 0x02
object.tag.symbol = 0x03
object.tag.string = 0x04
object.tag.number = 0x05
object.tag.bytevector = 0x06
object.tag.environment = 0x07
; Continuation, has 4 additional fields
object.tag.continuation = 0x08
object.tag.native_procedure = 0x09
object.tag.native_special = 0x0a
object.tag.lambda = 0x0b
object.tag.macro = 0x0c

object.tag.fixed = 0x100
object.tag.gc_copied = 0x200

object_size.empty_list = 0x04
object_size.cons_cell = 0x0c
object_size.symbol = 0x0c
object_size.string = 0x08
object_size.number = 0x08
object_size.bytevector = 0x04
object_size.environment = 0x0c
object_size.continuation = 0x18
object_size.native_procedure = 0x08
object_size.native_special = 0x08
object_size.lambda = 0x10
object_size.macro = 0x10

cons_cell.car = 0x04
cons_cell.cdr = 0x08

; We need symbol.next to not be overwritten by object.gc_copy
symbol.name_length = 0x04
symbol.next = 0x08
symbol.name_data = 0x0c

string.length = 0x04
string.data = 0x08

number.value = 0x04

bytevector.length = 0x04
bytevector.data = 0x08

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

macro.args = 0x04
macro.body = 0x08
macro.environment = 0x0c

; Memory is allocated in blocks of size memory_block_size
; Block structure:
; 0x00 Block size
; 0x04 Next block address
; 0x08 Address of free memory inside block
; 0x0c Start of data
memory_block_size = 1024*1024
max_object_size = 4*1024
block.size = 0x00
block.next = 0x04
block.free = 0x08
block.start = 0x0c

; Structure for scheme virtual machine state
; Continuation to execute
vm.current_continuation = 0x00
; First argument value passed to current continuation
vm.argument = 0x04
; Rest arguments values passed to current continuation
; First argument goes into vm.argument and rest into vm.args.
vm.args = 0x08
; Evaluation environment. global_env is the global environment.
vm.environment = 0x0c
; Here we maintain singly-linked list of all active VMs
; Address of next vm state.
vm.next = 0x10

vm_size = 0x14
