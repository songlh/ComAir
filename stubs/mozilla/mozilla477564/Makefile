
CXX=gcc
CFLAGS=-O2 -g
TARGET=mozilla477564


mozilla477564: mozilla477564.o
	${CXX} ${CFLAGS} mozilla477564.o -o ${TARGET}

mozilla477564.o: mozilla477564.c
	${CXX} ${CFLAGS} -c mozilla477564.c -o mozilla477564.o


clean:
	rm -rf *.o ${TARGET}
