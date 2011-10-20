planner.exe: planner.o
	gcc -g -o planner.exe planner.o -lmsvcrt -LC:/MinGW/lib -lgtk-win32-2.0 -lgdk-win32-2.0 -latk-1.0 -lgio-2.0 -lpangowin32-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lglib-2.0 -lintl -mwindows

planner.o: planner.c planner_ui.h
	gcc -g -c -mms-bitfields planner.c -IC:/MinGW/include/gtk-2.0 -IC:/MinGW/lib/gtk-2.0/include -IC:/MinGW/include/atk-1.0 -IC:/MinGW/include/cairo -IC:/MinGW/include/gdk-pixbuf-2.0 -IC:/MinGW/include/pango-1.0 -IC:/MinGW/include/glib-2.0 -IC:/MinGW/lib/glib-2.0/include -IC:/MinGW/include -IC:/MinGW/include/freetype2 -IC:/MinGW/include/libpng14 -Wall -Wl,--export-dynamic -mwindows

planner_ui.h: makedoth.exe
	./makedoth.exe planner.ui planner_ui.h

makedoth.exe: makedoth.c
	gcc -g -o makedoth makedoth.c

clean:
	rm -f planner.o planner.exe makedoth.exe planner_ui.h
