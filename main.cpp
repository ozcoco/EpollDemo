#include <iostream>


static void test();


int main()
{

    test();


    return 0;
}


#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <error.h>
#include <sys/epoll.h>
#include <cstring>
#include <algorithm>
#include <functional>
#include <fcntl.h>


template<typename __Type>
class ToUpper
{
public:
    inline __Type operator()(__Type &__t)
    {
        return toupper(__t);
    }
};


#define PROTOCOL 0
#define PORT 9999
#define DOMAIN AF_INET
#define TYPE SOCK_STREAM
#define S_IP "127.0.0.1"


static void test()
{

    int listen_fd, c_fd;

    struct sockaddr_in sockAddr{}, acceptAddr{};

    socklen_t c_addr_len;

    char buf[BUFSIZ];

    memset(buf, 0, BUFSIZ);

    memset(&sockAddr, 0, sizeof(sockAddr));

    memset(&acceptAddr, 0, sizeof(acceptAddr));

    listen_fd = socket(DOMAIN, TYPE, PROTOCOL);

    if (listen_fd < 0)
    {
        perror("create socket failure !");

        exit(errno);
    }

    sockAddr.sin_family = AF_INET;

    sockAddr.sin_port = htons(PORT);

    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_fd, (struct sockaddr *) &sockAddr, sizeof(sockAddr)) != 0)
    {
        perror("bind socket failure !");

        exit(errno);
    }

    if (listen(listen_fd, 200) != 0)
    {
        perror("listen socket failure !");

        exit(errno);
    }

    c_fd = accept(listen_fd, (struct sockaddr *) &acceptAddr, &c_addr_len);

    if (c_fd < 0)
    {
        perror("accept socket failure !");

        exit(errno);
    }


    while (true)
    {
        size_t len = read(c_fd, buf, sizeof(buf));

        if (len == 0) continue;

        if (strstr(buf, "exit"))
        {
            break;
        }

        printf("%s\n", buf);

        std::transform(buf, buf + len, buf, ToUpper<char>());

        write(c_fd, buf, len);

        memset(buf, 0, len);
    }

    close(c_fd);

    close(listen_fd);

    printf("exit!!!");

}


/**
 * set fd nonblock
 *
 * **/
void set_nonblock(int fd)
{
    int flag = fcntl(fd, F_GETFD);

    flag |= O_CLOEXEC;

    fcntl(fd, F_SETFD, flag); //close to exec

    flag = fcntl(fd, F_GETFL);

    flag |= O_NONBLOCK;

    fcntl(fd, F_SETFL, flag);
}


/**
 * set sock resume
 *
 * **/
void set_sock_reuse(int sock_fd)
{
    int reuse = 1;

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &reuse, sizeof(reuse)) == -1)
    {
        perror("set reuseaddr | reuseport  socket failure !");

        exit(errno);
    }
}


/**
 * init epoll
 *
 * **/
void init_epoll(int *epoll_fd)
{
    *epoll_fd = epoll_create1(EPOLL_CLOEXEC);

    if (*epoll_fd == -1)
    {
        perror("epoll_create1");

        exit(EXIT_FAILURE);
    }
}


#define MAX_EVENTS 10

struct epoll_event events[MAX_EVENTS];

void add_epoll_event(int epoll_fd, struct epoll_event &ev)
{

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

}


static void test2()
{
    int epoll_fd;

    int sock_fd, c_fd;

    struct sockaddr_in sockAddr{}, acceptAddr{};

    socklen_t c_addr_len;

    char buf[BUFSIZ];

    memset(buf, 0, BUFSIZ);

    memset(&sockAddr, 0, sizeof(sockAddr));

    memset(&acceptAddr, 0, sizeof(acceptAddr));

    sock_fd = socket(DOMAIN, TYPE, PROTOCOL);

    if (sock_fd < 0)
    {
        perror("create socket failure !");

        exit(errno);
    }

    set_nonblock(sock_fd);

    set_sock_reuse(sock_fd);

    init_epoll(&epoll_fd);


    {//add sock event
        struct epoll_event sock_ev{};

        sock_ev.events = EPOLLIN | EPOLLET;

        sock_ev.data.fd = sock_fd;

        add_epoll_event(epoll_fd, sock_ev);
    }

    sockAddr.sin_family = AF_INET;

    sockAddr.sin_port = htons(PORT);

    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock_fd, (struct sockaddr *) &sockAddr, sizeof(sockAddr)) != 0)
    {
        perror("bind socket failure !");

        exit(errno);
    }

    if (listen(sock_fd, 200) != 0)
    {
        perror("listen socket failure !");

        exit(errno);
    }





}