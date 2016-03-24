Implemented
-----------

(This stuff may still be a work-in-progress, but significant parts are done)

- Nanboxed value representation
- Copying, quad-color incremental, generational garbage collector
- Arena-based bump allocator for heap-allocated values
- Interned, immutable byte buffers
- Unicode-correct String implementation
- Miscellaneous type-safe efficient data structures
- Lexer
- Advanced 64-bit virtual-memory aware allocator API

In-progress
-----------

- Unboxed packed string representation for short ASCII strings

Unimplemented
-------------

- Parser (there's very little syntax, but there are a few things such as stack
  effect and word definitions).
- Numeric tower
- Stack checker
- Codegen
- VMgen-based stack interpreter
- libffi-based cffi
- Standard library

Future
------

- Optimized assembly interpreter a la LuaJIT and JavaScriptCore
- Tracing JIT compiler
- Concurrent garbage collection
- Investigate the potential of an Interprocedural SSA interpreter
  + Generate ISSA bytecode and interpret it directly while also using it to JIT
  + Could also do constant-prop, CSE, etc online
  + Top few stack items would be the "variables" for purposes of SSA; certain
    functions like `dip` would be assignments to non-top slots.
    - Makes register allocation for stack caching easier.
