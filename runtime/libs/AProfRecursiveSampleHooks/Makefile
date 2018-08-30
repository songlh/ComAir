CXX=clang
EXE_CFLAGS=-O2
CFLAGS=-O2 -Xclang -disable-O0-optnone -flto -use-gold-plugin -Wl,-plugin-opt=save-temps
LDFLAGS=-flto -use-gold-plugin -Wl,-plugin-opt=save-temps -Xclang -disable-O0-optnone -O0

TARGET=prof-rs-hook.o

ALL: prof-rs-hook.o

# recursive smple hooks
prof-rs-hook.o: aproflib.c
	${CXX} ${CFLAGS} -c aproflib.c -o prof-rs-hook.o


clean:
	rm -rf *.o ${TARGET}