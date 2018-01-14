CXX=gcc
CFLAGS=-g
TARGET=collections408


collections408: collections408.o
	${CXX} ${CFLAGS} collections408.o -o ${TARGET}

collections408.o: collections408.c
	${CXX} ${CFLAGS} -c collections408.c -o collections408.o


clean:
	rm -rf *.o ${TARGET}
