#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <proto.h>
#include "client.h"

struct client_conf_st client_conf = {
    .rcvport = DEFAULT_RCVPORT,
    .mgroup = DEFAULT_MGROUP,
    .player_cmd = DEFAULT_PLAYERCMD};

static void print_help();

static void print_help()
{
    /** 初始化级别： 默认值 < 配置文件 < 环境变量 < 命令行参数 */
    printf("%s\n", "-P, --port\t\t指定接收端口");
    printf("%s\n", "-M, --mgroup\t\t指定多播组");
    printf("%s\n", "-p, --player\t\t指定播放器");
    printf("%s\n", "-H, --help\t\t显示帮助");

    return;
}

static ssize_t writen(int fd, const char *buf, size_t count);

static ssize_t writen(int fd, const char *buf, size_t count)
{
    int pos = 0;
    ssize_t len;
    while (count > 0)
    {
        len = write(fd, buf + pos, count);
        if (len < 0)
        {
            if (errno == EINVAL)
                continue;
            perror("write()");
            return -1;
        }
        count -= len;
        pos += len;
    }
    return pos;
}

int main(int argc, char **argv)
{
    /* init */
    int ret = 0;
    size_t len = 0;
    int chosenid = 0;

    int index = 0;
    int cmd = 0;
    struct option argarr[] = {
        {"port", 1, NULL, 'P'},
        {"mgroup", 1, NULL, 'M'},
        {"player", 1, NULL, 'p'},
        {"help", 0, NULL, 'H'},
        {NULL, 0, NULL, 0}};

    int client_socket = 0;

    struct ip_mreqn mreqn;
    inet_pton(AF_INET, client_conf.mgroup, &mreqn.imr_multiaddr);
    inet_pton(AF_INET, "0.0.0.0", &mreqn.imr_address);
    mreqn.imr_ifindex = if_nametoindex("eth1");

    int val = 0;

    struct sockaddr_in laddr, serveraddr, raddr;
    socklen_t serveaddr_len, raddr_len;

    laddr.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr.s_addr);
    laddr.sin_port = htons(atoi(client_conf.rcvport));

    int pipefd[2] = {0};

    pid_t pid;

    struct msg_list_st *msg_list;
    struct msg_listentry_st *pos;
    struct msg_channel_st *msg_channel;

    /* getopt */
    while (true)
    {
        cmd = getopt_long(argc, argv, "P:M:p:H", argarr, &index);
        if (cmd < 0)
            break;

        switch (cmd)
        {
        case 'P':
            client_conf.rcvport = optarg;
            break;
        case 'M':
            client_conf.mgroup = optarg;
            break;
        case 'p':
            client_conf.player_cmd = optarg;
            break;
        case 'H':
            print_help();
            exit(EXIT_SUCCESS);
            break;
        default:
            abort();
            break;
        }
    }

    /* socket */
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    ret = setsockopt(client_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreqn, sizeof(mreqn));
    if (ret < 0)
    {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }

    val = 1;
    ret = setsockopt(client_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &val, sizeof(val));
    if (ret < 0)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    ret = bind(client_socket, (void *)&laddr, sizeof(laddr));
    if (ret < 0)
    {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    /* pipe */
    ret = pipe(pipefd);
    if (ret < 0)
    {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0)
    {
        perror("fork()");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        /* child */
        close(client_socket);
        close(pipefd[1]);
        dup2(pipefd[0], 0);
        if (pipefd[0] > 0)
            close(pipefd[0]);

        execl("/usr/bin/bash", "sh", "-c", client_conf.player_cmd, NULL);
        perror("execl()");
        exit(EXIT_FAILURE);
    }

    msg_list = malloc(MSG_LSIT_MAX);
    if (msg_list == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        len = recvfrom(client_socket, msg_list, MSG_LSIT_MAX, 0, (void *)&serveraddr, &serveaddr_len);
        if (len < sizeof(struct msg_list_st))
        {
            fprintf(stderr, "message is to small.\n");
            continue;
        }
        if (msg_list->chnid != LISTCHNID)
        {
            fprintf(stderr, "chnid is not match.\n");
            continue;
        }
        break;
    }

    for (pos = msg_list->entry; (char *)pos < ((char *)msg_list) + len; pos = (void *)((char *)pos + ntohs(pos->len)))
    {
        fprintf(stdout, "chnnel\t%d : %s\n", pos->chnid, pos->desc);
    }

    free(msg_list);

    while (true)
    {
        ret = scanf("%d", &chosenid);
        if (ret != 1)
            exit(EXIT_FAILURE);
    }

    msg_channel = malloc(MSG_CHANNEL_MAX);
    if (msg_channel == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        len = recvfrom(client_socket, msg_channel, MSG_CHANNEL_MAX, 0, (void *)&raddr, &raddr_len);
        if (raddr.sin_addr.s_addr != serveraddr.sin_addr.s_addr)
        {
            fprintf(stderr, "Ignore: address not match.\n");
            continue;
        }
        if (len < sizeof(struct msg_channel_st))
        {
            fprintf(stderr, "Ignore: message too smail.\n");
            continue;
        }
        if (msg_channel->chnid == chosenid)
        {
            ret = writen(pipefd[1], (void *)msg_channel->data, len-sizeof(chnid_t));
            if (ret < 0)
            {
                exit(EXIT_FAILURE);
            }
        }
    }

    free(msg_channel);
    close(client_socket);

    return 0;
}