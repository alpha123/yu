Overview
--------

Yu's VM is a fairly standard register-based variable-instruction-length 
indirect-threaded interpreter. It is heavily inspired by Lua 5.1's 
VM, and takes a bit from other projects (such as the "real machine" RISC-V 
ISA).

The instruction set is designed to be very simple but also targetable by a 
single-pass compiler (Yu's compiler just does a syntax-directed translation 
straight to bytecode). It is not necessarily designed for performance or 
compactness, though those are secondary goals. Yu is relatively fast by virtue 
of not having a large number of highly dynamic features, so the VM needn't try 
to overcompensate.

Nearly all instructions operate exclusively on registers, with no immediate 
values or stack access. Only LOAD* and STORE* instructions access an area other 
than the register store.

Many instructions are of the form OPCODE SRC1 SRC2 DEST.

Registers
---------

Yu's ISA uses 256 registers. R0 is fixed at the value 0, yielding 255 usable 
registers.

The reason for R0 is to reduce the size of the instruction set.  For example, 
`MOV Rn Rm` is simply encoded as `ADD R0 Rn Rm`. Some other instructions are 
made redundant by R0 as well (for example, there are no unconditional jumps, 
just `CMPNZ R0 R0 jump_loc`).

Instruction Format
------------------

Instructions are represented as unsigned varying-bit integers with the format

```
    u8     u8   u8    uX
+--------+----+----+------+
| OPCODE | R1 | R2 | REST |
+--------+----+----+------+
```

Where `REST` is read until the first non-0xFF byte, and the read bytes are 
summed. `REST` is limited to 32 bits, meaning the size of a single instruction 
cannot exceed 56 bits. Padding is inserted to the nearest multiple of 16, so a 
56-bit instruction would take 8 bytes of storage.

Thus, an ADD instruction is usually 32 bits, unless the target register is 
R256. R256 as a 3rd operand requires being encoded as a pair of bytes 0xFF, 
0x00.

Instructions are aligned on 4-byte boundaries, and are read as a 64-bit integer 
which gets split up into one or more instructions.

Upvalues
--------

Upvalues are managed by the VM and accessed with `UPLOAD` and `UPSTORE`.
