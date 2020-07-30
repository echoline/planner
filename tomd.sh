#!/bin/sh
# example script to convert planner data to markdown format

MONTHS="January February March April May June July August September October November December"

if test -z $title; then
	u=$user
	if test -z $u; then
		u=$USER;
	fi
	u=$( echo $( echo $u | awk '{ print substr($0, 1, 1) }' | tr '[a-z]' '[A-Z]' )$( echo $u | awk '{ print substr($0, 2, length($0)-1) }' ) )

	echo $u\'s planner data
	echo =====
	echo
else
	echo $title
	echo =====
	echo
fi

DIR=$1
if test -z $DIR; then
	DIR=.
fi

for YYYY in $DIR/[1-2][0-9][0-9][0-9]; do
	for MM in $YYYY/[0-1][0-9]; do
		for DD in $MM/[0-3][0-9]; do
			if test -f $DD/index.md; then
				echo -n $(echo $(basename $DD) | sed "s,^0,,");
				echo -n " ";
				echo -n $(echo $MONTHS | awk "{ print $"$(basename $MM)" }");
				echo -n ", ";
				echo $(basename $YYYY);
				echo ----
				cat $DD/index.md;
				echo; echo;
			fi
		done
	done
done

