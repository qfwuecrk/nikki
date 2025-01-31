#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
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

int main(int argc, char **argv)
{
    int ret = 0;

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

    struct sockaddr_in laddr;
    laddr.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr.s_addr);
    laddr.sin_port = htons(atoi(client_conf.rcvport));

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

    // pipe();

    // fork();

    //  子进程：调用解码器
    //  父进程：从网络上收包

    return 0;
}