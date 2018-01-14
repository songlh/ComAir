CXX=gcc
CFLAGS=-g
TARGET=collections429


collections429: collections429.o
	${CXX} ${CFLAGS} collections429.o -o ${TARGET}

collections429.o: collections429.c
	${CXX} ${CFLAGS} -c collections429.c -o collections429.o


clean:
	rm -rf *.o ${TARGET}
