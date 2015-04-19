// Copyright (c) 2015 Matthew Donald
// All rights reserved
//
// This example demonstrates how to run the Mongoose web server
// configuring the number of server threads automatically to
// match the number of processors + 1
// $Date: 2014-09-09 22:20:23 UTC $

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <pthread.h>

#include "mongoose.h"
#include "pidfile.h"

static pthread_mutex_t signal_mutex;
static volatile int running;

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

static int pidFile(char* prog) {
    char pidFile[255];
    strcpy(pidFile, "/var/run/");
    strcat(pidFile, basename(prog));
    strcat(pidFile, ".pid");
    return createPidFile(prog, pidFile, 0);
}

static struct mg_server** createThreads(int nprocs) {
    struct mg_server** server;
    int i;
    server = (struct mg_server**)malloc(nprocs * sizeof(struct mg_server*));
    for (i = 0; i < nprocs; i++) {
      server[i] = mg_create_server(NULL, event_handler);
    }
    return server;
}

static int getProcessorCnt(void) {
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocs < 1)
    {
      printf("Could not determine number of CPUs online: %s\n", strerror (errno));
      exit(-100);
    }
    return (int)nprocs;
}

static void* serve(void* server) {
    while (running) {
      mg_poll_server((struct mg_server *) server, 1000);
    }
    return NULL;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void intHandler(int signum) {
    if (running) {
      pthread_mutex_lock(&signal_mutex);
      running = 0;
      pthread_mutex_unlock(&signal_mutex);
    }
}

int main(int argc, char* argv[]) {
    int i;

    // Use a pid file to ensure a singleton process
    int pid_fd = pidFile(argv[0]);

    // Trap KILL's - cause 'running' flag to be set false
    running = 1;
    if (pthread_mutex_init(&signal_mutex, NULL) != 0)
    {
        printf("mutex init failed\n");
        exit(-100);
    }
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);

    // Setup server structures - number of processors + 1;
    static struct mg_server** server;
    int nprocs = getProcessorCnt();
    printf("%d CPU%s detected, %d threads will be created\n", nprocs, ((nprocs == 1) ? "" : "'s"), nprocs + 1);
    nprocs++;
    server = createThreads(nprocs);

    // Configure the server
    mg_set_option(server[0], "listening_port", "8000");
    for (i = 1; i < nprocs; i++) {
       mg_copy_listeners(server[0], server[i]);
    }

    // Start the servers
    printf("Listening on port %s\n", mg_get_option(server[0], "listening_port"));
    for (i = 0; i < nprocs; i++) {
      printf("... Starting server %d\n", i+1);
      mg_start_thread(serve, server[i]);
    }

    // Wait loop unil all threads have exited
    while(running)
      sleep(1);

    // Cleanup, and free server instance
    for (i = 0; i < nprocs; i++) {
      mg_destroy_server(&server[i]);
    }
    free(server);
    if (pid_fd >= 0)
      close(pid_fd);

    putchar('\n');
    exit(0);
}
