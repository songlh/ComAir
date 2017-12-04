CXX=clang
CFLAGS=-O2
TARGET=Telnet


Telnet: Telnet.o
	${CXX} ${CFLAGS} Telnet.o -o ${TARGET}

Telnet.o: Telnet.c
	${CXX} ${CFLAGS} -c Telnet.c -o Telnet.o


clean:
	rm -rf *.o ${TARGET}