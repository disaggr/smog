CXX=g++
CXXFLAGS=-lboost_program_options -lstdc++ -lpthread -Wall -Wextra -g

smog: smog.cpp
	$(CXX) -o smog smog.cpp $(CXXFLAGS)
