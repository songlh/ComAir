CXX=clang++
CXXFLAGS=-O0
TARGET=fib


fib: fib.o
	${CXX} ${CFLAGS} fib.o -o ${TARGET}

fib.o: fib-base.cpp
	${CXX} ${CXXFLAGS} -c fib-base.cpp -o fib.o


clean:
	rm -rf *.o ${TARGET}