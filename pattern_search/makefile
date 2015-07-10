all: dictionary.txt dictionary-big.txt

dictionary-big.txt: makefile dictionary.txt
	seq 1 20 | xargs -n 1 -I @ echo "cat dictionary.txt >> dictionary-big.txt" | sh

init: dictionary.txt dictionary-big.txt
	./woogle_index.py --input words.db --dict dictionary.txt
	./woogle_index.py --input words-big.db --dict dictionary-big.txt

time: words-big.db words.db
	time ./woogle_index.py --input words.db --search jo > jo.o
	time ./woogle_index.py --input words.db --search ab > ab.o
	time ./woogle_index.py --input words-big.db --search jo > jo-big.o
	time ./woogle_index.py --input words-big.db --search ab > ab-big.o

check:
	@echo "Checking output (no output means everything's ok)"
	@diff -u jo.o test/jo.t
	@diff -u ab.o test/ab.t
	@diff -u jo-big.o test/jo-big.t
	@diff -u ab-big.o test/ab-big.t

clean:
	rm -rf dictionary-big.txt words-big.db words.db *.o
