CXX=g++
CFLAGS=-g
TARGET=apache53803


apache53803: apache53803.o
	${CXX} ${CFLAGS} apache53803.o -o ${TARGET}

apache53803.o: apache53803.c
	${CXX} ${CFLAGS} -c apache53803.c -o apache53803.o


clean:
	rm -rf *.o ${TARGET}