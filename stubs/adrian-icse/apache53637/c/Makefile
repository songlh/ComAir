CXX=gcc
CFLAGS=-g
TARGET=apache53637


apache53637: apache53637.o
	${CXX} ${CFLAGS} apache53637.o -o ${TARGET}

apache53637.o: apache53637.c
	${CXX} ${CFLAGS} -c apache53637.c -o apache53637.o


clean:
	rm -rf *.o ${TARGET}
