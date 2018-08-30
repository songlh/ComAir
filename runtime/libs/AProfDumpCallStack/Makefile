CXX=clang
CFLAGS=-O2 -Xclang -disable-O0-optnone -flto -use-gold-plugin -Wl,-plugin-opt=save-temps
LDFLAGS=-flto -use-gold-plugin -Wl,-plugin-opt=save-temps -Xclang -disable-O0-optnone -O0

TARGET=dump-call-stack-hook.o


# aprof-dump-hook
dump-call-stack-hook.o: dumpcallstacklib.c
	${CXX} ${CFLAGS} -c dumpcallstacklib.c -o dump-call-stack-hook.o

clean:
	rm -rf *.o ${TARGET}