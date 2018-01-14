CXX=gcc
CFLAGS=-g
TARGET=collections434


collections434: collections434.o
	${CXX} ${CFLAGS} collections434.o -o ${TARGET}

collections434.o: collections434.c
	${CXX} ${CFLAGS} -c collections434.c -o collections434.o


clean:
	rm -rf *.o ${TARGET}
