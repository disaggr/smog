CXX=g++
CXXFLAGS=-I. -lboost_program_options -lstdc++ -lpthread -Wall -Wextra -g

smog: smog.o kernel.o linear_scan.o random_access.o random_write.o pointer_chase.o cold.o dirty_pages.o
	$(CXX) -o smog smog.o kernel.o linear_scan.o random_access.o random_write.o pointer_chase.o cold.o dirty_pages.o $(CXXFLAGS)

clean:
	$(RM) *.o
