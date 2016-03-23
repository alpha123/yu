# Options meant to be set on the command line, see the help rule for
# some descriptions.

CONFIG_FILE := .config

ifeq ($(wildcard $(CONFIG_FILE)),)
    COVERAGE ?= no
    DEBUG ?= yes
    DMALLOC ?= no
    PREFIX ?= /usr/local/bin
    PROFILE ?= no
else
    CONF_VARS := COVERAGE DEBUG DMALLOC PREFIX PROFILE =
    CONFIG := $(filter-out $(CONF_VARS),$(subst =, = ,$(sort $(shell cat $(CONFIG_FILE)))))
    COVERAGE := $(word 1, $(CONFIG))
    DEBUG := $(word 2, $(CONFIG))
    DMALLOC := $(word 3, $(CONFIG))
    PREFIX := $(word 4, $(CONFIG))
    PROFILE := $(word 5, $(CONFIG))
endif

INCLUDE_DIRS := -I/usr/local/include -I/usr/local/include/blas -Itest -Isrc -Idep
LIB_DIRS := -L/usr/local/lib -Ldep
LIBS := -lmpfr -lgmp -lm -l:libdeps.a
ASAN_FLAGS := -fsanitize=address -O1 -fno-optimize-sibling-calls -fno-omit-frame-pointer

SFMT_MEXP ?= 19937

override CFLAGS += -std=c99 -DSFMT_MEXP=$(SFMT_MEXP)

override LINK_FLAGS += $(LIB_DIRS)

$(file  >.config,COVERAGE=$(COVERAGE))
$(file >>.config,DEBUG=$(DEBUG))
$(file >>.config,DMALLOC=$(DMALLOC))
$(file >>.config,PREFIX=$(PREFIX))
$(file >>.config,PROFILE=$(PROFILE))

# cgdb <http://cgdb.github.io/> is a more convenient ncurses frontend
# for gdb. Unlike gdb -tui it does syntax coloring and stuff.
ifneq ($(shell which cgdb),)
    DB ?= cgdb
else
    DB ?= gdb
endif

# Force GNU M4 if available.
# The various m4 files use some GNU-specific features like ranges in
# translit. It would be nice for them to be more portable.
ifneq ($(shell which gm4),)
    M4 ?= gm4
else
    M4 ?= m4
endif

# TODO perhaps don't assume $FMT accepts a -i in-place flag.
FMT ?= clang-format

# If exctags does not exist, assume ‘ctags’ to be Exhuberant CTags.
ifneq ($(shell which exctags),)
    CTAGS ?= exctags
else
    CTAGS ?= ctags
endif

FLEX ?= flex

ifeq ($(COVERAGE),yes)
    override CFLAGS += -fprofile-arcs -ftest-coverage
    override LINK_FLAGS += -fprofile-arcs -ftest-coverage
endif

ifeq ($(DEBUG),yes)
    override CFLAGS += -gdwarf-4 -g3 -DDEBUG -Wall -Wextra -pedantic
# GCC doesn't seem to build with ASAN on my machine
ifneq ($(findstring clang,$(CC)),)
    CFLAGS += $(ASAN_FLAGS)
endif
else
    override CFLAGS += -DNDEBUG -O3 -ffast-math -ftree-vectorize
endif

ifeq ($(DMALLOC),yes)
    override CFLAGS += -DDMALLOC -DTEST_ALLOC=4
    LIBS += -ldmalloc
endif

ifeq ($(PROFILE),yes)
    override CFLAGS += -O1 -pg
    override LINK_FLAGS += -pg
endif

# See the comments for `clean`, but basically if --check (--check-symlink-times)
# is passed, $MAKEFLAGS will contain "L".
ifneq ($(findstring L,$(MAKEFLAGS)),)
    override CFLAGS += -DTEST_FAST
endif

COMMON_SRCS := $(wildcard src/*.c)
COMMON_OBJS := $(COMMON_SRCS:.c=.o)

TEST_OUT := test/test
TEST_SRCS := $(wildcard test/*.c)
TEST_OBJS := $(TEST_SRCS:.c=.o)

.PHONY: all clean test tags

all:
	@echo "There is no interpreter yet.  ̀(•́︿•̀)ʹ"

test/%.o: test/%.c
	$(CC) $(subst std=c99,std=c11,$(CFLAGS)) $(INCLUDE_DIRS) -c $< -o $@

src/lex.i: src/lex.l
	$(FLEX) -o $@ $<

src/lexer.o: src/lexer.c src/lex.i
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

test-bin: $(TEST_OBJS) $(COMMON_OBJS) deps  ## Build the test binary — run as \\033[037m$MAKECMD test-bin --check\\033[0m to build a quick check suite
	$(CC) $(LINK_FLAGS) -Wl,-Ttest/test.ld $(TEST_OBJS) $(COMMON_OBJS) -o $(TEST_OUT) $(LIBS)

test: test-bin  ## Build and run the test suite
	./$(TEST_OUT)

tags:  ## Create a ctags file for the source tree
	$(CTAGS) -R src

clean:  ## Remove all build output — run as \\033[37m$MAKECMD clean --keep\\033[0m to keep bundled dependencies
# This is kind of cute, but something of a hack:
# Don't clean bundled dependencies if clean is invoked with --keep.
# GNU Make treats this as --keep-going (which is harmless), and puts
# "k" in $MAKEFLAGS.
# Use bash instead of sh for the [[ ]] syntax for testing $MAKEFLAGS.
	rm -f tags build.ninja src/*.o test/*.o src/*.d test/*.d $(TEST_OUT) src/preprocessed/test
	rm -f src/*.gcda src/*.gcno test/*.gcda test/*.gcno src/*.html test/*.html
	@echo 'if [[ "$(MAKEFLAGS)" != *k* ]]; \
	then \
	  echo 'rm -f dep/**/*.o dep/**/*.d dep/libdeps.a'; \
	  rm -f dep/**/*.o dep/**/*.d dep/libdeps.a; \
	  echo 'rm -f dep/**/*.gcda dep/**/*.gcno'; \
	  rm -f dep/**/*.gcda dep/**/*.gcno; \
	fi' | bash


######################################################################
# Build bundled dependencies. These get statically linked with       #
# explicit -l:libXYZ.a flags above. See $LIBS.                       #
######################################################################

deps:  ## Build a static library of bundled dependencies
	$(CC) $(CFLAGS) -c dep/SFMT/SFMT.c -o dep/SFMT/SFMT.o
	$(CC) $(CFLAGS) -include dep/utf8proc/utf8proc.h -c dep/utf8proc/utf8proc.c -o dep/utf8proc/utf8proc.o
	$(CC) $(CFLAGS) -include dep/utf8proc/utf8proc.h -c dep/utf8proc/utf8proc_data.c -o dep/utf8proc/utf8proc_data.o
	$(CC) $(CFLAGS) -c dep/linenoise.c -o dep/linenoise.o
	$(CC) $(CFLAGS) -c dep/shoco.c -o dep/shoco.o
	$(AR) rcs dep/lib$@.a dep/**/*.o


######################################################################
# This section is for debugging the giant macro ‘templates’ used for #
# common data structures.                                            #
# Hopefully, you shouldn't have to fiddle with this.                 #
######################################################################

.PHONY: copy-templates debug-test-bin debug-test

# Assign with = instead of := because there won't be any files in src/preprocessed/
# until copy_templates runs.
TMPL_SRCS = $(wildcard src/preprocessed/*.c)
TMPL_OBJS = $(TMPL_SRCS:.c=.o)

copy-templates:
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

debug-test-bin: copy-templates deps $(TMPL_OBJS) test/ptest.o $(COMMON_OBJS)  ## Build a special test binary for debugging certain internal Yu structures
	$(CC) $(LIB_DIRS) -Wl,-Ttest/test.ld test/ptest.o $(TMPL_OBJS) $(COMMON_OBJS) -o src/preprocessed/test $(LIBS)

debug-test: debug-test-bin  ## Build and start debugging the test suite
	$(DB) $(DBFLAGS) src/preprocessed/test


######################################################################
# Create a Ninja file for building the test suite with Ninja. This   #
# is pretty experimental and build times are pretty fast anyway, but #
# it makes for interesting tinkering. ;-)                            #
######################################################################

.PHONY: ninja

ninja:  ## Create a build file for the Ninja build system
	$(M4) $(M4FLAGS) -DCC="$(CC)" -DCFLAGS="$(CFLAGS)" \
	    -DINCLUDE_DIRS="$(INCLUDE_DIRS)" -DLINK_FLAGS="$(LINK_FLAGS)" \
	    -DLIBS="$(LIBS)" \
	    m4/ninja.m4 > build.ninja


######################################################################
# I wish more Makefiles provided a `make targets` command, so let's  #
# do that here. See:                                                 #
#  http://marmelab.com/blog/2016/02/29/auto-documented-makefile.html #
#  https://gist.github.com/prwhite/8168133                           #
#  https://gist.github.com/depressiveRobot/46002002beabe2e7b4fd      #
######################################################################

.PHONY: help targets

targets:  ## Print a list of available targets with short descriptions
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
	@sh -c "echo -e 'Common targets:'"
	@sh -c "echo -e '  • \033[36m$(MAKE)\033[0m\tBuild the Yu interpreter' | expand -t 40"
	@sh -c "echo -e '  • \033[36m$(MAKE) install\033[0m\tInstall binaries to $(PREFIX)' | expand -t 40"
	@sh -c "echo -e '  • \033[36m$(MAKE) test\033[0m\tBuild and run the test suite' | expand -t 40"
	@sh -c "echo -e '  • \033[36m$(MAKE) targets\033[0m\tList available targets' | expand -t 40"
	@sh -c "echo -e '  • \033[36m$(MAKE) ninja\033[0m\tCreate a build.ninja file' | expand -t 40"
	@sh -c "echo -e 'Options (default):'"
	@sh -c "echo -e '  • \033[36mDEBUG\033[0m \033[37m($(DEBUG))\033[0m\tBuild with debug or release flags' | expand -t 50"
	@sh -c "echo -e '  • \033[36mPREFIX\033[0m \033[37m($(PREFIX))\033[0m\tPrefix to install binaries' | expand -t 50"
	@sh -c "echo -e '  • \033[36mPROFILE\033[0m \033[37m($(PROFILE))\033[0m\tInstrument binaries for profiling' | expand -t 50"
	@sh -c "echo -e '  • \033[36mDMALLOC\033[0m \033[37m($(DMALLOC))\033[0m\tDebug memory issues and report on leaks (requires libdmalloc)' | expand -t 50"
	@sh -c "echo -e '  • \033[36mCOVERAGE\033[0m \033[37m($(COVERAGE))\033[0m\tCompile with code coverage information for use with gcov' | expand -t 50"


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
