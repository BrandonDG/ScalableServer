#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_TCP_PORT 8005
#define BUFLEN			    80

int main(int argc, char **argv) {
	int    sd, port, bytes_to_read;
	struct hostent	*hp;
	struct sockaddr_in server;
	char   *host, *bp, rbuf[BUFLEN], sbuf[BUFLEN], **pptr;
	char   str[16];

  switch(argc) {
    case 2:
      host = argv[1];
      port = SERVER_TCP_PORT;
    break;
    case 3:
      host = argv[1];
      port = atoi(argv[2]);
    break;
    default:
      fprintf(stderr, "[command] [ip] [port]");
      exit(1);
    break;
  }

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Failed to create socket");
    exit(1);
  }
  bzero((char *)&server, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  if ((hp = gethostbyname(host)) == NULL) {
    fprintf(stderr, "Unknown server address\n");
    exit(1);
  }
  bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);

  if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
    fprintf(stderr, "Can't connect to server");
    perror("Connect");
    exit(1);
  }
  printf("Connected:   Server Name: %s\n", hp->h_name);
  pptr = hp->h_addr_list;
  printf("\t\tIP Address: %s\n", inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)));
	printf("Transmit:\n");

  fgets(sbuf, BUFLEN, stdin);

  send(sd, sbuf, BUFLEN, 0);

  printf("Receive:\n");
	bp = rbuf;
	bytes_to_read = BUFLEN;

  recv(sd, bp, bytes_to_read, MSG_WAITALL);
  printf("%s\n", rbuf);

  fflush(stdout);
  close(sd);
  return(0);
}
