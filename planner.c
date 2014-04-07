#include <gtk/gtk.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#endif

gchar *path = NULL;
gchar *dir = NULL;
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
	gtk_window_set_icon_name((GtkWindow*)dialog, "gtk-yes");
	gtk_widget_show_all(dialog);
}


gchar*
makepath(guint year, guint month, guint day) {
#ifdef _WIN32
	return g_strdup_printf("%s/.plans/%d/%02d/%02d/", g_getenv("USERPROFILE"), year, month + 1, day);
#else
	return g_strdup_printf("%s/.plans/%d/%02d/%02d/", g_getenv("HOME"), year, month + 1, day);
#endif
}

gchar*
details(GtkCalendar *calendar, guint year, guint month, guint day, gpointer user_data) {
	gchar *text;
	gchar *dir = makepath(year, month, day);
	gchar *path = g_strconcat(dir, "index.txt", NULL);

	g_free(dir);
	g_file_get_contents(path, &text, NULL, NULL);
	g_free(path);

	return text;
}

void
save(GtkTextView *widget) {
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar *text, *errstr;

	if (g_mkdir_with_parents(dir, 0700) != 0) {
		errstr = g_strdup_printf("unable to make directory %s", dir);
		error(errstr);
		g_free(errstr);

	}

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
load(GtkTextView *widget) {
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

	buffer = gtk_text_view_get_buffer(widget);
	gtk_text_buffer_set_text(buffer, text, length);

	g_free(text);
}

G_MODULE_EXPORT void
on_calendar1_day_selected(GtkCalendar *widget, gpointer arg) {
	guint year, month, day;
	gchar *errstr;

	gtk_calendar_get_date(widget, &year, &month, &day);

	if (path != NULL)
		g_free(path);

	if (dir != NULL)
		g_free(dir);

	dir = makepath(year, month, day);
	path = g_strconcat(dir, "index.txt", NULL);
	load(arg);

}

G_MODULE_EXPORT void
on_notes_paste_clipboard(GtkTextView *widget, gpointer unused) {
	save(widget);
}

G_MODULE_EXPORT void
on_notes_key_release_event(GtkTextView *widget, gpointer unused) {
	save(widget);
}

static gboolean
on_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
	cairo_set_source_rgba (cr, 1, 1, 1, 0.75);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr);

	return FALSE;
}

int main(int argc, char **argv) {
	GtkCalendar *calendar;
	GError *error = NULL;
	GdkScreen *screen;
	GdkVisual *visual;
	GtkWidget *notes;
	GtkWidget *grid;

	gtk_init(&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (window, "destroy", gtk_main_quit, NULL);

	gtk_widget_set_app_paintable (window, TRUE);
	screen = gdk_screen_get_default ();
	visual = gdk_screen_get_rgba_visual (screen);
	if (visual != NULL && gdk_screen_is_composited (screen)) {
		gtk_widget_set_visual (window, visual);
	}
	
	grid = gtk_grid_new ();
	gtk_container_add (GTK_CONTAINER (window), grid);

	notes = gtk_text_view_new ();
	gtk_widget_set_size_request (notes, 120, -1);
	gtk_widget_set_hexpand (notes, TRUE);
	gtk_widget_set_vexpand (notes, TRUE);
	g_signal_connect (notes, "draw", G_CALLBACK(on_draw), NULL);
	g_signal_connect (notes, "paste-clipboard", G_CALLBACK(on_notes_paste_clipboard), NULL);
	g_signal_connect (notes, "key-release-event", G_CALLBACK(on_notes_key_release_event), NULL);
	gtk_grid_attach (GTK_GRID (grid), notes, 1, 0, 1, 1);

	gtk_window_set_icon_name((GtkWindow*)window, "gtk-yes");

	calendar = GTK_CALENDAR (gtk_calendar_new ());
	gtk_widget_set_hexpand (GTK_WIDGET (calendar), FALSE);
	gtk_widget_set_vexpand (GTK_WIDGET (calendar), TRUE);
	gtk_calendar_set_detail_func(calendar, &details, NULL, NULL);
	gtk_calendar_set_detail_width_chars(calendar, 3);
	gtk_calendar_set_detail_height_rows(calendar, 1);
	on_calendar1_day_selected(calendar, notes);
	g_signal_connect (calendar, "day-selected", G_CALLBACK(on_calendar1_day_selected), notes);
	gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (calendar), 0, 0, 1, 1);

	gtk_window_set_title((GtkWindow*)window, "Planner");

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	return main(__argc, __argv);
}
#endif
