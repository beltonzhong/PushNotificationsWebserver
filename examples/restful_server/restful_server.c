/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#include "mongoose.h" 
#include "stdio.h"

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;
static int s_exit_flag = 0;
static char * url = "https://gcm-http.googleapis.com/gcm/send";
static char * headers = "Content-Type:application/json\r\nAuthorization:key=AIzaSyDvnf1lRGKzkfgcGbhUkFpiBW9FH4-DOvo\r";
static char data[100];

static void ev_handler_push(struct mg_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;

  switch (ev) {
    case MG_EV_CONNECT:
      if (* (int *) ev_data != 0) {
        fprintf(stderr, "connect() failed: %s\n", strerror(* (int *) ev_data));
        s_exit_flag = 1;
      }
      break;
    case MG_EV_HTTP_REPLY:
      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      fwrite(hm->message.p, 1, hm->message.len, stdout);
      fwrite(hm->body.p, 1, hm->body.len, stdout);
      putchar('\n');
      s_exit_flag = 1;
      break;
    default:
      break;
  }
}

void send_notification(char * item_name, char * token) {
  struct mg_mgr mgr;
  mg_mgr_init(&mgr, NULL);

  char * data1 = "{\"data\": {\"name\":\"";
  char * data2 = "\"},\n\"to\":\"";
  char * data3 = "\"}";
  strcpy(data, data1);
  strcat(data, item_name);
  strcat(data, data2);
  strcat(data, token);
  strcat(data, data3);
  mg_connect_http(&mgr, ev_handler_push, url, headers, data);

  while (s_exit_flag == 0) {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;
  char n1[100], n2[100];
  mg_get_http_var(&hm->body, "n1", n1, sizeof(n1));
  mg_get_http_var(&hm->body, "n2", n2, sizeof(n2));
  switch (ev) {
    case MG_EV_HTTP_REQUEST:
      if (mg_vcmp(&hm->uri, "/push/") == 0) {
        send_notification(n1, n2);                    /* Handle RESTful call */
      } else {
        mg_serve_http(nc, hm, s_http_server_opts);  /* Serve static content */
      }
      break;
    default:
      break;
  }
}

int main(int argc, char *argv[]) {
  struct mg_mgr mgr;
  struct mg_connection *nc;
  int i;
  char *cp;
#ifdef MG_ENABLE_SSL
  const char *ssl_cert = NULL;
#endif

  mg_mgr_init(&mgr, NULL);

  /* Process command line options to customize HTTP server */
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-D") == 0 && i + 1 < argc) {
      mgr.hexdump_file = argv[++i];
    } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
      s_http_server_opts.document_root = argv[++i];
    } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      s_http_port = argv[++i];
    } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
      s_http_server_opts.auth_domain = argv[++i];
#ifdef MG_ENABLE_JAVASCRIPT
    } else if (strcmp(argv[i], "-j") == 0 && i + 1 < argc) {
      const char *init_file = argv[++i];
      mg_enable_javascript(&mgr, v7_create(), init_file);
#endif
    } else if (strcmp(argv[i], "-P") == 0 && i + 1 < argc) {
      s_http_server_opts.global_auth_file = argv[++i];
    } else if (strcmp(argv[i], "-A") == 0 && i + 1 < argc) {
      s_http_server_opts.per_directory_auth_file = argv[++i];
    } else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
      s_http_server_opts.url_rewrites = argv[++i];
#ifndef MG_DISABLE_CGI
    } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      s_http_server_opts.cgi_interpreter = argv[++i];
#endif
#ifdef MG_ENABLE_SSL
    } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
      ssl_cert = argv[++i];
#endif
    } else {
      fprintf(stderr, "Unknown option: [%s]\n", argv[i]);
      exit(1);
    }
  }

  /* Set HTTP server options */
  nc = mg_bind(&mgr, s_http_port, ev_handler);
  if (nc == NULL) {
    fprintf(stderr, "Error starting server on port %s\n", s_http_port);
    exit(1);
  }

#ifdef MG_ENABLE_SSL
  if (ssl_cert != NULL) {
    const char *err_str = mg_set_ssl(nc, ssl_cert, NULL);
    if (err_str != NULL) {
      fprintf(stderr, "Error loading SSL cert: %s\n", err_str);
      exit(1);
    }
  }
#endif

  mg_set_protocol_http_websocket(nc);
  s_http_server_opts.document_root = ".";
  s_http_server_opts.enable_directory_listing = "yes";

  /* Use current binary directory as document root */
  if (argc > 0 && ((cp = strrchr(argv[0], '/')) != NULL ||
      (cp = strrchr(argv[0], '/')) != NULL)) {
    *cp = '\0';
    s_http_server_opts.document_root = argv[0];
  }

  printf("Starting RESTful server on port %s\n", s_http_port);
  for (;;) {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  return 0;
}
