#include <gtk/gtk.h>
#include <stdlib.h>
#include <windows.h>
#include "planner_ui.h"

GtkBuilder *builder = NULL;
gchar *path = NULL;
gchar *dir = NULL;

void
save() {
	GtkTextView *widget;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar *text;

	if (path == NULL) {
		return;
	}

	if (g_mkdir_with_parents(dir, 0700) != 0) {
		fprintf(stderr, "unable to make directory %s\n", dir);
		return;
	}

	widget = (GtkTextView*)gtk_builder_get_object(builder, "notes");
	buffer = gtk_text_view_get_buffer(widget);
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	text = gtk_text_buffer_get_slice(buffer, &start, &end, TRUE);

	if (g_file_set_contents(path, text, -1, NULL) == FALSE) {
		fprintf(stderr, "unable to save %s\n", path); 
	}
}

void
load() {
	GtkTextView *widget;
	GtkTextBuffer *buffer;
	gchar *text;
	gsize length;

	if (path == NULL) {
		return;
	}

	if (g_file_get_contents(path, &text, &length, NULL) == FALSE) {
		length = 0;
		text = g_strdup("");
	}

	widget = (GtkTextView*)gtk_builder_get_object(builder, "notes");
	buffer = gtk_text_view_get_buffer(widget);
	gtk_text_buffer_set_text(buffer, text, length);

	g_free(text);
}

G_MODULE_EXPORT void
on_calendar1_day_selected(GtkCalendar *widget, gpointer unused) {
	guint year, month, day;

	gtk_calendar_get_date(widget, &year, &month, &day);

	if (path != NULL)
		g_free(path);
	if (dir != NULL)
		g_free(dir);

	dir = g_strdup_printf("%s/.plans/%d/%02d/%02d/", g_getenv("USERPROFILE"), year, month + 1, day);
	path = g_strconcat(dir, "index.txt", NULL);

	load();
}

G_MODULE_EXPORT void
on_notes_paste_clipboard(GtkTextView *widget, gpointer unused) {
	save();
}

G_MODULE_EXPORT void
on_notes_key_release_event(GtkTextView *widget, gpointer unused) {
	save();
}

int main(int argc, char **argv) {
	GtkWidget *window;
	GError *error = NULL;

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_string(builder, planner_ui, -1, &error);
	window = GTK_WIDGET(gtk_builder_get_object(builder, "calendar"));
	
	if (window == NULL)
		return 4;

	on_calendar1_day_selected((GtkCalendar*)gtk_builder_get_object(builder, "calendar1"), NULL);

	gtk_builder_connect_signals(builder, NULL);

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	return main(__argc, __argv);
}
