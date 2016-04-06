/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#pragma once

#include <stdarg.h>
#include "yu_common.h"
#include "value.h"
#include "gc.h"

#ifndef VM_REGISTER_COUNT
#define VM_REGISTER_COUNT 1024
#endif

#ifndef VM_MAX_NESTED_BRANCH_DEPTH
#define VM_MAX_NESTED_BRANCH_DEPTH 512
#endif

#ifndef VM_MAX_CALL_DEPTH
#define VM_MAX_CALL_DEPTH 8092
#endif

// VA_ARGS are a 0-terminated list of bit widths of the opcode's expected
// arguments. Remember to modify yu_instr_decode if you add any opcodes with
// more than 3 arguments.
#define LIST_OPCODES(X)                         \
  X(VM_OP_NOP, 0)                               \
  X(VM_OP_HALT, 0)                              \
  X(VM_OP_PHI, 16,16,16,0)                      \
  X(VM_OP_CALL, 16,32,0)                        \
  X(VM_OP_RET, 0)                               \
  X(VM_OP_MOV, 16,16,0)                         \
  X(VM_OP_CMP, 16,16,16,0)                      \
  X(VM_OP_JZI, 16,32,32,0)                      \
  X(VM_OP_LOADK, 16,64,0)

DEF_PACKED_ENUM(vm_opcode, LIST_OPCODES)

static const u8 VM_OP_SIZES[] = {
  [VM_OP_NOP] = 1, [VM_OP_HALT] = 1, [VM_OP_PHI] = 7,
  [VM_OP_CALL] = 7, [VM_OP_RET] = 1, [VM_OP_MOV] = 5,
  [VM_OP_CMP] = 7, [VM_OP_JZI] = 11, [VM_OP_LOADK] = 11,
};

typedef struct {
  vm_opcode op;
  // Note that arguments are packed in argN->arg1 order in `rest`
  u8 rest[];
} vm_instruction;

YU_CONST
u8 vm_op_argcount(vm_opcode op);
YU_CONST
u8 vm_op_bitwidth(vm_opcode op, u8 n);

void vm_instr_decode(const vm_instruction * restrict inst, ...);

struct vm {
  value_t r[VM_REGISTER_COUNT];
  // Bitmap of register assignment status
  u64 r_state[VM_REGISTER_COUNT/64];

  // CFG edge stack for phi resolution.
  // LSB is 0 if the next phi instruction should use its first operand, 1 for
  // its second.
  u64 ces[VM_MAX_NESTED_BRANCH_DEPTH/64];
  u32 cesidx;

  vm_instruction * restrict prog;
  // Size in bytes; instructions are variable length
  size_t prog_sz;

  // Return stack pointer; aligned to VM_MAX_CALL_DEPTH so finding the bottom of
  // the stack is just a modulo operation (or a mask if the max call depth is a
  // power of 2, which it is by default).
  u32 *rsp;

  yu_allocator *mem_ctx;
  struct gc_info gc;

  size_t pc; // Program counter; in bytes since instructions are variable length
};

// prog_sz should be the size in bytes of prog, since the size of individual
// instructions may vary. This function makes a copy of prog.
YU_ERR_RET vm_init(struct vm *vm, yu_allocator *mctx, const vm_instruction * restrict prog, size_t prog_sz);
void vm_destroy(struct vm *vm);

// If this function returns an error, the VM is still in a valid state and may
// be resumed if the error is non-fatal.
YU_ERR_RET vm_exec(struct vm *vm);
