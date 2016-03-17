Implemented
-----------

(This stuff may still be a work-in-progress, but significant parts are done)

- Nanboxed value representation
- Incremental, quad-color copying generational garbage collector
- Arena-based bump allocator for heap-allocated values
- Interned, immutable byte buffers
- Unicode-correct String implementation
- Miscellaneous type-safe efficient data structures
- Lexer

In-progress
-----------
- Parser (there's very little syntax, but there are a few things such as stack
  effect and word definitions).

Unimplemented
-------------

- Codegen
- Numeric tower
- VMgen-based stack interpreter
- Stack checker
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
