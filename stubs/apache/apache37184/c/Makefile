CXX=gcc
CFLAGS=-O2
TARGET=apache37184


apache37184: apache37184.o
	${CXX} ${CFLAGS} apache37184.o -o ${TARGET}

apache37184.o: apache37184.c
	${CXX} ${CFLAGS} -c apache37184.c -o apache37184.o


clean:
	rm -rf *.o ${TARGET}
