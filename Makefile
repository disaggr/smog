CC=g++
CFLAGS=-lboost_program_options -lstdc++ -lpthread

smog: smog.cpp
	$(CC) -o smog smog.cpp $(CFLAGS)
