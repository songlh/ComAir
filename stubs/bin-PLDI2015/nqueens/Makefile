CXX=clang++
CXXFLAGS=-O0
TARGET=nqueens


nqueens: nqueens.o
	${CXX} ${CFLAGS} nqueens.o -o ${TARGET}

nqueens.o: nqueens-base.cpp
	${CXX} ${CXXFLAGS} -c nqueens-base.cpp -o nqueens.o


clean:
	rm -rf *.o ${TARGET}