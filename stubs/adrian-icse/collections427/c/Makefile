CXX=gcc
CFLAGS=-g
TARGET=collections427


collections427: collections427.o
	${CXX} ${CFLAGS} collections427.o -o ${TARGET}

collections427.o: collections427.c
	${CXX} ${CFLAGS} -c collections427.c -o collections427.o


clean:
	rm -rf *.o ${TARGET}
