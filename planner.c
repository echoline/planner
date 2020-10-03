#include <gtk/gtk.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#endif

gchar *path = NULL;
gchar *dir = NULL;
GtkWidget *window;
gboolean loadflag = 0;

typedef struct {
	GtkTextView *notes;
	GtkCalendar *calendar;
} GtkStuff;

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
	gchar *path = g_strconcat(dir, "index.md", NULL);

	g_free(dir);
	g_file_get_contents(path, &text, NULL, NULL);
	g_free(path);

	return text;
}

void
save(GtkTextBuffer *buffer) {
	GtkTextIter start, end;
	gchar *text, *errstr;

	if (g_mkdir_with_parents(dir, 0700) != 0) {
		errstr = g_strdup_printf("unable to make directory %s", dir);
		error(errstr);
		g_free(errstr);

	}

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

	loadflag = 1;
	buffer = gtk_text_view_get_buffer(widget);
	gtk_text_buffer_set_text(buffer, text, length);
	loadflag = 0;

	g_free(text);
}

G_MODULE_EXPORT void
on_calendar1_day_selected(GtkCalendar *widget, gpointer arg) {
	guint year, month, day;
	gchar *errstr;
	GtkStuff *stuff = (GtkStuff*)arg;

	gtk_calendar_get_date(stuff->calendar, &year, &month, &day);

	if (path != NULL)
		g_free(path);

	if (dir != NULL)
		g_free(dir);

	dir = makepath(year, month, day);
	path = g_strconcat(dir, "index.md", NULL);
	load(stuff->notes);
}

G_MODULE_EXPORT void
on_notes_changed(GtkTextBuffer *widget, gpointer arg) {
	if (loadflag)
		loadflag = 0;
	else
		save(widget);
	gtk_widget_queue_draw(GTK_WIDGET(((GtkStuff*)arg)->calendar));
}

G_MODULE_EXPORT void
on_today_clicked (GtkButton *widget, gpointer arg) {
	GDate date;
	GtkStuff *stuff = (GtkStuff*) (arg);

	g_date_set_time_t (&date, time (NULL));
	gtk_calendar_select_month (stuff->calendar,
				g_date_get_month (&date) - 1,
				g_date_get_year (&date));
	gtk_calendar_select_day (stuff->calendar,
				g_date_get_day (&date));
	on_calendar1_day_selected(stuff->calendar, stuff);
	loadflag = 0;
}

int main(int argc, char **argv) {
	GError *error = NULL;
	GtkWidget *grid;
	GtkWidget *tmp;
	GtkStuff *stuff = calloc(1, sizeof(GtkStuff));
	GValue val = G_VALUE_INIT;

	gtk_init(&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (window, "destroy", gtk_main_quit, NULL);
	gtk_window_set_resizable (GTK_WINDOW (window), 0);

	grid = gtk_grid_new ();
	gtk_container_add (GTK_CONTAINER (window), grid);

	stuff->notes = GTK_TEXT_VIEW (gtk_text_view_new ());
	gtk_text_view_set_wrap_mode (stuff->notes, GTK_WRAP_WORD);
	g_signal_connect (gtk_text_view_get_buffer(stuff->notes), "changed", G_CALLBACK(on_notes_changed), stuff);

	gtk_window_set_icon_name((GtkWindow*)window, "gtk-yes");

	stuff->calendar = GTK_CALENDAR (gtk_calendar_new ());
	gtk_calendar_set_detail_func(stuff->calendar, &details, NULL, NULL);
	gtk_calendar_set_detail_width_chars(stuff->calendar, 3);
	gtk_calendar_set_detail_height_rows(stuff->calendar, 1);
	on_calendar1_day_selected(stuff->calendar, stuff);
	loadflag = 0;
	g_signal_connect (stuff->calendar, "day-selected", G_CALLBACK(on_calendar1_day_selected), stuff);
	gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (stuff->calendar), 0, 1, 1, 9);

	tmp = gtk_button_new_with_label ("Today");
	gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (tmp), 0, 0, 1, 1);
	g_signal_connect (tmp, "clicked", (GCallback)on_today_clicked, stuff);

	tmp = gtk_scrolled_window_new (NULL, NULL);
	g_value_init (&val, G_TYPE_INT);
	g_value_set_int (&val, 240);
	g_object_set_property (G_OBJECT (tmp), "width-request", &val);
	gtk_container_add (GTK_CONTAINER (tmp), GTK_WIDGET (stuff->notes));
	gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (tmp), 1, 0, 1, 10);

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
