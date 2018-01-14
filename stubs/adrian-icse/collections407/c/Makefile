CXX=gcc
CFLAGS=-g
TARGET=collections407


collections407: collections407.o
	${CXX} ${CFLAGS} collections407.o -o ${TARGET}

collections407.o: collections407.c
	${CXX} ${CFLAGS} -c collections407.c -o collections407.o


clean:
	rm -rf *.o ${TARGET}
