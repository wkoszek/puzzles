wclarge: wclarge.c makefile
	$(CC) $(CFLAGS) -std=c99 -Wall -pedantic -o wclarge wclarge.c  -lpthread

cain.txt.bz2:
	curl -L -o cain.txt.bz2 http://downloads.skullsecurity.org/passwords/cain.txt.bz2

data.big.i: cain.txt.bz2
	bzcat cain.txt.bz2 | grep -v " " | grep -v '^$$' | grep -v '^[0-9]' > cain.txt
	./word_mixup.py cain.txt data.big.i 20
	cat data.big.i | sort | uniq -c | sort | sed 's/ //g' > data.big.t

check.big: wclarge data.big.i
	./wclarge -o 52428800 -n 4 data.big.i | col -b | sort | sed 's/ //g' > data.big.o
	diff -u data.big.t data.big.o

check.small: wclarge
	./wclarge -o 52428800 -n 4 data.small.i | sort | sed 's/ //g' > data.small.o
	diff -u data.small.t data.small.o

clean:
	rm -rf wclarge _.* *.i *.o cain.txt

mrproper: clean
	rm -rf cain.txt.bz2

.PHONY: check.small check.big clean
