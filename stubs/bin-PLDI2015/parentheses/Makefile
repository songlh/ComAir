CXX=clang++
CXXFLAGS=-O0
TARGET=parentheses


parentheses: parentheses.o
	${CXX} ${CFLAGS} parentheses.o -o ${TARGET}

parentheses.o: parentheses-base.cpp
	${CXX} ${CXXFLAGS} -c parentheses-base.cpp -o parentheses.o


clean:
	rm -rf *.o ${TARGET}