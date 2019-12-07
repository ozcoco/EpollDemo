#include <iostream>

static void test();

static void test2();

int main()
{

    //    test();
    test2();

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

template <typename __Type>
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

    struct sockaddr_in sockAddr
    {
    }, acceptAddr{};

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

    if (bind(listen_fd, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) != 0)
    {
        perror("bind socket failure !");

        exit(errno);
    }

    if (listen(listen_fd, 200) != 0)
    {
        perror("listen socket failure !");

        exit(errno);
    }

    c_fd = accept(listen_fd, (struct sockaddr *)&acceptAddr, &c_addr_len);

    if (c_fd < 0)
    {
        perror("accept socket failure !");

        exit(errno);
    }

    while (true)
    {
        size_t len = read(c_fd, buf, sizeof(buf));

        if (len == 0)
            continue;

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
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

/**
 * set sock resume
 *
 * **/
void set_sock_reuse(int sock_fd)
{

    int reuse = 1;

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &reuse, sizeof(reuse)) == -1)
    // if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
    {
        perror("set reuseaddr | reuseport  socket failure !");

        exit(errno);
    }
}

/**
 * init epoll
 *
 * **/
int create_epoll()
{
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);

    // int epoll_fd = epoll_create(1);

    if (epoll_fd == -1)
    {
        perror("epoll_create1");

        exit(EXIT_FAILURE);
    }

    return epoll_fd;
}

#define MAX_EVENTS 10

struct epoll_event events[MAX_EVENTS];

void add_epoll_event(int epoll_fd, int fd, struct epoll_event ev)
{
    if (-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev))
    {
        printf("epoll_ctl error, errno : %d \n", errno);
    }
}

void del_epoll_event(int epoll_fd, int fd)
{
    if (-1 == epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr))
    {
        printf("epoll_ctl error, errno : %d \n", errno);
    }
}

static void do_use_fd(const struct epoll_event &ev);

struct event_handle
{
    int epoll_fd;

    int fd;

    int events;

    void *arg;

    inline void send_callback()
    {
        char buf[BUFSIZ];

        bzero(buf, BUFSIZ);

        int len = read(fd, buf, BUFSIZ);

        write(STDOUT_FILENO, buf, len);
    }

    inline void recv_callback()
    {
        char buf[BUFSIZ];

        bzero(buf, BUFSIZ);

        for (;;)
        {
            int ret = recv(fd, buf, BUFSIZ, 0);

            if (ret < 0)
            {

                if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                    continue;

                printf("error\n");

                del_epoll_event(epoll_fd, fd);

                close(fd);

                break;
            }
            else if (ret == 0)
            {

                printf("客户端主动关闭请求！！！\n");

                del_epoll_event(epoll_fd, fd);

                close(fd);

                break;
            }
            else
            {
                write(STDOUT_FILENO, buf, ret);

                if (ret < BUFSIZ)
                {
                    printf("read finished \n");

                    break;
                }
            }
        }
    }

    inline void error_callback()
    {
    }
};

static void send_callback(int epoll_fd, int fd, void *arg);

static void recv_callback(int epoll_fd, int fd, void *arg);

static void error_callback(int epoll_fd, int fd, void *arg);

#define MAX_LINE 10
// #define MAX_EVENTS 10
#define MAX_LISTENFD 5

static int create_and_listen()
{
    int on = 1;

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;

    // fcntl(listenfd, F_SETFL, O_NONBLOCK);
    set_nonblock(listenfd);

    // setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    set_sock_reuse(listenfd);

    servaddr.sin_family = AF_INET;

    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    servaddr.sin_port = htons(9999);

    if (-1 == bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)))
    {
        printf("bind errno, errno : %d \n", errno);
    }

    if (-1 == listen(listenfd, MAX_LISTENFD))
    {
        printf("listen error, errno : %d \n", errno);
    }
    printf("listen in port 9999 !!!\n");

    return listenfd;
}

static void test2()
{
    const int &sock_fd = create_and_listen();

    struct sockaddr_in acceptAddr;

    socklen_t addr_len = sizeof(struct sockaddr);
    ;

    const int &epoll_fd = create_epoll();

    struct epoll_event ev;

    ev.data.fd = sock_fd;

    ev.events = EPOLLIN | EPOLLET;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ev);

    for (;;)
    {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1); //时间参数为0表示立即返回，为-1表示无限等待

        if (nfds == -1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == sock_fd)
            {

                int conn_sock = accept4(sock_fd, (struct sockaddr *)&acceptAddr, &addr_len,
                                        SOCK_CLOEXEC | SOCK_NONBLOCK);

                // int conn_sock = accept(sock_fd, (struct sockaddr *)&acceptAddr, &addr_len);

                if (conn_sock == -1)
                {
                    perror("--->accept");

                    printf("sock_fd: %d \n errno: %d", sock_fd, errno);

                    exit(EXIT_FAILURE);
                }

                set_nonblock(conn_sock);

                ev.events = EPOLLIN | EPOLLET;

                ev.data.fd = conn_sock;

                static struct event_handle evh;

                evh.fd = conn_sock;

                evh.epoll_fd = epoll_fd;

                evh.events = EPOLLIN;

                ev.data.ptr = &evh;

                add_epoll_event(epoll_fd, conn_sock, ev);
            }
            else
            {
                do_use_fd(events[n]);
            }
        }
    }

    close(sock_fd);

    for (auto &ev : events)
    {
        close(ev.data.fd);
    }
}

static void do_use_fd(const struct epoll_event &ev)
{
    struct event_handle *evh = static_cast<struct event_handle *>(ev.data.ptr);

    if (ev.events & EPOLLIN)
    {
        evh->recv_callback();
    }
    else if (ev.events & EPOLLOUT)
    {
        evh->send_callback();
    }
    else if (ev.events & EPOLLERR)
    {
        evh->error_callback();
    }
    else
    {
        //todo
    }
}
