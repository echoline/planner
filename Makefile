planner: planner.c planner_ui.h
	gcc -g -o planner `pkg-config --cflags --libs gtk+-2.0` planner.c -Wl,--export-dynamic -Wall

planner_ui.h: makedoth
	./makedoth planner.ui planner_ui.h

clean:
	rm planner makedoth planner_ui.h
