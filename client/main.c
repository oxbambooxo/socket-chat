#include <gtk/gtk.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <strings.h>
#include <errno.h>
#include <glib/gprintf.h>
#include <json-glib/json-glib.h>

GtkWindow *main_window, *init_window;
GtkTextBuffer *input_text_buffer, *output_text_buffer;
GtkTextView *input_text_view, *output_text_view;
GtkEntryBuffer *ip_entry_buffer;
GtkWidget *message_dialog = NULL;

gint sock_fd = 0;

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

void write_to_text_buffer(GtkTextBuffer *text_buffer, const gchar *string) {
    GtkTextIter last_iter;
    gtk_text_buffer_get_end_iter(text_buffer, &last_iter);
    gtk_text_buffer_insert(text_buffer, &last_iter, string, -1);
}

void clean_text_buffer(GtkTextBuffer *text_buffer) {
    GtkTextIter start_iter, end_iter;
    gtk_text_buffer_get_bounds(text_buffer, &start_iter, &end_iter);
    gtk_text_buffer_delete(text_buffer, &start_iter, &end_iter);
}

gboolean output_content(gpointer data) {
    write_to_text_buffer(output_text_buffer, data);
    gtk_text_view_scroll_mark_onscreen(output_text_view, gtk_text_buffer_get_insert(output_text_buffer));
//    gtk_text_view_scroll_to_mark(output_text_view, gtk_text_buffer_get_insert(output_text_buffer),0.0, TRUE, 0.0, 1.0);
    g_free(data);
    return G_SOURCE_REMOVE;
}

gchar *timestamp_to_string(time_t timestamp) {
    GDateTime *datetime = g_date_time_new_from_unix_local(time(&timestamp));
    gchar *string = g_date_time_format(datetime, "%Y-%m-%d %H:%M:%S");
    g_date_time_unref(datetime);
    return string;
}

void show_message(char *format, ...) {
    gchar str[2048];
    va_list args;
    va_start(args, format);
    g_vsprintf(str, format, args);
    va_end(args);

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

void start_chat() {
    gtk_widget_hide(GTK_WIDGET(init_window));
    gtk_widget_show_all(GTK_WIDGET(main_window));
}

void end_chat() {
    gtk_widget_hide(GTK_WIDGET(main_window));
    gtk_widget_show(GTK_WIDGET(init_window));
}

gboolean server_disconnect(gpointer data) {
    g_message("server disconnect %s", strerror(errno));
    end_chat();
    show_message("已断开连接");
    if (sock_fd) {
        close(sock_fd);
        sock_fd = 0;
    }
    return G_SOURCE_REMOVE;
}

gboolean input_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    switch (event->keyval) {
        case GDK_KEY_Return:
            // if (event->state & GDK_CONTROL_MASK)
        {
            gchar *data_text = get_from_text_buffer(input_text_buffer);

            JsonBuilder *builder = json_builder_new();

            json_builder_begin_object(builder);
            json_builder_set_member_name(builder, "type");
            json_builder_add_int_value(builder, 1);
            json_builder_set_member_name(builder, "message");
            json_builder_add_string_value(builder, data_text);
            json_builder_end_object(builder);

            JsonNode *root = json_builder_get_root(builder);
            JsonGenerator *gen = json_generator_new();
            json_generator_set_root(gen, root);
            json_node_free(root);

            gsize str_length;
            gchar *str = json_generator_to_data(gen, &str_length);

            g_object_unref(gen);
            g_object_unref(builder);

            gchar *data = g_malloc(str_length + 4);
            guint32 pack_data_length = g_htonl(str_length);
            memcpy(data, &pack_data_length, 4);
            memcpy(data + 4, str, str_length);

            // FIXME 发送失败检测
            if (send(sock_fd, (void *) data, str_length + 4, 0) <= 0) {
                g_message("send data failed %s", strerror(errno));
                g_idle_add(server_disconnect, NULL);
            }

            g_free(str);
            g_free(data_text);
            g_free(data);
            clean_text_buffer(input_text_buffer);
            return 1;
        }
        default:
            return FALSE;
    }
    return FALSE;
}

#define BUFSIZE 2048

gpointer handle_connect(gpointer none) {
    char buffer[BUFSIZE];
    GString *data = g_string_new(NULL);
    ssize_t len = 0;
    while (1) {
        while (data->len < 4) {
            len = recv(sock_fd, buffer, BUFSIZE, 0);
            if (len <= 0) {
                g_idle_add(server_disconnect, NULL);
                g_string_free(data, 1);
                return NULL;
            }
            g_string_append_len(data, buffer, len);
        }
        guint32 i;
        memcpy(&i, data->str, 4); // 数据前4字节为长度
        guint32 length = g_ntohl(i); // 数据长度

        while (data->len < (length + 4)) {
            len = recv(sock_fd, buffer, BUFSIZE, 0);
            if (len <= 0) {
                g_message("server disconnect %s", strerror(errno));
                g_idle_add(server_disconnect, NULL);
                g_string_free(data, 1);
                return NULL;
            }
            g_string_append_len(data, buffer, len);
        }

        GError *error = NULL;
        JsonParser *parser = json_parser_new();
        if (!json_parser_load_from_data(parser, data->str + 4, length, &error)) {
            g_message("Unable parse: %s", error->message);
            g_error_free(error);
        } else {
            JsonReader *reader = json_reader_new(json_parser_get_root(parser));
            /*  < gchar ** > usging demo
            int a = json_reader_count_elements(reader);
            g_print("-- reader has %d member\n", a);
            gchar **b = json_reader_list_members(reader);
            for (gchar **c = b; *c; c++) {
                g_print("-- reader.%s\n", *c);
            }
            g_strfreev(b);
             */

            json_reader_read_member(reader, "type");
            gint64 type = json_reader_get_int_value(reader);
            json_reader_end_member(reader);
            json_reader_read_member(reader, "message");
            const gchar *message = json_reader_get_string_value(reader);
            json_reader_end_member(reader);
            json_reader_read_member(reader, "timestamp");
            gint64 timestamp = json_reader_get_int_value(reader);
            json_reader_end_member(reader);

            if (type == 1 && message && timestamp) {
                gchar *prefix = timestamp_to_string(timestamp);
                gchar *content = g_strconcat(prefix, "\r\n", message, "\r\n", NULL);
                g_idle_add(output_content, content);
                g_free(prefix);
            }
            g_object_unref(reader);
        }
        g_object_unref(parser);
        g_string_erase(data, 0, length + 4);
    }
}

void create_connect(GtkWidget *widget, gpointer *data) {
    const gchar *ip_input = gtk_entry_buffer_get_text(ip_entry_buffer);
    gchar **ip_input_strv = g_strsplit(ip_input, ":", 2);
    gchar *server_name, *server_port;
    if (ip_input_strv[0]) {
        server_name = ip_input_strv[0];
    } else {
        server_name = "127.0.0.1";
    }
    if (g_strv_length(ip_input_strv) == 2 && ip_input_strv[1]) {
        server_port = ip_input_strv[1];
    } else {
        server_port = "56789";
    }

    struct addrinfo hints, *result, *aip;
    int err = 0;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    if ((err = getaddrinfo(server_name, server_port, &hints, &result)) != 0) {
        show_message("查找主机出错: %s", gai_strerror(err));
        g_strfreev(ip_input_strv);
        return;
    };
    g_strfreev(ip_input_strv);

    for (aip = result; aip != NULL; aip = result->ai_next) {
        struct sockaddr_in *aip_addr_in = (struct sockaddr_in *) aip->ai_addr;
        char addr[20];
        inet_ntop(aip->ai_family, (void *) &(aip_addr_in->sin_addr), addr, aip_addr_in->sin_len);
//        char addr = inet_ntoa(aip_addr_in->sin_addr);
        g_message("server addr: %s, port %d", addr, ntohs(aip_addr_in->sin_port));
        if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            show_message("创建socket出错: %s", strerror(errno));
        } else {
            if (connect(sock_fd, aip->ai_addr, aip->ai_addrlen) != 0) {
                g_message("connect failed");
                show_message("网络连接失败: %s", strerror(errno));
                close(sock_fd);
                sock_fd = 0;
            } else {
                g_message("connect succed");
                start_chat();
                GThread *handle = g_thread_new("handle_connect", handle_connect, NULL);
                g_thread_unref(handle);
                break; /* Success */
            }
        }
    }
    if (aip == NULL) {               /* No address succeeded */
        show_message("连接失败");
        return;
    }
}

gboolean check_ip_entry_enter(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    if (event->keyval == GDK_KEY_Return) {
        create_connect(NULL, NULL);
        return 1;
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
    g_signal_connect(init_button, "clicked", G_CALLBACK(create_connect), NULL);
    g_signal_connect_swapped(ip_entry, "key_press_event", G_CALLBACK(check_ip_entry_enter), NULL);
    g_signal_connect(init_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(GTK_WIDGET(init_window));
    gtk_main();


    return 0;
}