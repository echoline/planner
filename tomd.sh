#!/bin/sh
# example script to convert planner data to markdown format

MONTHS="January February March April May June July August September October November December"

echo $USER\'s planner data
echo =====
echo

for YYYY in $HOME/.plans/[0-9][0-9][0-9][0-9]; do
	for MM in $YYYY/[0-1][0-9]; do
		for DD in $MM/[0-3][0-9]; do
			if test -f $DD/index.md; then
				echo -n $(echo $(basename $DD) | sed "s,^0,,");
				echo -n " ";
				echo -n $(echo $MONTHS | cut -d' ' -f$(basename $MM));
				echo -n " ";
				echo $(basename $YYYY);
				echo ----
				cat $DD/index.md;
				echo; echo;
			fi
		done
	done
done

