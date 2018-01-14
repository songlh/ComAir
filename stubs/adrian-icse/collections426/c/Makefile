CXX=gcc
CFLAGS=-g
TARGET=collections426


collections426: collections426.o
	${CXX} ${CFLAGS} collections426.o -o ${TARGET}

collections426.o: collections426.c
	${CXX} ${CFLAGS} -c collections426.c -o collections426.o


clean:
	rm -rf *.o ${TARGET}
