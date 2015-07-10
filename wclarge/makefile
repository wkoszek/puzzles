wclarge: wclarge.c makefile
	$(CC) $(CFLAGS) -std=c99 -Wall -pedantic -o wclarge wclarge.c  -lpthread

fetch:
	curl -L -o cain.txt.bz2 http://downloads.skullsecurity.org/passwords/cain.txt.bz2
showdict:
	bzcat cain.txt.bz2 | sort | uniq -c | awk '{ print $$2,$$1 }'	
unpack:
	rm -rf cain.txt
	bzcat cain.txt.bz2 | grep -v " " | grep -v '^$$' > cain.txt
	./word_mixup.py _.tmp data.2.i 20

clean.i:
	rm -rf dat.i dat.o
prep.1: clean.i
	ln -s data.1.i dat.i 
prep.2: clean.i
	ln -s data.2.i dat.i

check: wclarge
	sort dat.i | uniq -c | sort | sed 's/ //g' > _.sys.0
	./wclarge -o 52428800 -n 4 dat.i | sort | sed 's/ //g' > _.prog.0
	diff -u _.sys.0 _.prog.0

clean:
	rm -rf wclarge _.* dat.i
mrproper: clean
	rm -rf cain.txt.bz2

check.1: prep.1 check
check.2: prep.2 check

.PHONY: prep.1 prep.2 check clean
