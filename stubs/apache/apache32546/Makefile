CXX=g++
CFLAGS=-O2
TARGET=apache32546


apache32546: apache32546.o
	${CXX} ${CFLAGS} apache32546.o -o ${TARGET}

apache32546.o: apache32546.c
	${CXX} ${CFLAGS} -c apache32546.cpp -o apache32546.o


clean:
	rm -rf *.o ${TARGET}
