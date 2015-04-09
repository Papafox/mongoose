#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "mongoose.h"

static int running;		// exit flag for poll loop

static void do_checkip_call(struct mg_connection *conn) {
    // Prevent proxies and browsers from caching response, also prevent indexing by robols/search engines
    mg_send_header(conn, "Cache-Control", "max-age=0, post-check=0, pre-check=0, no-store, no-cache, must-revalidate");
    mg_send_header(conn, "Pragma", "no-cache");
    mg_send_header(conn, "Content-Type", "text/html");
    mg_send_header(conn, "X-Robots-Tag", "noindex");

    // Return the remote ipaddr
    mg_printf_data(conn, "<html><head><meta name=\"robots\" content=\"noindex\"/><title>Current IP Check</title></head><body>Current IP Address: %s</body></html>", conn->remote_ip);
}

static int event_handler(struct mg_connection *conn, enum mg_event event) {
  switch (event) {
    case MG_AUTH:
      return MG_TRUE;
    case MG_REQUEST:
      if (strcmp(conn->uri, "/") == 0) {
        do_checkip_call(conn);
        return MG_TRUE;
      }
      return MG_FALSE;
    default: return MG_FALSE;
  }
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void intHandler(int dummy) {
    running = 0;
}

int main(void) {
    struct mg_server *server;

    // Create and configure the server
    server = mg_create_server(NULL, event_handler);
    mg_set_option(server, "listening_port", "8000");

    // Trap KILL's - cause 'running' flag to be set false
    running = 1;
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);

    // Serve request. Hit Ctrl-C to terminate the program
    printf("Checkip started\n");
    printf("Listening on port %s\n", mg_get_option(server, "listening_port"));
    while (running) {
      mg_poll_server(server, 1000);
    }

    // Cleanup, and free server instance
    mg_destroy_server(&server);
    printf("Checkip shutdown\n");

    return 0;
}
