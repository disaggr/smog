CXX=g++
CFLAGS=-I. -lboost_program_options -lyaml -lstdc++ -lpthread -Wall -Wextra -g 
CXXFLAGS=$(CFLAGS)

BIN = smog

OBJ = smog.o \
      util.o \
      args.o \
      parser.o \
      kernel.o \
      monitor.o \
      kernels/kernels.o \
      kernels/linear_scan.o \
      kernels/random_access.o \
      kernels/random_write.o \
      kernels/pointer_chase.o \
      kernels/cold.o \
      kernels/dirty_pages.o

$(BIN): $(OBJ)
	$(CXX) -o $@ $(OBJ) $(CXXFLAGS)

opt:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -O1"
	mv smog smog-O1
	$(MAKE) clean
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -O2"
	mv smog smog-O2
	$(MAKE) clean
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -O3"
	mv smog smog-O3
	$(MAKE) clean
	$(MAKE)

clean:
	$(RM) $(OBJ)

veryclean:
	$(RM) $(OBJ) smog smog-O*
