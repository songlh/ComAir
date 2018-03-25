CXX=clang
EXE_CFLAGS=-O2
CFLAGS=-O2 -Xclang -disable-O0-optnone -flto -use-gold-plugin -Wl,-plugin-opt=save-temps
LDFLAGS=-flto -use-gold-plugin -Wl,-plugin-opt=save-temps -Xclang -disable-O0-optnone -O0

TARGET=prof-lps-hook.o

ALL: prof-lps-hook.o

# aproflib-loop-array-linkedlist
prof-lps-hook.o: aproflib.c
	${CXX} ${CFLAGS} -c aproflib.c -o prof-lps-hook.o


clean:
	rm -rf *.o ${TARGET}