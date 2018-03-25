CXX=clang
EXE_CFLAGS=-O2
CFLAGS=-O2 -Xclang -disable-O0-optnone -flto -use-gold-plugin -Wl,-plugin-opt=save-temps
LDFLAGS=-flto -use-gold-plugin -Wl,-plugin-opt=save-temps -Xclang -disable-O0-optnone -O0

TARGET=prof-lall-hook.o

ALL: prof-lall-hook.o

# aproflib-loop-array-linkedlist
prof-lall-hook.o: aproflib.c
	${CXX} ${CFLAGS} -c aproflib.c -o prof-lall-hook.o


clean:
	rm -rf *.o ${TARGET}