CXX=gcc
CFLAGS=-O2
TARGET=StatCalculator


StatCalculator: StatCalculator.o
	${CXX} ${CFLAGS} StatCalculator.o -o ${TARGET}

StatCalculator.o: StatCalculator.c
	${CXX} ${CFLAGS} -c StatCalculator.c -o StatCalculator.o


clean:
	rm -rf *.o ${TARGET}
