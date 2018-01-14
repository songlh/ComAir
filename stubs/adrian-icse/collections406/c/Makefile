CXX=gcc
CFLAGS=-g
TARGET=collections406


collections406: collections406.o
	${CXX} ${CFLAGS} collections406.o -o ${TARGET}

collections406.o: collections406.c
	${CXX} ${CFLAGS} -c collections406.c -o collections406.o


clean:
	rm -rf *.o ${TARGET}
