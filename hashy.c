#include <stdlib.h>
#include <string.h>

/* Needed for TokyoCabinet*/
/*
#include <stdbool.h>
#include <stdint.h>
*/

/* Events */
#include <event.h>
#include <evhttp.h>

/* Tokyo Cabinet */
#include <tcutil.h>
#include <tcbdb.h>

void put_request(struct evhttp_request* request, TCBDB* database) {
	char buffer[256];

	evbuffer_remove(request->input_buffer, buffer, 255);
	tcbdbput2(database, request->uri, buffer);
	tcbdbsync(database);

	evhttp_send_reply(request, HTTP_OK, "OK", NULL);
}

void get_request(struct evhttp_request* request, TCBDB* database) {
	char* buffer;
	size_t buffer_len;
	struct evbuffer* out;

	buffer = tcbdbget2(database, request->uri);
	out = evbuffer_new();

	if (buffer != NULL) {
		buffer_len = strlen(buffer);
		evbuffer_add(out, buffer, buffer_len);
	}

	evhttp_add_header(request->output_headers, "Content-Type", "text/plain");
	evhttp_send_reply(request, HTTP_OK, "OK", out);

	evbuffer_free(out);
	free(buffer);
}

void get_dir_request(struct evhttp_request* request, TCBDB* database) {
	int i;
	char* key;
	size_t key_length;
	TCLIST* key_list;
	struct evbuffer* result;

	result = evbuffer_new();
	key_list = tcbdbfwmkeys2(database, request->uri, -1);
	for (i=0; (key = (char*)tclistval2(key_list, i)) != NULL; i++) {
		key_length = strlen(key);
		evbuffer_add(result, key, key_length+1);
		evbuffer_add(result, "\n", 1);
	}
	evhttp_send_reply(request, HTTP_OK, "OK", result);
	tclistdel(key_list);
}

void handle_request(struct evhttp_request* request, void* arg) {
	TCBDB* database = (TCBDB*) arg;
	switch (request->type) {
		case EVHTTP_REQ_GET:
			if (request->uri[strlen(request->uri)-1] == '/') {
				get_dir_request(request, database);
			} else {
				get_request(request, database);
			}
			break;
		case EVHTTP_REQ_PUT:
			put_request(request, database);
			break;
		default:
			evhttp_send_error(request, 405, "Method Not Allowed");
			break;
	}
}

int main() {
	struct event_base* ev_base;
	struct evhttp* server;
	TCBDB* database;

	/* Open the DB */
	database = tcbdbnew();
	if (!tcbdbopen(database, "test.db", BDBOWRITER | BDBOCREAT)) {
		fputs("Couldn't Open Database\n", stderr);
		tcbdbdel(database);
		return 1;
	}

	ev_base = event_base_new();
	server = evhttp_new(ev_base);

	evhttp_bind_socket(server, "0.0.0.0", 8765);
	evhttp_set_gencb(server, handle_request, (void*)database);

	event_base_dispatch(ev_base);

	event_base_free(ev_base);
	tcbdbdel(database);

	return 0;
}
