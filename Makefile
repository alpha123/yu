DEBUG ?= yes

INCLUDE_DIRS := -I/usr/local/include -I/usr/local/include/blas -Itest -Isrc -I.
LIB_DIRS := -L/usr/local/lib -LSFMT -Lutf8proc
LIBS := -lmpfr -lgmp -lm -l:libsfmt.a -l:libutf8proc.a
ASAN_FLAGS := -fsanitize=address -O1 -fno-omit-frame-pointer

CFLAGS ?= -std=c99

SFMT_MEXP ?= 19937
CFLAGS += -DSFMT_MEXP=$(SFMT_MEXP)

# cgdb <http://cgdb.github.io/> is a more convenient ncurses frontend
# for gdb. Unlike gdb -tui it does syntax coloring and stuff.
ifneq ($(shell which cgdb),)
	DB ?= cgdb
else
	DB ?= gdb
endif

# TODO does Linux alias m4 to gm4? (or vice-versa)
# On *BSD we always want to use GNU M4.
M4 ?= gm4

# TODO perhaps don't assume $FMT accepts a -i in-place flag.
FMT ?= clang-format

# If exctags does not exist, assume ‘ctags’ to be Exhuberant CTags.
ifneq ($(shell which exctags),)
    CTAGS ?= exctags
else
    CTAGS ?= ctags
endif

ifeq ($(DEBUG),yes)
	CFLAGS += -gdwarf-4 -g3 -DDEBUG -Wall -Wextra -pedantic
	# GCC doesn't seem to build with ASAN on my machine
	ifneq ($(findstring clang,$(CC)),)
		CFLAGS += $(ASAN_FLAGS)
	endif
else
	CFLAGS += -DNDEBUG -O3
endif

COMMON_SRCS := $(wildcard src/*.c)
COMMON_OBJS := $(COMMON_SRCS:.c=.o)

TEST_OUT := test/test
TEST_SRCS := $(wildcard test/*.c)
TEST_OBJS := $(TEST_SRCS:.c=.o)

.PHONY: all clean test tags

all:

test/%.o: test/%.c
	$(CC) $(subst std=c99,std=c11,$(CFLAGS)) $(INCLUDE_DIRS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

test_driver: $(TEST_OBJS) $(COMMON_OBJS) deps
	$(CC) $(LDFLAGS) $(LIB_DIRS) -Wl,-Ttest/test.ld $(TEST_OBJS) $(COMMON_OBJS) -o $(TEST_OUT) $(LIBS)

test: test_driver  ## Build and run the test suite
	./$(TEST_OUT)

tags:  ## Create a ctags file for the source tree
	$(CTAGS) -R src

clean:  ## Remove all build output — run as \\033[37m$MAKECMD clean --keep\\033[0m to keep dependencies
# This is kind of cute, but something of a hack:
# Don't clean bundled dependencies if clean is invoked with --keep.
# GNU Make treats this as --keep-going (which is harmless), and puts
# "k" in $MAKEFLAGS.
# Use bash instead of sh for the [[ ]] syntax for testing $MAKEFLAGS.
	rm -f tags src/*.o test/*.o $(TEST_OUT) src/preprocessed/test
	@echo 'if [[ "$(MAKEFLAGS)" != *k* ]]; \
	then \
	  echo 'rm -f SFMT/*.o SFMT/libsfmt.a utf8proc/*.o utf8proc/libutf8proc.a'; \
	  rm -f SFMT/*.o SFMT/libsfmt.a utf8proc/*.o utf8proc/libutf8proc.a; \
	fi' | bash


######################################################################
# Build bundled dependencies. These get statically linked with       #
# explicit -l:libXYZ.a flags above. See $LIBS.                       #
######################################################################

libsfmt:
	$(CC) $(CFLAGS) -c SFMT/SFMT.c -o SFMT/SFMT.o
	$(AR) rcs SFMT/$@.a SFMT/SFMT.o

libutf8proc:
	$(CC) $(CFLAGS) -include utf8proc/utf8proc.h -c utf8proc/utf8proc.c -o utf8proc/utf8proc.o
	$(CC) $(CFLAGS) -include utf8proc/utf8proc.h -c utf8proc/utf8proc_data.c -o utf8proc/utf8proc_data.o
	$(AR) rcs utf8proc/$@.a utf8proc/utf8proc.o utf8proc/utf8proc_data.o

deps: libsfmt libutf8proc  ## Build static libraries of bundled dependencies



######################################################################
# This section is for debugging the giant macro ‘templates’ used for #
# common data structures.                                            #
# Hopefully, you shouldn't have to fiddle with this.                 #
######################################################################

.PHONY: copy_templates build_debug_test debug_templates

# Assign with = instead of := because there won't be any files in src/preprocessed/
# until copy_templates runs.
TMPL_SRCS = $(wildcard src/preprocessed/*.c)
TMPL_OBJS = $(TMPL_SRCS:.c=.o)

copy_templates:
	cp $(filter-out test/ptest.c,$(TEST_SRCS)) src/preprocessed
	$(M4) $(M4FLAGS) include/build_debug_templates.h.m4 > include/build_debug_templates.h

src/preprocessed/%.i: src/preprocessed/%.c
	$(CC) $(CFLAGS) -nostdinc -Iinclude/dummy $(INCLUDE_DIRS) -E -P -c $< -o $@
	$(FMT) $(FMTFLAGS) -i $@

src/preprocessed/%.o: src/preprocessed/%.i
	$(CC) $(subst std=c99,std=c11,$(CFLAGS)) $(INCLUDE_DIRS) \
	    -include include/build_debug_templates.h \
            -x c -c $< -o $@

$(TMPL_OBJS): $(TMPL_SRCS:.c=.i)

build_debug_test: copy_templates deps $(TMPL_OBJS) test/ptest.o
	$(CC) $(LDFLAGS) $(LIB_DIRS) -Wl,-Ttest/test.ld test/ptest.o $(TMPL_OBJS) -o src/preprocessed/test $(LIBS)

debug_templates: build_debug_test  ## Build a special preprocessed test executable for debugging large macro expansions
	$(DB) $(DBFLAGS) src/preprocessed/test


######################################################################
# I wish more Makefiles provided a `make targets` command, so let's  #
# do that here. See:                                                 #
#  http://marmelab.com/blog/2016/02/29/auto-documented-makefile.html #
#  https://gist.github.com/prwhite/8168133                           #
#  https://gist.github.com/depressiveRobot/46002002beabe2e7b4fd      #
######################################################################

.PHONY: help targets

targets:  ## Print a list of available targets with short descriptions.
# Only include alphanumeric targets, i.e. exclude stuff like %.o
# Double xargs sh -c: the first one handles escape characters (e.g. colors).
# The second is an extra level of indirection to handle setting variables
# that target descriptions can use, as well as formatting columns.
# Yes, we actually do need that many backslashes.
	@grep -E '^[a-zA-Z_\-]+:.*?##.+' $(MAKEFILE_LIST) | sort | \
	    awk 'BEGIN{FS=":.*?##\s*"};{printf "\033[36m%s\033[0m\\\\\\\\\\\\\\\\\\\\\\\t%s\n",$$1,$$2}' | \
	    xargs -I'{}' sh -c "echo -e '{}'" | \
	    xargs -I'{}' sh -c "MAKECMD=$(MAKE) sh -c 'echo {}' | expand -t 30"

help:  ## Print a synopsis of useful Makefile targets
	@sh -c "echo -e '\033[37m$(MAKE)\033[0m — Build the Yu REPL'"
	@sh -c "echo -e '\033[37m$(MAKE) targets\033[0m — List available targets'"


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

new:  ## Create a new skeleton source file — see \\033[37m$MAKECMD new\\033[0m for details
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
#      Use bash explicitly because we can't be sure /bin/sh is bash and the if
#      statement uses bash syntax.
	$(eval $@_FILE := $(subst :, ,$(filter-out $@,$(MAKECMDGOALS))))
	$(eval $@_TYPE := $(word 1,$($@_FILE)))
	$(eval $@_NAME := $(word 2,$($@_FILE)))
	@echo 'if [[ "$($@_TYPE)" == t* ]]; \
	then \
	    $(M4) $(M4FLAGS) -Dtest_name=$($@_NAME) m4/test.c.m4 > test/test_$($@_NAME).c && \
	    echo "✓ Created test/test_$($@_NAME).c"; \
	elif [[ "$($@_TYPE)" == s* ]]; \
	then \
	    $(M4) $(M4FLAGS) -Dsrc_name=$($@_NAME) m4/src.c.m4 > src/$($@_NAME).c && \
	    echo "✓ Created src/$($@_NAME).c"; \
	    $(M4) $(M4FLAGS) -Dsrc_name=$($@_NAME) m4/src.h.m4 > src/$($@_NAME).h && \
	    echo "✓ Created src/$($@_NAME).h"; \
	elif [[ "$($@_TYPE)" == c* ]]; \
	then \
	    $(M4) $(M4FLAGS) -Dsrc_name=$($@_NAME) m4/common.h.m4 > src/yu_$($@_NAME).h && \
	    echo "✓ Created src/yu_$($@_NAME).h" && \
	    echo "Don'\''t forget to add it to yu_common.h"; \
	elif [[ "$($@_TYPE)" ]]; \
	then \
	    echo "Don'\''t know how to generate ‘$($@_TYPE)’"; \
	    echo "Valid templates:"; \
	    echo -e "  • \033[37mnew test:$($@_NAME)\033[0m	Create a test suite" | expand -t 30; \
	    echo -e "  • \033[37mnew src:$($@_NAME)\033[0m	Create a new source/header pair in the src/ directory" | expand -t 30; \
	    echo -e "  • \033[37mnew common:$($@_NAME)\033[0m	Create a new Yu internal header file" | expand -t 30; \
	else \
	    echo "Valid templates:"; \
	    echo -e "  • \033[37mnew test:<name>\033[0m	Create a test suite" | expand -t 30; \
	    echo -e "  • \033[37mnew src:<name>\033[0m	Create a new source/header pair in the src/ directory" | expand -t 30; \
	    echo -e "  • \033[37mnew common:<name>\033[0m	Create a new Yu internal header file" | expand -t 30; \
	fi' | bash

%:         # Catch all.
# Somewhat obscure sytax that means ‘silently do nothing’.
	@:
