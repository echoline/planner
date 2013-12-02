planner: planner.o
	gcc -g -o planner planner.o `pkg-config --libs gtk+-3.0` -Wl,--export-dynamic

planner.o: planner.c planner_ui.h
	gcc -g -c planner.c `pkg-config --cflags gtk+-3.0`

planner_ui.h: makedoth
	./makedoth planner.ui planner_ui.h

makedoth: makedoth.c
	gcc -g -o makedoth makedoth.c

clean:
	rm -f planner.o planner makedoth planner_ui.h
