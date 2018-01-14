CXX=clang++
CXXFLAGS=-O0
TARGET=minmax


minmax: minmax.o
	${CXX} ${CFLAGS} minmax.o -o ${TARGET}

minmax.o: minmax-base.cpp
	${CXX} ${CXXFLAGS} -c minmax-base.cpp -o minmax.o


clean:
	rm -rf *.o ${TARGET}