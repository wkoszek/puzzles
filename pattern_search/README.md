Woogle - simple search for a pattern in 1M word DB with sorted output
---------------------------------------------------------------------
Copyright (c) 2014 Wojciech A. Koszek <wkoszek@FreeBSD.org>

The idea is that you have 1M words. Each word has a score.
You want to print out all words having a pattern so that they're
sorted out by a score. You only want top 10 results.

Constraints:
- can use any language
- can use any library
- fit within couple of hours of development time
- should be better than O(N)
- should display top 10 results sorted by score.

Quickstart
----------

	make
	./woogle_index.py --input words.db --dict dictionary.txt
	./woogle_index.py --input words.db --search PATTERN

My hackish solution
-------------------

NOTE: Probably doesn't meet "better than O(N) requirement"

Didn't have enough time to find anything custom made.
Recognized solution would fit nicely within SQLite implementation,
since regular expression matching is already there, and SQLite 
has ORDER BY and LIMIT for limiting.

Details
-------

I started from fetching the dictionary file

	http://docs.oracle.com/javase/tutorial/collections/interfaces/examples/dictionary.txt

You'll get it fetched by typing:

	make

To init databases:

	make init

Make init will call `woogle_index.py`, take the dictionary, assign random
score to the word and create a SQLite database on your disk.

The makefile will make 'dictionary-big.txt' too. Record difference:

	$ wc -l dictionary*.txt
	 2411040 dictionary-big.txt
	   80368 dictionary.txt

So -big version is 30x bigger.

To time the results for 2 patterns:

	make time

Just did very brief experiments:

Avg time for dictionary: 0.46s.
Avg time for dictionary-big: 12s

Notes
-----

dictionary.txt in this repository was fetched from:

	http://docs.oracle.com/javase/tutorial/collections/interfaces/examples/dictionary.txt

