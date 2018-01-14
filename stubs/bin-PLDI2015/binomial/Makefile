CXX=clang++
CXXFLAGS=-O0
TARGET=binomial


binomial: binomial.o
	${CXX} ${CFLAGS} binomial.o -o ${TARGET}

binomial.o: binomial-base.cpp
	${CXX} ${CXXFLAGS} -c binomial-base.cpp -o binomial.o


clean:
	rm -rf *.o ${TARGET}