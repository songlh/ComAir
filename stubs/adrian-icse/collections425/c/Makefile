CXX=gcc
CFLAGS=-g
TARGET=collections425


collections425: collections425.o
	${CXX} ${CFLAGS} collections425.o -o ${TARGET}

collections425.o: collections425.c
	${CXX} ${CFLAGS} -c collections425.c -o collections425.o


clean:
	rm -rf *.o ${TARGET}
