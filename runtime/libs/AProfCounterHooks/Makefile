CXX=clang
CFLAGS=-O2 -Xclang -disable-O0-optnone -flto -use-gold-plugin -Wl,-plugin-opt=save-temps
LDFLAGS=-flto -use-gold-plugin -Wl,-plugin-opt=save-temps -Xclang -disable-O0-optnone -O0

TARGET=prof-counter-hook.o


# prof-counter-hook
prof-counter-hook.o: aproflib.c
	${CXX} ${CFLAGS} -c aproflib.c -o prof-counter-hook.o

clean:
	rm -rf *.o ${TARGET}