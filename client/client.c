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
#include <time.h>
#include <sys/time.h>

#define SERVER_TCP_PORT 8005
#define BUFLEN			    80

int main(int argc, char **argv) {
	struct hostent	   *hp;
	struct sockaddr_in server;
  struct timeval     begin, end;
  int                sd, port, bytes_to_read, n;
	char               *host, *bp, **pptr;
	char               str[16], rbuf[BUFLEN], sbuf[BUFLEN], lbuf[BUFLEN];
  FILE               *fp;

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

  if ((fp = fopen("client_results", "a")) == 0) {
    fprintf(stderr, "client fopen\n");
    exit(1);
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
  pptr = hp->h_addr_list;

  sbuf[0] = 'H';
  sbuf[1] = '\0';

  gettimeofday(&begin, NULL);
  send(sd, sbuf, BUFLEN, 0);

	bp = rbuf;
	bytes_to_read = BUFLEN;
  recv(sd, bp, bytes_to_read, MSG_WAITALL);
  gettimeofday(&end, NULL);

  n = end.tv_usec - begin.tv_usec;
  if (n < 0) {
    n *= - 1;
  }
  sprintf(lbuf, "%s %d %ld %lu.%06d", inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)),
            sd, sizeof(sbuf), (end.tv_sec - begin.tv_sec), n);
  printf("%s\n", lbuf);
  fprintf(fp, "%s\n", lbuf);

  fflush(stdout);
  close(sd);
  fclose(fp);

  return(0);
}
