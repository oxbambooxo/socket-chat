#include <gtk/gtk.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <strings.h>

GtkWindow *main_window, *init_window;
GtkTextBuffer *input_text_buffer, *output_text_buffer;
GtkTextView *input_text_view, *output_text_view;
GtkEntryBuffer *ip_entry_buffer;
GtkWidget *message_dialog = NULL;

#define BUFSIZE 4096
gchar sock_buffer[BUFSIZE];
gint sock_fd = 0;
gchar NET_SEND = '\x01';

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

gboolean input_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    switch (event->keyval) {
        case GDK_KEY_Return:
            // if (event->state & GDK_CONTROL_MASK)
        {
            gchar *data_text = get_from_text_buffer(input_text_buffer);
            size_t data_length = strlen(data_text);
            gchar *data = g_malloc(data_length + 5);
            data[0] = 1;
            guint32 pack_data_length = g_htonl(data_length);
            memcpy(data + 1, &pack_data_length, 4);
            memcpy(data + 5, data_text, data_length);
            if (sock_fd) {
                // FIXME 发送失败检测
                send(sock_fd, (void *) data, data_length + 5, 0);
            } else {
                gtk_widget_show(GTK_WIDGET(init_window));
            }
            g_free(data_text);
            g_free(data);
            gtk_text_view_scroll_mark_onscreen(output_text_view, gtk_text_buffer_get_insert(output_text_buffer));
            clean_text_buffer(input_text_buffer);
            return TRUE;
        }
        default:
            return FALSE;
    }
    return FALSE;
}

void show_message(gchar *str) {
    if (G_IS_OBJECT(message_dialog)) {
        gtk_widget_destroy(message_dialog);
    }
    message_dialog = gtk_message_dialog_new(init_window,
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_NONE,
                                            NULL);
    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(message_dialog), str);
    gtk_window_set_transient_for(GTK_WINDOW(message_dialog), init_window);
    gtk_widget_set_size_request(message_dialog, 230, 80);
    gtk_window_set_resizable(GTK_WINDOW(message_dialog), FALSE);
    gtk_widget_show_all(message_dialog);
    g_signal_connect(message_dialog, "destroy", G_CALLBACK(gtk_widget_destroy), NULL);
}

void init_connect(GtkWidget *widget, gpointer *data) {
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        show_message("创建socket出错");
        return;
    }
    struct hostent *server = gethostbyname(gtk_entry_buffer_get_text(ip_entry_buffer));
    if (server == NULL) {
        show_message("不正确的服务器地址");
        return;
    }
    struct sockaddr_in dest;
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    bcopy(server->h_addr, &dest.sin_addr.s_addr, (size_t) server->h_length);
    dest.sin_port = htons(56789);
    if (connect(sock_fd, (struct sockaddr *) &dest, sizeof(dest)) != 0) {
        show_message("网络连接失败");
        return;
    } else {
        gtk_widget_hide(GTK_WIDGET(init_window));
        gtk_widget_show_all(GTK_WIDGET(main_window));
    }
}

gboolean check_ip_entry_enter(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    if (event->keyval == GDK_KEY_Return) {
        init_connect(NULL, NULL);
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char **argv) {

    GtkBuilder *builder;
    GtkButton *init_button;
    GtkEntry *ip_entry;

    gtk_init(&argc, &argv);
    builder = gtk_builder_new_from_file(CLIENT_GLADE);
    main_window = GTK_WINDOW(gtk_builder_get_object(builder, "main_window"));
    init_window = GTK_WINDOW(gtk_builder_get_object(builder, "init_window"));
    init_button = GTK_BUTTON(gtk_builder_get_object(builder, "init_button"));
    ip_entry = GTK_ENTRY(gtk_builder_get_object(builder, "ip_entry"));
    ip_entry_buffer = GTK_ENTRY_BUFFER(gtk_builder_get_object(builder, "ip_entry_buffer"));
    input_text_buffer = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "input_text_buffer"));
    output_text_buffer = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "output_text_buffer"));
    input_text_view = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "input_text_view"));
    output_text_view = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "output_text_view"));

    gtk_builder_connect_signals(builder, NULL);
    g_object_unref(G_OBJECT(builder));
    g_signal_connect(input_text_view, "key_press_event", G_CALLBACK(input_press_event), NULL);
    g_signal_connect(init_button, "clicked", G_CALLBACK(init_connect), NULL);
    g_signal_connect_swapped(ip_entry, "key_press_event", G_CALLBACK(check_ip_entry_enter), init_button);
    g_signal_connect(init_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(GTK_WIDGET(init_window));
    gtk_main();


    return 0;
}