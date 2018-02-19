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

int main(int argc, char **argv) {
  int    sd, new_sd, client_len, port;
  int    bytes_to_read;
  char   *bp, buf[BUFLEN];
  struct sockaddr_in server, client;

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

    printf(" Remote Address: %s\n", inet_ntoa(client.sin_addr));
    bp = buf;
    bytes_to_read = BUFLEN;
    recv(new_sd, bp, bytes_to_read, MSG_WAITALL);
    printf("Sending: %s\n", buf);

    send(new_sd, buf, BUFLEN, 0);
    close(new_sd);
  }
}
