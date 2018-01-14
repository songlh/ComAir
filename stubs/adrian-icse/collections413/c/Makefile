CXX=gcc
CFLAGS=-g
TARGET=collections413


collections413: collections413.o
	${CXX} ${CFLAGS} collections413.o -o ${TARGET}

collections413.o: collections413.c
	${CXX} ${CFLAGS} -c collections413.c -o collections413.o


clean:
	rm -rf *.o ${TARGET}
