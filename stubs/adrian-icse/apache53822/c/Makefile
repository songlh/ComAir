CXX=gcc
CFLAGS=-g
TARGET=apache53822


apache53822: apache53822.o
	${CXX} ${CFLAGS} apache53822.o -o ${TARGET}

apache53822.o: apache53822.c
	${CXX} ${CFLAGS} -c apache53822.c -o apache53822.o


clean:
	rm -rf *.o ${TARGET}
