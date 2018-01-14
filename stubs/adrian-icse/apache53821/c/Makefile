CXX=g++
CFLAGS=-g
TARGET=apache53821


apache53821: apache53821.o
	${CXX} ${CFLAGS} apache53821.o -o ${TARGET}

apache53821.o: apache53821.c
	${CXX} ${CFLAGS} -c apache53821.c -o apache53821.o


clean:
	rm -rf *.o ${TARGET}