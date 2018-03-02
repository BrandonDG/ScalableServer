/*-----------------------------------------------------------------------
--	SOURCE FILE: multhithreaded_server.c
--
--	PROGRAM:     multi_svr.exe
--
--	DATE:		 February 25, 2018
--
--	DESIGNERS:	 Brandon Gillespie & Justen DePourcq
--
--	PROGRAMMERS: Brandon Gillespie
--
--	NOTES:
--	Multithreaded Server for Assignment 2 in COMP8005 BCIT Btech Network
--  Security and Administration
-----------------------------------------------------------------------*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <omp.h>
#include <errno.h>

#define SERVER_TCP_PORT 7000
#define BUFLEN          80
#define TRUE            1

/*-----------------------------------------------------------------------
--	FUNCTION:	  main
--
--	DATE:       February 25, 2018
--
--	DESIGNER:   Brandon Gillespie & Justen DePourcq
--
--  PROGRAMMER:	Brandon Gillespie
--
--	INTERFACE:	int main (int argc, char **argv)
--
--	RETURNS:    int
--
--	NOTES:
--	main.
-----------------------------------------------------------------------*/
int main(int argc, char **argv) {
  int                sd, new_sd, client_len, port;
  int                bytes_to_read, thread_num, e;
  char               *bp, buf[BUFLEN], lbuf[BUFLEN];
  struct sockaddr_in server, client;
  FILE               *fp;
  size_t             data_sent;

  thread_num = 0;

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

  listen(sd, 4096);
  omp_set_num_threads(500);
  #pragma omp parallel private(thread_num, new_sd, fp, lbuf, client, data_sent)
  {
    while (TRUE) {
      data_sent = 0;
      client_len = sizeof(client);
      if ((new_sd = accept(sd, (struct sockaddr *)&client, &client_len)) == -1) {
        fprintf(stderr, "Can't accept client\n");
        exit(1);
      }

      if ((fp = fopen("server_results", "a")) == 0) {
        fprintf(stderr, "server fopen\n");
        exit(1);
      }
      thread_num = omp_get_thread_num() + 1;
      bp = buf;
      bytes_to_read = BUFLEN;
      while ((e = recv(new_sd, bp, bytes_to_read, MSG_WAITALL)) > 0) {
        send(new_sd, buf, BUFLEN, 0);
        sprintf(lbuf, "%s %u %ld", inet_ntoa(client.sin_addr), thread_num, sizeof(buf));
        printf("%s\n", lbuf);
        data_sent += sizeof(lbuf);
      }
      if (e == -1) {
        fprintf(fp, "Error: %s\n", strerror(errno));
        perror("Connection Error");
      } else {
        sprintf(lbuf, "%s %u %ld", inet_ntoa(client.sin_addr), thread_num, data_sent);
        fprintf(fp, "%s\n", lbuf);
      }

      close(new_sd);
      fclose(fp);
    }
  }
  return 0;
}
