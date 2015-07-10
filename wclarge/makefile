wclarge: wclarge.c makefile
	$(CC) $(CFLAGS) -std=c99 -Wall -pedantic -o wclarge wclarge.c  -lpthread

cain.txt.bz2:
	curl -L -o cain.txt.bz2 http://downloads.skullsecurity.org/passwords/cain.txt.bz2

data.big.i: cain.txt.bz2
	bzcat cain.txt.bz2 | grep -v " " | grep -v '^$$' > cain.txt
	./word_mixup.py cain.txt data/data.big.i 1000

check.big: wclarge data.big.i
	./test/wc_gen_ref data/data.big.i > data.big.t
	./wclarge -o 52428800 -n 4 data/data.big.i | col -b | sort | sed 's/ //g' > data.big.o
	diff -u data.big.t data.big.o

check.small: wclarge
	./test/wc_gen_ref data/data.small.i > data.small.t
	./wclarge -o 52428800 -n 4 data/data.small.i | sort | sed 's/ //g' > data.small.o
	diff -u data.small.t data.small.o

clean:
	rm -rf wclarge _.* *.i *.o cain.txt

mrproper: clean
	rm -rf cain.txt.bz2

.PHONY: check.small check.big clean
