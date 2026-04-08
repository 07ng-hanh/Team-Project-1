CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g

all: simulator

simulator: main.o stock.o backtest.o trade.o fileio.o utils.o
	$(CC) $(CFLAGS) -o simulator main.o stock.o backtest.o trade.o fileio.o utils.o

main.o: main.c stock.h backtest.h trade.h fileio.h
	$(CC) $(CFLAGS) -c main.c

stock.o: stock.c stock.h utils.h
	$(CC) $(CFLAGS) -c stock.c

backtest.o: backtest.c backtest.h stock.h trade.h
	$(CC) $(CFLAGS) -c backtest.c

trade.o: trade.c trade.h stock.h
	$(CC) $(CFLAGS) -c trade.c

fileio.o: fileio.c fileio.h stock.h
	$(CC) $(CFLAGS) -c fileio.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f *.o simulator *.txt

run: simulator
	./simulator