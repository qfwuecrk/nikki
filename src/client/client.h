#ifndef CLIENT_H__
#define CLIENT_H__

#define DEFAULT_PLAYERCMD "mpg123 - > /dev/null 2>&1"

struct client_conf_st
{
    char *rcvport;
    char *mgroup;
    char *player_cmd;
};

extern struct client_conf_st client_conf;

#endif