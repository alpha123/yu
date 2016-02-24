INCLUDE_DIRS := -I/usr/local/include -I/usr/local/include/blas
LIB_DIRS := -L/usr/local/lib -Lutf8proc 
NUMLIBS := -lmpfr -lgmp -lopenblas -lm
STRLIBS := -lm -l:libutf8proc.a
ASAN_FLAGS := -fsanitize=address -O1 -fno-omit-frame-pointer

CFLAGS := -std=c99 -O0 $(INCLUDE_DIRS) $(LIB_DIRS) -DDEBUG -DSFMT_MEXP=19937

strtest:
	clang37 -g strtest.c yu_common.c yu_buf.c yu_str.c $(CFLAGS) $(STRLIBS) $(ASAN_FLAGS)

strtest_gcc:
	gcc5 -g strtest.c yu_common.c yu_buf.c yu_str.c $(CFLAGS) $(STRLIBS)

numtest:
	clang37 -g numtest.c yu_common.c yu_numeric.c $(CFLAGS) $(NUMLIBS) $(ASAN_FLAGS)

numtest_gcc:
	gcc5 -g numtest.c yu_common.c yu_numeric.c $(CFLAGS) $(NUMLIBS)

hashtest:
	clang37 -g hashtest.c yu_common.c $(CFLAGS) $(ASAN_FLAGS)

hashtest_gcc:
	gcc5 -g hashtest.c yu_common.c $(CFLAGS)

treetest:
	clang37 -g treetest.c yu_common.c $(CFLAGS) $(ASAN_FLAGS)

treetest_gcc:
	gcc5 -g treetest.c yu_common.c $(CFLAGS)

listtest:
	clang37 -g listtest.c SFMT/SFMT.c yu_common.c $(CFLAGS) $(ASAN_FLAGS)

listtest_gcc:
	gcc5 -g listtest.c SFMT/SFMT.c yu_common.c $(CFLAGS)

heaptest:
	clang37 -g heaptest.c SFMT/SFMT.c yu_common.c $(CFLAGS) $(ASAN_FLAGS)

heaptest_gcc:
	gcc5 -g heaptest.c SFMT/SFMT.c yu_common.c $(CFLAGS)

lex.i: lex.l
	flex -d -o $@ lex.l

lextest: lex.i
	clang37 -g linenoise.c lextest.c lexer.c yu_common.c $(CFLAGS) $(STRLIBS) $(ASAN_FLAGS)

lextest_gcc: lex.i
	gcc5 -g linenoise.c lextest.c lexer.c yu_common.c $(CFLAGS) $(STRLIBS)
