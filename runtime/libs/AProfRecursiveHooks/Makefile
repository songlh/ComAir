CXX=clang
EXE_CFLAGS=-O2
CFLAGS=-O2 -Xclang -disable-O0-optnone -flto -use-gold-plugin -Wl,-plugin-opt=save-temps
LDFLAGS=-flto -use-gold-plugin -Wl,-plugin-opt=save-temps -Xclang -disable-O0-optnone -O0

TARGET=prof-recu-hook.o

ALL: prof-recu-hook.o

# aproflib-loop-array-linkedlist
prof-recu-hook.o: aproflib.c
	${CXX} ${CFLAGS} -c aproflib.c -o prof-recu-hook.o


clean:
	rm -rf *.o ${TARGET}