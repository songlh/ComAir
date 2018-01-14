CXX=gcc
CFLAGS=-g
TARGET=apache53622


apache53622: apache53622.o
	${CXX} ${CFLAGS} apache53622.o -o ${TARGET}

apache53622.o: apache53622.c
	${CXX} ${CFLAGS} -c apache53622.c -o apache53622.o


clean:
	rm -rf *.o ${TARGET}
