Large WC(1)
-----------
Copyright (c) 2014 by Wojciech A. Koszek <wkoszek@FreeBSD.org>

The programming problem:

There's 120 GB text file and a UNIX box with 4GB of RAM and many CPU cores.
Write a program/script that outputs a file listing the frequency of all
words in the file (i.e. a TSV file with two columns <word, frequency>). Note
that the set of words in the file may not fit in memory.

Quickstart
----------

	make fetch
	make unpack
	make check.1	# will check correctness against system utility (small file)
	make check.2	# same as check.1, but for the big file.

Research
--------

There was already a solution implemented in Python:

	https://github.com/swarajban/multithreadedWordCounting

The solution method is based on trie structure, and the serialization is
done to the disk. The way the trie is serialized is taking advantage of
direct mapping of trie (keys) -> filesystem (directories).

Wanted to try something else for a solution.

Haven't done extensive testing.

Solution
--------

For my solution, due to limited time, I've made several "simplifying"
assumptions. I assumed OS is running on 64-bit machine.

I also decided to use a Trie. My implementation is in ANSI C. In the
production code this should become a more modular/cleanup program.

In UNIX we'd get this via:

	sort bigfile.db | uniq -c

Solution uses an assumption that using mmap()ing an input file will let the
OS handle memory swapping.  The further the input file you seek in, the old
pages from VM subsystem will be pruned or moved to a backing store (swap).
This is pretty straighforward.

For output I use anonymous map, where I ftruncate() the output file to a
predicted trie size, and I allocate from this too. Basically for this also
OS should take care of moving memory back and forth from the physical
memory.

The whole thing is multi-threaded. The program takes an input file mapped in
the memory and assumes N threads. Memory is divided into 4 chunks. Each
chunk is "tasted" and size is adjusted so that each chunk has a complete
"words" (no chunk ends within a word). Then threads are started.

Each worker thread accesses the memory within its chunk. Chunk memory
begin/end is known to the thread. Figuring out words is based on scanning
consecutive bytes until 'isalpha()' C API function returns true, in which
case we know we've finished parsing the word.

The addition to the trie is protected by a mutex. I had no time to optimize
that, so there's 1 mutex per 4 threads and it'll be the major source of
contention in this program, besides the main memory.

The problem with the trie is that it may not fit into the memory. I feel my
implementation will have problem with words "AAAAAAAAAAAA" and "ZZZZZZZZZZZ"
being next to each other, since the Trie leaf for "AAAAAAAAAAAAA" and Trie's
leaf for "ZZZZZZZZZZZZZZZZ" will be far apart, so the paging will be very
expensive. In the target implementation I'd try to implement:

	read chunk of input file
	sort words
	for each word:
		add to trie

Negative effect of my implementation is that Trie isn't easily serialized.
Swaraj's implementation is thus better in this regard.

Testing
-------

To fetch the same dictionary:

	make fetch

To understand the content of the dictionary, you do:

	make showdict

The problem with the dictionary is that it's full of correct English words,
but each is unique -- they're non repeating. To provide myself a real test
data I wrote a program which takes file as input and writes N GB file with
randomly chosen words repeated random number of times.

There's a Python program `word_mixup.py` for solving this. I read all words
from a file. Then I randomly select words and repeat them random number of
files. This all happens until output file is N MBs big (N is specified from
the command line) It's all done via:

	make unpack

Then you should be able to test my solution on a simple file:

	make check.1

And also on a newly created bigger file with word repetitions:

	make check.2

Cleanup:

	make clean

Development notes
-----------------
scanf() has problems with spaces unless you specify the formatting string
with regular expression.

awk $1/$2 also handles spaces wrongly.
