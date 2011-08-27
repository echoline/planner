#include <gtk/gtk.h>
#include "planner_ui.h"

GtkBuilder *builder = NULL;
gchar *path = NULL;

void
save() {
	GtkTextView *widget;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar *text;

	if (path == NULL) {
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

void
on_calendar1_day_selected(GtkCalendar *widget, gpointer unused) {
	guint year, month, day;
	gchar *dir;

	gtk_calendar_get_date(widget, &year, &month, &day);

	if (path != NULL) {
		g_free(path);
		path = NULL;
	}

	dir = g_strdup_printf("%s/.plans/%d/%02d/%02d/", g_get_home_dir(), year, month + 1, day);

	if (g_mkdir_with_parents(dir, 0700) == 0) {
		path = g_strconcat(dir, "index.md", NULL);
		load();

	} else {
		fprintf(stderr, "unable to make directory %s\n", dir);

	}

	g_free(dir);
}

void
on_notes_paste_clipboard(GtkTextView *widget, gpointer unused) {
	save();
}

void
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
