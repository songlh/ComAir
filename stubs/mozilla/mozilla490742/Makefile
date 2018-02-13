CXX=gcc
CFLAGS=-O2 -g
TARGET=mozilla490742


mozilla477564: mozilla490742.o
	${CXX} ${CFLAGS} mozilla490742.o -o ${TARGET} -l sqlite3

mozilla477564.o: mozilla490742.c
	${CXX} ${CFLAGS} -c mozilla490742.c -o mozilla477564.o


clean:
	rm -rf *.o ${TARGET}
