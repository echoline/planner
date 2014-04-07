planner: planner.o
	gcc -g -o planner planner.o `pkg-config --libs gtk+-3.0` -Wl,--export-dynamic

planner.o: planner.c 
	gcc -g -c planner.c `pkg-config --cflags gtk+-3.0`

clean:
	rm -f planner.o planner 
