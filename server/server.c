#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_TCP_PORT 8005
#define BUFLEN 80
#define TRUE 1

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *client_function(void*);

typedef struct thread_param {
  int socket;
  char *ip;
} thread_param;

int main(int argc, char **argv) {
  int                sd, new_sd, client_len, port;
  //int                bytes_to_read;
  //char               *bp, buf[BUFLEN], lbuf[BUFLEN];
  struct sockaddr_in server, client;
  FILE               *fp;

  switch (argc) {
    case 1:
      port = SERVER_TCP_PORT;
    break;
    case 2:
      port = atoi(argv[1]);
    break;
    default:
      fprintf(stderr, "[command] [port]\n");
      exit(1);
    break;
  }

  if ((fp = fopen("server_results", "w")) == 0) {
    fprintf(stderr, "server fopen\n");
    exit(1);
  }
  fclose(fp);

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Failed to create socket");
  }

  bzero((char *)&server, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
    perror("Failed to bind socket");
    exit(1);
  }

  listen(sd, 5);

  while (TRUE) {
    client_len = sizeof(client);
    if ((new_sd = accept(sd, (struct sockaddr *)&client, &client_len)) == -1) {
      fprintf(stderr, "Can't accept client\n");
      exit(1);
    }
    pthread_t client_thread;
    struct thread_param thp;
    thp.socket = new_sd;
    thp.ip = inet_ntoa(client.sin_addr);
    pthread_create(&client_thread, NULL, client_function, &thp);
    pthread_join(client_thread, NULL);
  }
  return 0;
}

void *client_function(void *thp) {
  char *bp, buf[BUFLEN], lbuf[BUFLEN];
  int  bytes_to_read;
  FILE *fp;
  if ((fp = fopen("server_results", "a")) == 0) {
    fprintf(stderr, "server fopen\n");
    exit(1);
  }

  struct thread_param *local_thp = thp;

  bp = buf;
  bytes_to_read = BUFLEN;
  recv(local_thp->socket, bp, bytes_to_read, MSG_WAITALL);
  send(local_thp->socket, buf, BUFLEN, 0);

  sprintf(lbuf, "%s %u %ld", local_thp->ip, pthread_self(), sizeof(buf));
  printf("%s\n", lbuf);
  fprintf(fp, "%s\n", lbuf);

  close(local_thp->socket);
  fclose(fp);
}
