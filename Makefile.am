
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = smog

smog_CPPFLAGS = -Isrc/ -Wall -Wextra
smog_CFLAGS = @LIBYAML_CFLAGS@ @PTHREAD_CFLAGS@
smog_LDADD = @LIBYAML_LIBS@ @PTHREAD_LIBS@
AM_YFLAGS = -d

smog_SOURCES = src/smog.c src/smog.h \
               src/args.c src/args.h \
               src/util.c src/util.h \
               src/parser.y src/lexer.c src/lexer.h \
               src/buffer.c src/buffer.h \
               src/thread.c src/thread.h \
               src/kernel.c src/kernel.h src/kerneltypes.h \
               src/monitor.c src/monitor.h \
               src/kernels/cold.c src/kernels/cold.h \
               src/kernels/dirty_pages.c src/kernels/dirty_pages.h \
               src/kernels/linear_scan.c src/kernels/linear_scan.h \
               src/kernels/pointer_chase.c src/kernels/pointer_chase.h \
               src/kernels/random_access.c src/kernels/random_access.h

BUILT_SOURCES = src/parser.h
