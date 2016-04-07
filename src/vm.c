/**
 * Copyright (c) 2016 Peter Cannici
 * Licensed under the MIT (X11) license. See LICENSE.
 */

#include "vm.h"
#include "math_ops.h"

YU_CONST
u8 vm_op_argcount(vm_opcode op) {
#define RETURN_ARG_COUNT(op, ...) case op: return sizeof((int[]){__VA_ARGS__})/sizeof(int)-1;
  switch (op) {
  LIST_OPCODES(RETURN_ARG_COUNT)
  }
  assert(!"Invalid opcode");
  return 0;
#undef RETURN_ARG_COUNT
}

YU_CONST
u8 vm_op_bitwidth(vm_opcode op, u8 n) {
#define RETURN_ARG_AT_IDX(op, ...) case op: return ((u8[]){__VA_ARGS__})[n];
  switch (op) {
  LIST_OPCODES(RETURN_ARG_AT_IDX)
  }
  assert(!"Invalid opcode");
  return 0;
#undef RETURN_ARG_AT_IDX
}

// NOTE this function is totally broken on big-endian architectures
void vm_instr_decode(const vm_instruction * restrict inst, ...) {
  va_list args;
  va_start(args, inst);
  u8 argc = vm_op_argcount(inst->op);
  u8 *val8;
  u16 *val16;
  u32 *val32;
  u64 *val64;
  const u8 *rest = inst->rest;
  switch (argc) {
  case 0: break;
  case 1: arg1: switch (vm_op_bitwidth(inst->op, 0)) {
    case  8:  val8 = va_arg(args,  u8 *);  memcpy(val8, rest,  sizeof(u8)); break;
    case 16: val16 = va_arg(args, u16 *); memcpy(val16, rest, sizeof(u16)); break;
    case 32: val32 = va_arg(args, u32 *); memcpy(val32, rest, sizeof(u32)); break;
    case 64: val64 = va_arg(args, u64 *); memcpy(val64, rest, sizeof(u64)); break;
    default: assert(!"Unexpected opcode argument length");
    }
    break;
  case 2: arg2: switch (vm_op_bitwidth(inst->op, 1)) {
    case  8:  val8 = va_arg(args,  u8 *);  memcpy(val8, rest,  sizeof(u8)); rest +=  sizeof(u8); break;
    case 16: val16 = va_arg(args, u16 *); memcpy(val16, rest, sizeof(u16)); rest += sizeof(u16); break;
    case 32: val32 = va_arg(args, u32 *); memcpy(val32, rest, sizeof(u32)); rest += sizeof(u32); break;
    case 64: val64 = va_arg(args, u64 *); memcpy(val64, rest, sizeof(u64)); rest += sizeof(u64); break;
    default: assert(!"Unexpected opcode argument length");
    }
    goto arg1;
  case 3: switch (vm_op_bitwidth(inst->op, 2)) {
    case  8:  val8 = va_arg(args,  u8 *);  memcpy(val8, rest,  sizeof(u8)); rest +=  sizeof(u8); break;
    case 16: val16 = va_arg(args, u16 *); memcpy(val16, rest, sizeof(u16)); rest += sizeof(u16); break;
    case 32: val32 = va_arg(args, u32 *); memcpy(val32, rest, sizeof(u32)); rest += sizeof(u32); break;
    case 64: val64 = va_arg(args, u64 *); memcpy(val64, rest, sizeof(u64)); rest += sizeof(u64); break;
    default: assert(!"Unexpected opcode argument length");
    }
    goto arg2;
  }

  va_end(args);
}

YU_ERR_RET vm_init(struct vm *vm, yu_allocator *mctx, const vm_instruction * restrict prog, size_t prog_sz) {
  YU_ERR_DEFVAR

  memset(vm->r, 0, sizeof vm->r);
  memset(vm->r_state, 0, sizeof vm->r_state);
  memset(vm->ces, 0, sizeof vm->ces);
  vm->mem_ctx = mctx;
  vm->pc = 0;
  YU_CHECK(yu_alloc(mctx, (void **)&vm->rsp, VM_MAX_CALL_DEPTH, sizeof *vm->rsp, VM_MAX_CALL_DEPTH));
  YU_CHECK(yu_alloc(mctx, (void **)&vm->prog, 1, prog_sz, 0));
  memcpy(vm->prog, prog, prog_sz);
  vm->prog_sz = prog_sz;
  // TODO verify that the instruction sequence is valid!

  YU_CHECK(gc_init(&vm->gc, mctx));

  YU_ERR_DEFAULT_HANDLER(yu_local_err)
}

void vm_destroy(struct vm *vm) {
  yu_free(vm->mem_ctx, vm->rsp);
  yu_free(vm->mem_ctx, vm->prog);
}

#if VM_USE_THREADED_DISPATCH
#define ABSJUMP(addr) goto *dispatch[(ip = (vm_instruction *)(addr))->op]
#define I(name) I_VM_OP_ ## name
#else
#define ABSJUMP(addr) (ip = (vm_instruction *)(addr)); continue
#define I(name) case VM_OP_ ## name
#endif

#define JUMP(addr) ABSJUMP(vm->prog + (size_t)(addr))
#define NEXT ABSJUMP((char *)ip + VM_OP_SIZES[ip->op])

// Be sure to save the current instruction offset in bytes in the program
// counter so that the VM can be resumed with a subsequent vm_exec() call.
#define EXIT(err) do{vm->pc = (uintptr_t)ip - (uintptr_t)vm->prog; return (err);} while(0)

static YU_INLINE
void vm_set(struct vm *vm, u16 r, value_t x) {
  vm->r[r] = x;
  assert(!(vm->r_state[r/elemcount(vm->r_state)] & (UINT64_C(1) << r%elemcount(vm->r_state))));
  vm->r_state[r/elemcount(vm->r_state)] |= (UINT64_C(1) << r%elemcount(vm->r_state));
}

static YU_INLINE
void vm_unset(struct vm *vm, u16 r) {
  vm->r_state[r/elemcount(vm->r_state)] &= ~(UINT64_C(1) << r%elemcount(vm->r_state));
}

static YU_INLINE
value_t vm_get(struct vm *vm, u16 r) {
  return vm->r[r];
}

static YU_INLINE
void vm_ces_push(struct vm *vm, bool which) {
  u32 ci = vm->cesidx/elemcount(vm->ces);
  u64 mask = UINT16_C(1) << vm->cesidx%elemcount(vm->ces);
  vm->ces[ci] = (vm->ces[ci] & ~mask) | (-which & mask);
  ++vm->cesidx;
  // TODO throw an error condition? Unclear what to do here—this is a VM
  // failure, not an error with the calling code per se.
  assert(vm->cesidx < VM_MAX_NESTED_BRANCH_DEPTH);
}

static YU_INLINE
bool vm_ces_pop(struct vm *vm) {
  bool out = vm->ces[vm->cesidx/elemcount(vm->ces)] & (UINT64_C(1) << vm->cesidx%elemcount(vm->ces));
  --vm->cesidx;
  return out;
}

YU_ERR_RET vm_exec(struct vm *vm) {
#if VM_USE_THREADED_DISPATCH
#define DEF_DISPATCH_TABLE(op, ...) [op] = &&I_##op,
  static void *dispatch[] = {
    LIST_OPCODES(DEF_DISPATCH_TABLE)
  };
#undef DEF_DISPATCH_TABLE
#endif

  // Program counter is in bytes
  vm_instruction *ip = (vm_instruction *)((char *)vm->prog + vm->pc);


  // Typically the types are used as follows:
  // -  8: Currently unused
  // - 16: Frequently a register number
  // - 32: An index into the instruction array
  // - 64: A type-punned value_t—careful with this

  u8 imm1_8, imm2_8, imm3_8;
  u16 imm1_16, imm2_16, imm3_16;
  u32 imm1_32, imm2_32, imm3_32;
  u64 imm1_64, imm2_64, imm3_64;

#if VM_USE_THREADED_DISPATCH
  goto *dispatch[ip->op];
#else
  while (true) switch (ip->op) {
#endif

 I(NOP):
  NEXT;

 I(HALT):
  EXIT(YU_OK);

 I(PHI):
  vm_instr_decode(ip, &imm3_16, &imm2_16, &imm1_16);
  if (vm_ces_pop(vm)) {
    vm_set(vm, imm1_16,  vm_get(vm, imm3_16));
    vm_unset(vm, imm3_16);
  }
  else {
    vm_set(vm, imm1_16, vm_get(vm, imm2_16));
    vm_unset(vm, imm2_16);
  }
  NEXT;

 I(CALL):
  NEXT;

 I(RET):
  NEXT;

 I(MOV):
  vm_instr_decode(ip, &imm2_16, &imm1_16);
  vm_set(vm, imm1_16, vm_get(vm, imm2_16));
  vm_unset(vm, imm2_16);
  NEXT;

 I(CMP):
  NEXT;

 I(JMPI):
  vm_instr_decode(ip, &imm1_32);
  JUMP(imm1_32);

 I(TESTI):
  vm_instr_decode(ip, &imm3_32, &imm2_32, &imm1_16);
  if (value_is_truthy(vm_get(vm, imm1_16))) {
    vm_ces_push(vm, false);
    JUMP(imm2_32);
  }
  else {
    vm_ces_push(vm, true);
    JUMP(imm3_32);
  }

 I(LOADK): {
  vm_instr_decode(ip, &imm2_64, &imm1_16);
  // TREAD CAREFULLY
  // Type-pun the second immediate value exploiting the fact that values are
  // represented as nanboxed 64-bit doubles.
  value_t k;
  memcpy(&k, &imm2_64, sizeof imm2_64);
  vm_set(vm, imm1_16, k);
  NEXT;
 }

 I(ADD):
  vm_instr_decode(ip, &imm3_16, &imm2_16, &imm1_16);
  vm_set(vm, imm1_16, value_add(&vm->gc, vm_get(vm, imm2_16), vm_get(vm, imm3_16)));
  NEXT;

#if !VM_USE_THREADED_DISPATCH
  }
#endif
}
