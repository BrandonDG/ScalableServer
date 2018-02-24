#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <netdb.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <omp.h>

#define TRUE 		        1
#define FALSE 		      0
#define EPOLL_QUEUE_LEN	256
#define BUFLEN		      80
#define SERVER_PORT	    8005

int fd_server;

static void  system_fatal(const char* message);
       void* clear_socket(void* fd);
static int   clear_socket2(int fd);
       void  close_fd(int);

int main (int argc, char* argv[]) {
	int                       i, arg, threads_amount;
	int                       num_fds, fd_new, epoll_fd;
	static struct epoll_event events[EPOLL_QUEUE_LEN], event;
	int                       port = SERVER_PORT;
	struct sockaddr_in        addr, remote_addr;
	socklen_t                 addr_size = sizeof(struct sockaddr_in);
	struct sigaction          act;

	// set up the signal handler to close the server socket when CTRL-c is received
  act.sa_handler = close_fd;
  act.sa_flags = 0;
  if ((sigemptyset(&act.sa_mask) == -1 || sigaction (SIGINT, &act, NULL) == -1)) {
    perror ("Failed to set SIGINT handler");
    exit (EXIT_FAILURE);
  }

	// Create the listening socket
	fd_server = socket(AF_INET, SOCK_STREAM, 0);
	if (fd_server == -1) {
    system_fatal("socket");
  }

	// set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
	arg = 1;
	if (setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1) {
    system_fatal("setsockopt");
  }

	// Make the server listening socket non-blocking
	if (fcntl(fd_server, F_SETFL, O_NONBLOCK | fcntl (fd_server, F_GETFL, 0)) == -1) {
    system_fatal("fcntl");
  }

	// Bind to the specified listening port
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	if (bind(fd_server, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
    system_fatal("bind");
  }

	// Make threads for applications
	/*
	threads_amount = get_nprocs();
	pthread_t threads[threads_amount];
	for (size_t i = 0; i < threads_amount; i++) {
		pthread_create(&threads[i], NULL, &clear_socket(), );
	} */

	// Listen for fd_news; SOMAXCONN is 128 by default
	if (listen(fd_server, SOMAXCONN) == -1) {
    system_fatal("listen");
  }

	// Create the epoll file descriptor
	epoll_fd = epoll_create(EPOLL_QUEUE_LEN);
	if (epoll_fd == -1) {
		system_fatal("epoll_create");
	}

	// Add the server socket to the epoll event loop
	event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
	event.data.fd = fd_server;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_server, &event) == -1) {
		system_fatal("epoll_ctl");
	}

  // Execute the epoll event loop
	while (TRUE) {
		//struct epoll_event events[MAX_EVENTS];
		num_fds = epoll_wait(epoll_fd, events, EPOLL_QUEUE_LEN, -1);
		if (num_fds < 0) {
			system_fatal("Error in epoll_wait!");
		}

		//omp_set_num_threads(8);
		//#pragma omp parallel private(fd_new)
		//{
			//#pragma omp for
			for (i = 0; i < num_fds; i++) {
	  		// Case 1: Error condition
	  		if (events[i].events & (EPOLLHUP | EPOLLERR)) {
					fputs("epoll: EPOLLERR\n", stderr);
					close(events[i].data.fd);
					continue;
	  		}
	  		assert(events[i].events & EPOLLIN);

	  		// Case 2: Server is receiving a connection request
	  		if (events[i].data.fd == fd_server) {
					//socklen_t addr_size = sizeof(remote_addr);
					fd_new = accept(fd_server, (struct sockaddr*) &remote_addr, &addr_size);
					if (fd_new == -1) {
			    			if (errno != EAGAIN && errno != EWOULDBLOCK) {
									perror("accept");
			    			}
			    			continue;
					}

					// Make the fd_new non-blocking
					if (fcntl(fd_new, F_SETFL, O_NONBLOCK | fcntl(fd_new, F_GETFL, 0)) == -1) {
						system_fatal("fcntl");
					}

					// Add the new socket descriptor to the epoll loop
					event.data.fd = fd_new;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_new, &event) == -1) {
						system_fatal("epoll_ctl");
					}
					printf(" Remote Address:  %s\n", inet_ntoa(remote_addr.sin_addr));
					continue;
	  		}

				/*
				#pragma single
				{
					//printf("%d\n", omp_get_thread_num());
		  		// Case 3: One of the sockets has read data
		  		if (!clear_socket2(events[i].data.fd)) {
						// epoll will remove the fd from its set
						// automatically when the fd is closed
						close(events[i].data.fd);
		  		}
				} */


				pthread_t new_thread;
				pthread_create(&new_thread, NULL, clear_socket, &(events[i].data.fd));
				pthread_join(new_thread, NULL);
			//}
		}
	}
	close(fd_server);
	exit(EXIT_SUCCESS);
}

void* clear_socket(void* gfd) {
	int	 n, bytes_to_read;
	char *bp, buf[BUFLEN];
	int *fd = gfd;

	//printf("%d\n", pthread_self());

	while (TRUE) {
		bp = buf;
		bytes_to_read = BUFLEN;

		while ((n = recv(*fd, bp, bytes_to_read, MSG_WAITALL)) > 0) {
			printf("sending:%s\n", buf);
			send(*fd, buf, BUFLEN, 0);
		}
		close(*fd);
		return (void*)TRUE;
	}
	close(*fd);
	return(0);
}

static int clear_socket2(int fd) {
	int	 n, bytes_to_read;
	char *bp, buf[BUFLEN];

	while (TRUE) {
		bp = buf;
		bytes_to_read = BUFLEN;

		while ((n = recv(fd, bp, bytes_to_read, MSG_WAITALL)) > 0) {
			printf("sending:%s\n", buf);
			send(fd, buf, BUFLEN, 0);
		}
		close(fd);
		return TRUE;
	}
	close(fd);
	return(0);
}

static void system_fatal(const char* message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void close_fd(int signo) {
  close(fd_server);
	exit(EXIT_SUCCESS);
}
