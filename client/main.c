#include <gtk/gtk.h>

GtkTextBuffer *input_text_buffer, *output_text_buffer;
GtkTextView *input_text_view, *output_text_view;

void print_from_text_buffer(GtkTextBuffer *text_buffer) {
    GtkTextIter start_iter, end_iter;
    g_print("print text buffer: %p\n", text_buffer);
    gtk_text_buffer_get_bounds(text_buffer, &start_iter, &end_iter);
    g_print("%s\n", gtk_text_buffer_get_text(text_buffer, &start_iter, &end_iter, FALSE));
}

gchar *get_from_text_buffer(GtkTextBuffer *text_buffer) {
    GtkTextIter start_iter, end_iter;
    gtk_text_buffer_get_bounds(text_buffer, &start_iter, &end_iter);
    return gtk_text_buffer_get_text(text_buffer, &start_iter, &end_iter, FALSE);
}

void write_to_text_buffer(GtkTextBuffer *text_buffer, gchar *string) {
    GtkTextIter last_iter;
    gtk_text_buffer_get_end_iter(text_buffer, &last_iter);
    gtk_text_buffer_insert(text_buffer, &last_iter, string, -1);
}

void clean_text_buffer(GtkTextBuffer *text_buffer) {
    GtkTextIter start_iter, end_iter;
    gtk_text_buffer_get_bounds(text_buffer, &start_iter, &end_iter);
    gtk_text_buffer_delete(text_buffer, &start_iter, &end_iter);
}

gchar *create_message_prefix() {
    time_t t = time(NULL); // get timestamp maybe other
    GDateTime *datetime = g_date_time_new_from_unix_local(t);
    gchar *s = g_date_time_format(datetime, "%Y-%m-%d %H:%M:%S");
    gchar *prefix = g_strconcat(s, "\r\n", NULL);
    g_free(s);
    g_date_time_unref(datetime);
    return prefix;
}

gboolean press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    switch (event->keyval) {
        case GDK_KEY_Return:
            if (event->state & GDK_CONTROL_MASK) {
                gchar *buf_string = get_from_text_buffer(input_text_buffer);
                gchar *prefix = create_message_prefix();
                gchar *string = g_strconcat(prefix, buf_string, "\r\n", NULL);
                write_to_text_buffer(output_text_buffer, string);
                g_free(buf_string);
                g_free(prefix);
                g_free(string);

                GtkTextMark *end_mark = gtk_text_buffer_get_insert(output_text_buffer);
                gtk_text_view_scroll_mark_onscreen(output_text_view, end_mark);

                clean_text_buffer(input_text_buffer);
                return TRUE;
            }
            break;
        default:
            return FALSE;
    }
    return FALSE;
}


int main(int argc, char **argv) {

    GtkBuilder *builder;
    GtkWidget *window;

    gtk_init(&argc, &argv);
    builder = gtk_builder_new_from_file(CLIENT_GLADE);
    window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
    input_text_buffer = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "input_text_buffer"));
    output_text_buffer = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "output_text_buffer"));
    input_text_view = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "input_text_view"));
    output_text_view = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "output_text_view"));

    gtk_builder_connect_signals(builder, NULL);
    g_object_unref(G_OBJECT(builder));
    g_signal_connect(input_text_view, "key_press_event", G_CALLBACK(press_event), NULL);
    gtk_widget_show_all(window);
    gtk_main();


    return 0;
}