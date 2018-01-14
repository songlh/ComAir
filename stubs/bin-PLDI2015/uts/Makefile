CXX=clang++
CXXFLAGS=-O0
TARGET=uts


uts: uts.o brg_sha1.o
	${CXX} ${CFLAGS} uts.o brg_sha1.o -o ${TARGET} -lm

uts.o: uts-base.cpp
	${CXX} ${CXXFLAGS} -c uts-base.cpp -o uts.o

brg_sha1.o: brg_sha1.cpp
	${CXX} ${CXXFLAGS} -c brg_sha1.cpp -o brg_sha1.o

clean:
	rm -rf *.o ${TARGET}