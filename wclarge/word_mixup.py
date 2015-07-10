#!/usr/bin/env python3
# Copyright (c) 2014 Wojciech A. Koszek <wkoszek@FreeBSD.org>
#
# Usage:
#
# ./word_mixup.py file_in file_out wordnum
# It takes words from file_in, and randomizes them with random repeating
# up to a point where file_out has at least wordnum words
#

import	sys
import	random
import	time

def main():
    if len(sys.argv) < 4:
            print("{0} file_in file_out wordnum".format(sys.argv[0]))
            sys.exit(64)

    [ g_fn_in, g_fn_out, wordnum ] = sys.argv[1:4]

    g_wordnum = int(wordnum)

    print("# input file '" + g_fn_in + "', output file '" + g_fn_out,
                    "', wordnum:" + str(g_wordnum))

    with open(g_fn_in, "r") as fi:
            data = [line.strip() for line in fi.read().split("\n")]
    fi.close();

    wordnum_cur = 0
    with open(g_fn_out, "w") as fo:
            while wordnum_cur < g_wordnum:
                    # 1. Get some random bits
                    r = random.randint(0, 0xffffffff)

                    # Fetch the number of repetitions to make sure we're getting
                    # realistic repetitions
                    rep_num = r % 0x7
                    r >>= 3

                    # Get word and repeat it several times.
                    wi = r % len(data)
                    word = data[wi]
                    to_wr = ""
                    for i in range(0, rep_num + 1):
                            to_wr += word + "\n"
                            wordnum_cur += 1
                    fo.write(to_wr)

                    # Progress
                    if wordnum_cur % 1000 == 0:
                            sys.stdout.write('.')
                            sys.stdout.flush()
            print("");
    fo.close();

if __name__ == "__main__":
    main()
