CXX=gcc
CFLAGS=-g
TARGET=collections410


collections410: collections410.o
	${CXX} ${CFLAGS} collections410.o -o ${TARGET}

collections410.o: collections410.c
	${CXX} ${CFLAGS} -c collections410.c -o collections410.o


clean:
	rm -rf *.o ${TARGET}
