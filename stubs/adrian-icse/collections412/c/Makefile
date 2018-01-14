CXX=gcc
CFLAGS=-g
TARGET=collections412


collections412: collections412.o
	${CXX} ${CFLAGS} collections412.o -o ${TARGET}

collections412.o: collections412.c
	${CXX} ${CFLAGS} -c collections412.c -o collections412.o


clean:
	rm -rf *.o ${TARGET}
