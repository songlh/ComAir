CXX=clang
CFLAGS=-O2 -Xclang -disable-O0-optnone -flto
TARGET=aproflib


aproflib.bc: aproflib.c
	${CXX} ${CFLAGS} -c aproflib.c -o aproflib.bc


clean:
	rm -rf *.bc ${TARGET}