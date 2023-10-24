
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = smog

smog_CPPFLAGS = -Isrc/ -Wall -Wextra
smog_LDADD = -lyaml

smog_SOURCES = src/smog.cpp src/smog.h \
               src/args.c \
               src/util.c src/util.h \
               src/parser.c src/parser.h \
               src/monitor.cpp src/monitor.h \
               src/kernel.cpp src/kernel.h \
               src/kernels/kernels.c src/kernels/kernels.h \
               src/kernels/cold.cpp src/kernels/cold.h \
               src/kernels/dirty_pages.cpp src/kernels/dirty_pages.h \
               src/kernels/linear_scan.cpp src/kernels/linear_scan.h \
               src/kernels/pointer_chase.cpp src/kernels/pointer_chase.h \
               src/kernels/random_access.cpp src/kernels/random_access.h \
               src/kernels/random_write.cpp src/kernels/random_write.h
