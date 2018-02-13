
CXX=gcc
CFLAGS=-O2
TARGET=mozilla35294


mozilla35294: mozilla35294.o
	${CXX} ${CFLAGS} mozilla35294.o -o ${TARGET}

mozilla35294.o: mozilla35294.c
	${CXX} ${CFLAGS} -c mozilla35294.c -o mozilla35294.o


clean:
	rm -rf *.o ${TARGET}
