CXX=gcc
CFLAGS=-g
TARGET=groovy5739


groovy5739: groovy5739.o
	${CXX} ${CFLAGS} groovy5739.o -o ${TARGET}

groovy5739.o: groovy5739.c
	${CXX} ${CFLAGS} -c groovy5739.c -o groovy5739.o


clean:
	rm -rf *.o ${TARGET}
