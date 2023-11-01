
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = smog

smog_CPPFLAGS = -Isrc/ -Wall -Wextra
smog_CFLAGS = @LIBYAML_CFLAGS@
smog_CXXFLAGS = $(smog_CFLAGS)
smog_LDADD = @LIBYAML_LIBS@
AM_YFLAGS = -d

smog_SOURCES = src/smog.cpp src/smog.h \
               src/args.c src/args.h \
               src/util.c src/util.h \
               src/parser.y src/lexer.c src/lexer.h \
               src/monitor.cpp src/monitor.h \
               src/kernel.cpp src/kernel.h \
               src/kernels/all.h \
               src/kernels/kernels.c src/kernels/kernels.h \
               src/kernels/cold.cpp src/kernels/cold.h \
               src/kernels/dirty_pages.cpp src/kernels/dirty_pages.h \
               src/kernels/linear_scan.cpp src/kernels/linear_scan.h \
               src/kernels/pointer_chase.cpp src/kernels/pointer_chase.h \
               src/kernels/random_access.cpp src/kernels/random_access.h \
               src/kernels/random_write.cpp src/kernels/random_write.h

BUILT_SOURCES = src/parser.h
