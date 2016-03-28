divert(-1)
define(`quote', `ifelse(`$#',`0',`',``$*'')')
define(`dquote', ``$@'')
define(`dquote_elt', `ifelse(`$#',`0',`',`$#',`1',```$1''',```$1'',$0(shift($@))')')

define(`_arg1', `$1')

define(`foreach',  `pushdef(`$1')_$0(`$1', (dquote(dquote_elt$2)), `$3')popdef(`$1')')
define(`_foreach', `ifelse(`$2', `(`')', `', `define(`$1', _arg1$2)$3`'$0(`$1', (dquote(shift$2)), `$3')')')
define(`foreach_line', `foreach(`$1', (patsubst($2,`.+$',`\&`,'')), `ifelse(len($1),0,`',`$3')')')

define(`slice', `substr(`$1',`$2',eval(len(`$1')+`$3'))')

define(`in_file', `patsubst(`$1', `\.o', `.c')')
define(`out_file', `patsubst(`$1', `\.c', `.o')')

define(`build_objs', `foreach_line(`SRC_FILE', `esyscmd(`ls -1 $1')', `build out_file(SRC_FILE): cc SRC_FILE
'ifelse($2, `', `', `  $2
'))')

define(`link_static', `build $1: ar out_file(slice(patsubst(esyscmd(`ls' in_file(`$2')),`.+$',`\& $'),0,-2))')

define(`link_out', `build $1: ld out_file(slice(patsubst(esyscmd(`ls' in_file(`$2')),`.+$',`\& $'),0,-2))')

divert

cc = CC
include_dirs = INCLUDE_DIRS
cflags = CFLAGS $include_dirs

libs = LIBS -lpthread
link_flags = LINK_FLAGS

rule cc
  command = $cc -MMD -MT $out -MF $out.d $cflags -c $in -o $out
  description = Summoning object file $out
  depfile = $out.d

rule ar
  command = ar crs $out $in
  description = Creating static library $out

rule ld
  command = $cc $link_flags $in -o $out $libs
  description = Linking $out

rule lex
  command = flex -o $out $in
  description = Flexing


build src/lex.i: lex src/lex.l

build_objs(`src/*.c')
build_objs(`test/*.c', `cflags = patsubst(CFLAGS, `-std=c99', `-std=c11') $include_dirs')
build_objs(`dep/utf8proc/*.c', `cflags = -include utf8proc/utf8proc.h $cflags')
build_objs(`dep/SFMT/SFMT.c')
build_objs(`dep/linenoise/linenoise.c')
build_objs(`dep/shoco/shoco.c')
build_objs(`dep/nedmalloc/nedmalloc.c')

link_static(`dep/libdeps.a', `dep/**/*.o')

link_out(`test/test', `src/*.o test/*.o') | dep/libdeps.a src/lex.i
  link_flags = -Wl,-Ttest/test.ld $link_flags
