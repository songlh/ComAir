CXX=gcc
CFLAGS=-g
TARGET=collections409


collections409: collections409.o
	${CXX} ${CFLAGS} collections409.o -o ${TARGET}

collections409.o: collections409.c
	${CXX} ${CFLAGS} -c collections409.c -o collections409.o


clean:
	rm -rf *.o ${TARGET}
