#!/usr/bin/python3
# Copyright (c) 2014 Wojciech A. Koszek <wkoszek@FreeBSD.org>
#
# Usage:
#
# ./word_mixup.py file_in file_out MB_num
# It takes words from file_in, and randomizes them with random repeating
# up to a point where file_out is at least MB_num or MegaBytes.
#

import	sys
import	random
import	time

#print(sys.argv)
#print(len(sys.argv))

if len(sys.argv) < 4:
	print("{0} file_in file_out, MB_num", sys.argv[0])
	sys.exit(64)

[ g_fn_in, g_fn_out, size ] = sys.argv[1:4]

g_size_limit = int(size) * 2**20;

print("# input file '" + g_fn_in + "', output file '" + g_fn_out,
		"', size(MB): " + str(size))

with open(g_fn_in, "r") as fi:
	data = [line.strip() for line in fi.read().split("\n")]
fi.close();

with open(g_fn_out, "w", 10*2**20) as fo:
	size = 0
	wrote_1mb = 0
	while size < g_size_limit:
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
		fo.write(to_wr)
		size += len(to_wr)

		# Progress
		wrote_1mb += len(to_wr)
		if wrote_1mb >= 2**20:
			wrote_1mb = 0
			sys.stdout.write('.');
			sys.stdout.flush();
	print("");
fo.close();
