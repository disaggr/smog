CXX=g++
CXXFLAGS=-I. -lboost_program_options -lstdc++ -lpthread -Wall -Wextra -g

smog: smog.o kernel.o linear_scan.o random_access.o random_write.o pointer_chase.o cold.o dirty_pages.o
	$(CXX) -o smog smog.o kernel.o linear_scan.o random_access.o random_write.o pointer_chase.o cold.o dirty_pages.o $(CXXFLAGS)

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
	$(RM) *.o

veryclean:
	$(RM) *.o smog smog-O*
