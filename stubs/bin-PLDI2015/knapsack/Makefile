CXX=clang++
CXXFLAGS=-O0
TARGET=knapsack


knapsack: knapsack.o getoptions.o
	${CXX} ${CFLAGS} knapsack.o getoptions.o -o ${TARGET}

knapsack.o: knapsack-base.cpp
	${CXX} ${CXXFLAGS} -c knapsack-base.cpp -o knapsack.o

getoptions.o: getoptions.cpp
	${CXX} ${CXXFLAGS} -c getoptions.cpp -o getoptions.o


clean:
	rm -rf *.o ${TARGET}