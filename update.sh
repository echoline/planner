#!/bin/sh

for Y in [1-2][0-9][0-9][0-9]; do
	if test ! -d $Y; then
		continue;
	fi;
	cd $Y;
	for M in [0-1][0-9]; do
		if test ! -d $M; then
			continue;
		fi;
		cd $M;
		for D in [0-3][0-9]; do
			if test ! -d $D; then
				continue;
			fi
			cd $D;
			if test -f index.txt; then
				mv index.txt index.md;
			fi
			cd ..;
		done
		cd ..;
	done
	cd ..;
done
