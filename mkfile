</$objtype/mkfile

TARG=planner

planner: planner-plan9.$O
	$O^l -o planner planner-plan9.$O

planner-plan9.$O: planner-plan9.c
	$O^c planner-plan9.c

clean:
	rm -f planner-plan9.[$OS] planner
