// Copyright (c) 2015 Matthew Donald
// All rights reserved
//
// This example demostrates how to run the Mongoose web server
// in background as a daemon
// $Date: 2014-09-09 22:20:23 UTC $

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "mongoose.h"

static int running;

static void do_req(struct mg_connection *conn) {

    // Prevent proxies and browsers from caching response, and search engines from indexing page
//  mg_send_header(conn, "Cache-Control", "max-age=0, post-check=0, pre-check=0, no-store, no-cache, must-revalidate");
//  mg_send_header(conn, "Pragma", "no-cache");
    mg_send_header(conn, "Content-Type", "text/html");
    mg_send_header(conn, "X-Robots-Tag", "noindex");

    // Return the contents of the request
    mg_printf_data(conn, "<html>\n<head>\n"
                         "<title>Hello World</title>\n"
                         "</head>\n"
                         "<body>\n"
                         "<table border='0'>\n" );
    mg_printf_data(conn, "<tr><td>IP address</td><td>%s</td></tr>",	conn->remote_ip);
    mg_printf_data(conn, "<tr><td>Method</td><td>%s</td></tr>", 	conn->request_method);
    mg_printf_data(conn, "<tr><td>URI</td><td>%s</td></tr>", 		conn->uri);
    mg_printf_data(conn, "<tr><td>Query</td><td>%s</td></tr>", 		conn->query_string);

    // print the headers
    mg_printf_data(conn, "<tr><td colspan='2' align='center'>H E A D E R S</td></tr>", conn->query_string);
    int i;
    for (i = 0; i < conn->num_headers; i++) {
      mg_printf_data(conn, "<tr><td>%s",conn->http_headers[i].name);
      mg_printf_data(conn, ":</td><td>%s</td></tr>", conn->http_headers[i].value);
    }

    // print the content
    if (conn->content != NULL) {
      mg_printf_data(conn, "<tr><td colspan='2' align='center'>C O N T E N T</td></tr>", conn->query_string);
      mg_printf_data(conn, "<tr><td>Length</td><td>%d</td></tr>", 	conn->content_len);
      mg_printf_data(conn, "<tr><td>Content</td><td>%s</td></tr>", 	conn->content);
    }
    mg_printf_data(conn, "</table>\n</body>\n</html>");
}

static int event_handler(struct mg_connection *conn, enum mg_event event) {
  switch (event) {
    case MG_AUTH: return MG_TRUE;
    case MG_REQUEST:
      do_req(conn);
      return MG_TRUE;
    default: return MG_FALSE;
  }
}

static void redir2null(FILE* oldfile, int oflags) {
    int newfd;
    int oldfd = fileno(oldfile);

    // first flush any pending output
    fflush(oldfile);

    // open /dev/null
    newfd = open("/dev/null", oflags);

    // redirect fd to the new file (/dev/null)
    dup2(newfd, oldfd);

    // close the now redundent new file
    close(newfd);
}

static void daemonMode(void) {
    pid_t process_id = 0;
    pid_t sid = 0;

    // 1. create a child process
    process_id = fork();
    if (process_id < 0) {
      syslog(LOG_ERR, "fork() failed! - exiting");
      exit(-1);
    }

    // 2. kill the parent process
    if (process_id > 0)
      exit(0);

    // 3. unmask the file mode
    umask(0);

    // 4. start a new session
    sid = setsid();
    if (sid < 0) {
      syslog(LOG_ERR, "setsid() failed! - exiting");
      exit(-1);
    }

    // 5. redirect stdin, stdout and stderr to null
    redir2null(stdin, O_RDONLY);
    redir2null(stdout, O_WRONLY);
    redir2null(stderr, O_RDWR);       // Note stderr must be r/w
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void intHandler(int dummy) {
    running = 0;
}

int main(void) {
    char* user = "nobody";
    time_t start, finish;

    // Open the log
    openlog(NULL, LOG_CONS | LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "Daemon starting");

    // Get the start time
    time(&start);

    // Demonize the server
    daemonMode();

    // Trap KILL's - cause 'running' flag to be set false
    running = 1;
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);

    // Create and configure the server
    struct mg_server* server = mg_create_server(NULL, event_handler);
    mg_set_option(server, "listening_port", "8000");
    syslog(LOG_INFO, "Switching server to run as user %s", user);
    mg_set_option(server, "run_as_user", user);

    // Serve request. Issue SIGTERM to terminate the program
    syslog(LOG_INFO, "Listening on port %s\n", mg_get_option(server, "listening_port"));
    while (running) {
      mg_poll_server(server, 1000);
    }

    // Compute to elapsed time
    time(&finish);
    double dur = difftime(finish, start);
    long hours, mins, secs, t;
    hours = dur/3600.00;
    t  = fmod(dur,3600.00);
    mins = t / 60;
    secs = t % 60;

    // Cleanup, and free server instance
    mg_destroy_server(&server);
    syslog(LOG_NOTICE, "Daemon stopping after %lu:%02lu:%02lu", hours, mins, secs);

    return 0;
}
