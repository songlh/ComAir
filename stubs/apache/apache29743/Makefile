
CXX=g++
CFLAGS=-O2
TARGET=apache29743


apache29743: apache29743.o
	${CXX} ${CFLAGS} apache29743.o -o ${TARGET}

apache29743.o: apache29743.cpp
	${CXX} ${CFLAGS} -c apache29743.cpp -o apache29743.o


clean:
	rm -rf *.o ${TARGET}
