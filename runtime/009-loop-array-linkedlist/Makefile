CXX=clang
CFLAGS=-O2 -Xclang -disable-O0-optnone -flto -use-gold-plugin -Wl,-plugin-opt=save-temps
CFLAGS=-flto -use-gold-plugin -Wl,-plugin-opt=save-temps -Xclang -disable-O0-optnone -O0

TARGET=aproflib

# aproflib-loop-array-linkedlist
proflall.o: aproflib.c
	${CXX} ${CFLAGS} -c aproflib.c -o proflall.o


clean:
	rm -rf *.o ${TARGET}