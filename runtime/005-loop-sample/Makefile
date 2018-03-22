CXX=clang
CFLAGS=-O2 -Xclang -disable-O0-optnone -flto -use-gold-plugin -Wl,-plugin-opt=save-temps
CFLAGS=-flto -use-gold-plugin -Wl,-plugin-opt=save-temps -Xclang -disable-O0-optnone -O2

TARGET=aproflib


aproflib-loop-sample.bc: aproflib.c
	${CXX} ${CFLAGS} -c aproflib.c -o aproflib-loop-sample.bc


clean:
	rm -rf *.bc ${TARGET}