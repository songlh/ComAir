CXX=clang++
CXXFLAGS=-O0
TARGET=graphcol


graphcol: graphcol.o
	${CXX} ${CFLAGS} graphcol.o -o ${TARGET}

graphcol.o: graphcol-base.cpp
	${CXX} ${CXXFLAGS} -c graphcol-base.cpp -o graphcol.o


clean:
	rm -rf *.o ${TARGET}