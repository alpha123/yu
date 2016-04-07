/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "test.h"
#include "vm.h"

#define INIT_VM(vm_var, prog, prog_sz)                                  \
  struct vm vm;                                                         \
  yu_err _vminiterr = vm_init(&vm, (yu_allocator *)&mctx, prog, prog_sz); \
  assert(_vminiterr == YU_OK);

struct instr {
  u64 op, a, b, c;
};

size_t assemble(vm_instruction *prog, struct instr *bytecode) {
  size_t total_sz = 0;
  struct instr *op = bytecode;
  u8 *out = (u8 *)prog;
  while (op->op != (u64)-1) {
    total_sz += VM_OP_SIZES[op->op];
    ++op;
  }
  if (out == NULL)
    return total_sz;

  op = bytecode;
  size_t i = 0;
  while (op->op != (u64)-1) {
    out[i++] = (u8)op->op;
    for (u8 j = vm_op_argcount(op->op); j > 0; j--) {
      switch (j-1) {
      case 0:
        memcpy(out + i, &op->a, vm_op_bitwidth(op->op, j-1) / 8);
        break;
      case 1:
        memcpy(out + i, &op->b, vm_op_bitwidth(op->op, j-1) / 8);
        break;
      case 2:
        memcpy(out + i, &op->c, vm_op_bitwidth(op->op, j-1) / 8);
        break;
      }
      i += vm_op_bitwidth(op->op, j-1) / 8;
    }
    ++op;
  }
  return total_sz;
}

#define ASM(var, arr)                           \
  vm_instruction *var;                          \
  size_t var##_sz;                              \
  do{                                           \
    var##_sz = assemble(NULL, (arr));           \
    var = alloca(var##_sz);                     \
    assemble(var, (arr));                       \
  }while(0)

#define DONE {(u64)-1,0,0,0}
#define HALT {VM_OP_HALT,0,0,0},DONE
#define SLOT 0

#define SETUP \
  TEST_GET_ALLOCATOR(mctx);

#define TEARDOWN \
  yu_alloc_ctx_free(&mctx);

#define LIST_VM_TESTS(X) \
  X(opcode_argcount, "Should return the expected number of arguments of an opcode") \
  X(opcode_bitwidth, "Should return the expected size of an opcode's argument at an index") \
  X(instr_decode, "Should decode instructions according to that opcode's format") \
  X(instr_dispatch, "VM should execute instructions in the correct order")

TEST(opcode_argcount)
  PT_ASSERT_EQ(vm_op_argcount(VM_OP_RET), 0);
  PT_ASSERT_EQ(vm_op_argcount(VM_OP_LOADK), 2);
  PT_ASSERT_EQ(vm_op_argcount(VM_OP_PHI), 3);
END(opcode_argcount)

TEST(opcode_bitwidth)
  PT_ASSERT_EQ(vm_op_bitwidth(VM_OP_MOV, 0), 16);
  PT_ASSERT_EQ(vm_op_bitwidth(VM_OP_TESTI, 2), 32);
  PT_ASSERT_EQ(vm_op_bitwidth(VM_OP_LOADK, 1), 64);
END(opcode_bitwidth)

TEST(instr_decode)
  struct instr mov_a[] = {{VM_OP_MOV, 50, 40, 0}, DONE};
  ASM(mov, mov_a);

  u16 ra, rb;
  vm_instr_decode(mov, &rb, &ra);
  PT_ASSERT_EQ(ra, 50);
  PT_ASSERT_EQ(rb, 40);


  value_t x = value_from_double(3.141592654);
  u64 xx;
  memcpy(&xx, &x, sizeof x);
  struct instr loadk_a[] = {{VM_OP_LOADK, 65, xx, 0}, DONE};
  ASM(loadk, loadk_a);

  u64 im;
  vm_instr_decode(loadk, &im, &ra);
  PT_ASSERT_EQ(ra, 65);
  PT_ASSERT_EQ(im, xx);


  struct instr phi_a[] = {{VM_OP_PHI, 72, 70, 71}, DONE};
  ASM(phi, phi_a);

  u16 rc;
  vm_instr_decode(phi, &rc, &rb, &ra);
  PT_ASSERT_EQ(ra, 72);
  PT_ASSERT_EQ(rb, 70);
  PT_ASSERT_EQ(rc, 71);
END(instr_decode)

TEST(instr_dispatch)
  struct instr prog_a[] = {{VM_OP_LOADK, 64, SLOT, 0}, {VM_OP_LOADK, 65, SLOT, 0},
                          {VM_OP_ADD, 66, 64, 65}, HALT};
  value_t a = value_from_int(42), b = value_from_int(5);
  memcpy(&prog_a[0].b, &a, sizeof a);
  memcpy(&prog_a[1].b, &b, sizeof b);
  ASM(prog, prog_a);
  INIT_VM(vm, prog, prog_sz)
  yu_err ok = vm_exec(&vm);
  assert(ok == YU_OK);
  PT_ASSERT(value_is_int(vm.r[66]));
  PT_ASSERT_EQ(value_to_int(vm.r[66]), 47);
  vm_destroy(&vm);
END(instr_dispatch)


SUITE(vm, LIST_VM_TESTS)
