#!/bin/sh

(
	cd wclarge/
	make check.small
	make check.big
	make clean
)
(
	cd pattern_search/
	make init
	make time
	make check
	make clean
)
