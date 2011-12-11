#include <gtk/gtk.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "planner_ui.h"

GtkBuilder *builder = NULL;
gchar *path = NULL;
GtkWidget *window;

void
error(char *errstr) {
	GtkWidget *dialog = gtk_message_dialog_new (
				(GtkWindow*)window,
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				"%s", errstr);
	g_signal_connect_swapped (dialog, "response",
				G_CALLBACK (gtk_widget_destroy),
				dialog);
	gtk_window_set_icon_name((GtkWindow*)dialog, GTK_STOCK_NO);
	gtk_widget_show_all(dialog);
}

void
save() {
	GtkTextView *widget;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar *text, *errstr;

	if (path == NULL) {
		return;
	}

	widget = (GtkTextView*)gtk_builder_get_object(builder, "notes");
	buffer = gtk_text_view_get_buffer(widget);
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	text = gtk_text_buffer_get_slice(buffer, &start, &end, TRUE);

	if (g_file_set_contents(path, text, -1, NULL) == FALSE) {
		errstr = g_strdup_printf("unable to save to %s", path);
		error(errstr);
		g_free(errstr);
	}

	g_free(text);
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
	gchar *dir, *errstr;

	gtk_calendar_get_date(widget, &year, &month, &day);

	if (path != NULL)
		g_free(path);

#ifdef _WIN32
	dir = g_strdup_printf("%s/.plans/%d/%02d/%02d/", g_getenv("USERPROFILE"), year, month + 1, day);
#else
	dir = g_strdup_printf("%s/.plans/%d/%02d/%02d/", g_getenv("HOME"), year, month + 1, day);
#endif

	if (g_mkdir_with_parents(dir, 0700) == 0) {
		path = g_strconcat(dir, "index.md", NULL);
		load();

	} else {
		errstr = g_strdup_printf("unable to make directory %s", dir);
		error(errstr);
		g_free(errstr);

	}

	g_free(dir);
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
	GError *error = NULL;

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_string(builder, planner_ui, -1, &error);
	window = GTK_WIDGET(gtk_builder_get_object(builder, "calendar"));
	
	if (window == NULL)
		return 4;

	gtk_window_set_icon_name((GtkWindow*)window, GTK_STOCK_YES);

	on_calendar1_day_selected((GtkCalendar*)gtk_builder_get_object(builder, "calendar1"), NULL);

	gtk_builder_connect_signals(builder, NULL);

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	return main(__argc, __argv);
}
#endif
