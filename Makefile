DEBUG ?= yes

INCLUDE_DIRS := -I/usr/local/include -I/usr/local/include/blas -Itest -Isrc -I.
LIB_DIRS := -L/usr/local/lib -Lutf8proc 
LIBS := -lmpfr -lgmp -lm -l:libutf8proc.a
ASAN_FLAGS := -fsanitize=address -O1 -fno-omit-frame-pointer

SFMT_MEXP ?= 19937
CFLAGS ?= -std=c99 $(INCLUDE_DIRS) -DSFMT_MEXP=$(SFMT_MEXP)

# cgdb <http://cgdb.github.io/> is a more convenient ncurses frontend
# for gdb. Unlike gdb -tui it does syntax coloring and stuff.
ifneq ($(shell which cgdb),)
	DB ?= cgdb
else
	DB ?= gdb
endif

FMT ?= clang-format

ifeq ($(DEBUG),yes)
	# -Wparenthesis is annoying
	CFLAGS += -gdwarf-4 -g3 -DDEBUG -Wall -Wextra -pedantic -Wno-parentheses
	# GCC doesn't seem to build with ASAN on my machine
	ifneq ($(findstring "clang",$(CC)),)
		CFLAGS += $(ASAN_FLAGS)
	endif
else
	CFLAGS += -DNDEBUG
endif

COMMON_HEADERS := $(wildcard src/yu_*.h)
COMMON_SRCS := $(wildcard src/yu_*.c)
COMMON_OBJS := $(COMMON_SRCS:.c=.o)

TEST_OUT := test/test
TEST_SRCS := $(wildcard test/*.c) $(COMMON_SRCS)
TEST_OBJS := $(TEST_SRCS:.c=.o)

.PHONY: all clean test

all:

test/%.o: test/%.c
	$(CC) $(subst std=c99,std=c11,$(CFLAGS)) $(INCLUDE_DIRS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

test_driver: $(TEST_OBJS)
	$(CC) $(LDFLAGS) $(LIB_DIRS) -Wl,-Ttest/test.ld $(TEST_OBJS) -o $(TEST_OUT) $(LIBS)

test: test_driver
	./$(TEST_OUT)

clean:
	rm src/*.o test/*.o $(TEST_OUT)

# This section is for debugging the giant macro ‘templates’ used for
# common data structures.
# Hopefully, you shouldn't have to fiddle with this.

.PHONY: copy_templates build_debug_test debug_templates

TMPL_SRCS = $(wildcard src/preprocessed/*.c)
TMPL_OBJS = $(TMPL_SRCS:.c=.o)

copy_templates:
	cp $(filter-out test/ptest.c,$(TEST_SRCS)) src/preprocessed

src/preprocessed/%.i: src/preprocessed/%.c
	$(CC) -nostdinc -Iinclude/dummy $(CFLAGS) -E -P -c $< -o $@
	$(FMT) $(FMTFLAGS) -i $@

# TODO figure out how to get this to work without including every
# file on the command line.
src/preprocessed/%.o: src/preprocessed/%.i
	$(CC) $(subst std=c99,std=c11,$(CFLAGS)) $(INCLUDE_DIRS) \
            -include stdbool.h -include stddef.h -include stdint.h \
            -include stdlib.h -include inttypes.h -include math.h \
            -include utf8proc/utf8proc.h -include SFMT/SFMT.h \
            -x c -c $< -o $@

$(TMPL_OBJS): $(TMPL_SRCS:.c=.i)

build_debug_test: copy_templates $(TMPL_OBJS) test/ptest.o
	$(CC) $(LDFLAGS) $(LIB_DIRS) -Wl,-Ttest/test.ld test/ptest.o $(TMPL_OBJS) -o src/preprocessed/test $(LIBS)

debug_templates: build_debug_test
	$(DB) $(DBFLAGS) src/preprocessed/test
