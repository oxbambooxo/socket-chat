#include <stdlib.h>
#include <uv.h>
#include <jansson.h>


typedef struct client {
    uv_tcp_t *conn;
    unsigned int data_len; // package length field's value
    char *buf;
    unsigned int buf_len; // client receive real data length;
} client_t;

void client_init(uv_tcp_t *conn) {
    client_t *c = (client_t *) malloc(sizeof(client_t));
    c->conn = conn;
    c->buf = NULL;
    c->buf_len = 0;
    c->data_len = 0;
    conn->data = c;
}

void client_free(uv_tcp_t *conn) {
    client_t *c = (client_t *) conn->data;
    if (c->buf) {
        free(c->buf);
    }
    free(c);
    uv_close((uv_handle_t *) conn, NULL);
    free(conn);
}

void client_receive_data(client_t *c, char *buf, ssize_t len) {
    if (c->buf_len == 0) {

    }
}

void client_decode_data(client_t *c) {
    json_error_t error;
    json_t *root = json_loads(c->buf, JSON_DECODE_ANY, &error);
    if (root) {
        fprintf(stdout, "%s\n", json_dumps(root, JSON_INDENT(2)));

        json_decref(root);
    }
    else {
        fprintf(stderr, "json decode error: %s\n", error.text);
    }
}

void client_handle_data(client_t *c, int type, char *data) {
    switch (type) {
        case 1:;
        default:;
    }
}


void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    *buf = uv_buf_init((char *) malloc(suggested_size), (unsigned int) suggested_size);
}

void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    if (nread >= 0) {
//        write_data((uv_stream_t *) &file_pipe, nread, *buf, on_file_write);
        printf("len:%d\n", (int) nread);
        client_receive_data((client_t *) stream->data, buf->base, nread);
    } else {
        if (nread != UV_EOF) {
            fprintf(stderr, "Error at reading: %s.\n", uv_strerror((int) nread));
        }
        client_free((uv_tcp_t *) stream);
    }

    if (buf->base)
        free(buf->base);
}

void on_connection(uv_stream_t *server, int status) {
    if (status < 0) {
        fprintf(stderr, "Error on listening: %s.\n", uv_strerror(status));
    }

    uv_tcp_t *conn = (uv_tcp_t *) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(server->loop, conn);

    int result = uv_accept(server, (uv_stream_t *) conn);
    if (result == 0) { // success
        client_init(conn);
        uv_read_start((uv_stream_t *) conn, alloc_buffer, on_read);
    } else { // error
        uv_close((uv_handle_t *) conn, NULL);
    }
}

int main(int argc, char *argv[]) {
    int port;
    if (argc > 1) {
        port = atoi(argv[1]);
    } else {
        port = 56789;
    }

    uv_loop_t *loop = uv_default_loop();

    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    struct sockaddr_in bind_addr;
    uv_ip4_addr("0.0.0.0", port, &bind_addr);

    uv_tcp_bind(&server, (const struct sockaddr *) &bind_addr, 0);
    int r = uv_listen((uv_stream_t *) &server, 1024, on_connection);
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return 1;
    }
    return uv_run(loop, UV_RUN_DEFAULT);
}
