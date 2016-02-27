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

# TODO perhaps don't assume $FMT accepts a -i in-place flag.
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


######################################################################
# This section is for debugging the giant macro ‘templates’ used for #
# common data structures.                                            #
# Hopefully, you shouldn't have to fiddle with this.                 #
######################################################################

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


######################################################################
# Expand the various m4 templates in m4/ to generate boilerplate.    #
# This is something of a Makefile anti-pattern, so don't try this at #
# home. It works by removing the ‘new’ part of MAKECMDGOALS and      #
# using the other specified targets as arguments to m4. `%:` is a    #
# catch-all rule so that `make new test:file` doesn't try to execute #
# a (non-existent) `test:file` target.                               #
#                                                                    #
# See http://stackoverflow.com/a/6273809 if you want to try to       #
# decipher this rule.                                                #
#                                                                    #
# Valid subcommands:                                                 #
#   new test:<name> — expands m4/test.c.m4 into a test suite <name>  #
#   new src:<name> — expands m4/src.{c,h}.m4 src/<name>.{c,h}        #
#   new common:<name> — expands m4/common.h.m4 into src/yu_<name>.h  #
######################################################################

# TODO does Linux alias m4 to gm4? (or vice-versa)
M4 ?= gm4

new:
	# Turn a target like `new subcommand:name` into a few variables.
	# Prefix them with $@_ (new_) just to avoid any potential namespace
	# collisions.
	#
	# General description of what's going on for a command such as
	# `make new test:splaytree`:
	#   1. MAKECMDGOALS is a magic variable that make sets to "new test:splaytree"
	#   2. $@_FILE will be set to "test splaytree" by removing "new" and replacing
	#      colons with spaces (so that we can use the built-in word function).
	#      Note that "make new test splaytree" would try to invoke the ‘test’ target
	#      above, which is why this command specifies subcommand with a colon.
	#   3. $@_TYPE and $@_NAME get set to the subcommand and file name
	#   4. Bash gets called with a big if statement that invokes the correct m4
	#      command for each type of template file.
	#      Use bash explicitly in case the user runs make from a non-POSIX shell
	#      such as fish.
	$(eval $@_FILE := $(subst :, ,$(filter-out $@,$(MAKECMDGOALS))))
	$(eval $@_TYPE := $(word 1,$($@_FILE)))
	$(eval $@_NAME := $(word 2,$($@_FILE)))
	@echo 'if [[ $($@_TYPE) == t* ]]; \
	then \
	    $(M4) $(M4FLAGS) -Dtest_name=$($@_NAME) m4/test.c.m4 > test/test_$($@_NAME).c && \
	    echo "✓ Created test/test_$($@_NAME).c"; \
	elif [[ $($@_TYPE) == s* ]]; \
	then \
	    $(M4) $(M4FLAGS) -Dsrc_name=$($@_NAME) m4/src.c.m4 > src/$($@_NAME).c && \
	    echo "✓ Created src/$($@_NAME).c"; \
	    $(M4) $(M4FLAGS) -Dsrc_name=$($@_NAME) m4/src.h.m4 > src/$($@_NAME).h && \
	    echo "✓ Created src/$($@_NAME).h"; \
	elif [[ $($@_TYPE) == c* ]]; \
	then \
	    $(M4) $(M4FLAGS) -Dsrc_name=$($@_NAME) m4/common.h.m4 > src/yu_$($@_NAME).h && \
	    echo "✓ Created src/yu_$($@_NAME).h" && \
	    echo "Don'\''t forget to add it to yu_common.h"; \
	else \
	    echo "Don'\''t know how to generate ‘$($@_TYPE)’"; \
	    echo "Valid templates:"; \
	    echo "  • new test:$($@_NAME)	Create a test suite" | expand -t 30; \
	    echo "  • new src:$($@_NAME)	Create a new source/header pair in the src/ directory" | expand -t 30; \
	    echo "  • new common:$($@_NAME)	Create a new Yu internal header file" | expand -t 30; \
	fi' | bash

%:         # Catch all.
	@: # Somewhat obscure sytax that means ‘silently do nothing’.
