all: btc compare

btc: btc.c pgm.c utils.c btc.h main.c 
	gcc $^ -O3 -lm -o btc

compare: btc.c pgm.c utils.c btc.h compare.c
	gcc $^ -O3 -lm -o compare

clean:
	rm -f compare btc *.pgm *.btc
