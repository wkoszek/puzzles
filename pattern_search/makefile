all: dictionary.txt dictionary-big.txt

URL=http://docs.oracle.com/javase/tutorial/collections/interfaces/examples/dictionary.txt

dictionary.txt: makefile
	wget -O dictionary.txt ${URL}

dictionary-big.txt: makefile dictionary.txt
	seq 1 20 | xargs -n 1 -i@ echo "cat dictionary.txt >> dictionary-big.txt" | sh

init: dictionary.txt dictionary-big.txt
	./woogle_index.py --input words.db --dict dictionary.txt
	./woogle_index.py --input words-big.db --dict dictionary-big.txt

time: words-big.db words.db
	time ./woogle_index.py --input words.db --search jo
	time ./woogle_index.py --input words.db --search ab
	time ./woogle_index.py --input words-big.db --search jo
	time ./woogle_index.py --input words-big.db --search ab
