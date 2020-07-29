#!/bin/sh

ishg=0
if test -d .hg; then
	ishg=1
fi

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
				if test $ishg == 0; then
					mv index.txt index.md;
				else
					hg rename index.txt index.md || mv index.txt index.md;
				fi
			fi
			cd ..;
		done
		cd ..;
	done
	cd ..;
done
