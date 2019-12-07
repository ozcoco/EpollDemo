
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
 
 
#define MAX_LINE     10
#define MAX_EVENTS   10
#define MAX_LISTENFD 5
 
int createAndListen() {
	int on = 1;
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in servaddr;
	fcntl(listenfd, F_SETFL, O_NONBLOCK);
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(9999);
 
	if (-1 == bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)))  {
		printf("bind errno, errno : %d \n", errno); 
	}
 
	if (-1 == listen(listenfd, MAX_LISTENFD))  {
		printf("listen error, errno : %d \n", errno); 
	}
	printf("listen in port 5859 !!!\n");
	return listenfd;
}
 
 
int main(int argc, char const *argv[])
{
	struct epoll_event ev, events[MAX_EVENTS];
	int epollfd = epoll_create(1);     //这个参数已经被忽略，但是仍然要大于
	if (epollfd < 0)  {
		printf("epoll_create errno, errno : %d\n", errno);
	}
	int listenfd = createAndListen();
	ev.data.fd = listenfd;
	ev.events = EPOLLIN;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);
 
	for ( ;; )  {
		int fds = epoll_wait(epollfd, events, MAX_EVENTS, -1);   //时间参数为0表示立即返回，为-1表示无限等待
		if (fds == -1)  {
			printf("epoll_wait error, errno : %d \n", errno);
			break;
		}
		else {
			printf("trig %d !!!\n", fds);
		}
 
		for (int i = 0; i < fds; i++)  {
			if (events[i].data.fd == listenfd)  {
				struct sockaddr_in cliaddr;
				socklen_t clilen = sizeof(struct sockaddr_in);
				int connfd = accept(listenfd, (sockaddr*)&cliaddr, (socklen_t*)&clilen);
				if (connfd > 0)  {
					printf("new connection from %s : %d, accept socket fd: %d \n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), connfd);
				}
				else  {
					printf("accept error, connfd : %d, errno : %d \n", connfd, errno);
				}
				fcntl(connfd, F_SETFL, O_NONBLOCK); 
				ev.data.fd = connfd;
				ev.events = EPOLLIN | EPOLLET;
				if (-1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev))  {
					printf("epoll_ctl error, errno : %d \n", errno);
				}
			}
			else if (events[i].events & EPOLLIN)  {
				int sockfd;
				if ((sockfd =events[i].data.fd) < 0)  {
					printf("EPOLLIN socket fd < 0 error \n");
					continue;
				}
				char szLine[MAX_LINE + 1] ;
				int readLen = 0;
				bzero(szLine, MAX_LINE + 1);
				if ((readLen = read(sockfd, szLine, MAX_LINE)) < 0)  {
					printf("readLen is %d, errno is %d \n", readLen, errno);
					if (errno == ECONNRESET)  {
						printf("ECONNRESET closed socket fd : %d \n", events[i].data.fd);
						close(sockfd);
					}
				}
				else if (readLen == 0)  {
					printf("read 0 closed socket fd : %d \n", events[i].data.fd);
					//epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd , NULL);  
					//close(sockfd);
				}
				else  {
					printf("read %d content is %s \n", readLen, szLine);
				}
 
				bzero(szLine, MAX_LINE + 1);
				if ((readLen = read(sockfd, szLine, MAX_LINE)) < 0)  {
					printf("readLen2 is %d, errno is %d , ECONNRESET is %d \n", readLen, errno, ECONNRESET);
					if (errno == ECONNRESET)  {
						printf("ECONNRESET2 closed socket fd : %d \n", events[i].data.fd);
						close(sockfd);
					}
				}
				else if (readLen == 0)  {
					printf("read2 0 closed socket fd : %d \n", events[i].data.fd);
				}
				else  {
					printf("read2 %d content is %s \n", readLen, szLine);
				}
			}
		}
 
	}
	return 0;
}