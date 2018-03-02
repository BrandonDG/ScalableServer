/*-----------------------------------------------------------------------
--  SOURCE FILE: poll_server.c
--
--  PROGRAM:     poll_svr.exe
--
--  DATE:     February 25, 2018
--
--  DESIGNERS:   Brandon Gillespie & Justen DePourcq
--
--  PROGRAMMERS: Justen DePourcq
--
--  NOTES:
--  Poll Server for Assignment 2 in COMP8005 BCIT Btech Network
--  Security and Administration
-----------------------------------------------------------------------*/
#include <limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/poll.h>
#include <errno.h>

#define SERVER_TCP_PORT 7000
#define LISTENQ         2000
#define BUFLEN          80
#define MAXFD           2000
#define TRUE            1

/*-----------------------------------------------------------------------
--  FUNCTION:    SystemFatal
--
--  DATE:       February 25, 2018
--
--  INTERFACE:  static void SystemFatal(const char* message)
--
--  RETURNS:    void
--
--  NOTES:
--  Displays error message in perror and exits the application.
-----------------------------------------------------------------------*/
static void SystemFatal(const char* message) {
    perror(message);
    exit(EXIT_FAILURE);
}

/*-----------------------------------------------------------------------
--  FUNCTION:    main
--
--  DATE:       February 25, 2018
--
--  DESIGNER:   Brandon Gillespie & Justen DePourcq
--
--  PROGRAMMER:  Justen DePourcq
--
--  INTERFACE:  int main (int argc, char **argv)
--
--  RETURNS:    int
--
--  NOTES:
--  main.
-----------------------------------------------------------------------*/
int main(int argc, char **argv) {
    int i, max, server_fd, new_sd, sockfd;
    int n, arg, port;
    char buf[BUFLEN];
    socklen_t client_len;
    struct pollfd client[MAXFD];
    struct sockaddr_in client_addr, server;
    FILE *fp;

    switch(argc) {
      case 1:
        port = SERVER_TCP_PORT;
        break;
      case 2:
        port = atoi(argv[1]);
        break;
      default:
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        exit(1);
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      SystemFatal("Cannot Create Socket!");
    }

    arg = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1) {
      SystemFatal("setsockopt");
    }

    if ((fp = fopen("poll_server_results", "w")) == 0) {
      fprintf(stderr, "server fopen\n");
      exit(1);
    }

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
      SystemFatal("bind error");
    }

    listen(server_fd, LISTENQ);

    client[0].fd = server_fd;
    client[0].events = POLLRDNORM;
    for (i = 1; i < MAXFD; i++) {
      client[i].fd = -1;
      max = 0;
    }

    while(TRUE) {
      n = poll(client, max + 1, -1);
      if (client[0].revents & POLLRDNORM) {
        client_len = sizeof(client_addr);
        if ((new_sd = accept(server_fd, (struct sockaddr *) &client_addr, &client_len)) == -1) {
           SystemFatal("accept error");
        }

        for (i = 1; i < MAXFD; i++)
          if (client[i].fd < 0) {
            client[i].fd = new_sd;
            break;
          }
          if (i == MAXFD) {
            SystemFatal("too many clients");
          }
          client[i].events = POLLRDNORM;

          if (i > max) {
            max = i;
          }

          if (--n <= 0) {
            continue;
          }
        }

        for (i = 1; i <= max; i++)  {
          if ((sockfd = client[i].fd) < 0) {
            continue;
          }

          if (client[i].revents & (POLLRDNORM | POLLERR)) {
            size_t data_sent = 0;
            if ((n = read(sockfd, buf, BUFLEN)) < 0) {
              if (errno == ECONNRESET) {
                close(sockfd);
                client[i].fd = -1;
              } else
                SystemFatal("read error");
                } else if (n == 0) {
                  close(sockfd);
                  client[i].fd = -1;
                } else {
                  write(sockfd, buf, n);
                  data_sent += BUFLEN;
                  char lbuf[BUFLEN];
                  sprintf(lbuf, "Socket: %u   Amount of Data: %ld", client[i].fd, data_sent);
                  printf("%s\n", lbuf);
                  fprintf(fp, "%s\n", lbuf);
                }

                if (--n <= 0) {
                  break;
                }

            }
        }
    }
    return 0;
}
