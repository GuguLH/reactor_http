#include "tcp_server.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <port> <pathname>.\n", argv[0]);
    }

    int ret = 0;
    // 切换工作目录
    ret = chdir(argv[2]);
    if (ret == -1)
    {
        perror("Error chdir()");
        return -1;
    }

    unsigned short port = atoi(argv[1]);

    tcp_server_t *server = tcp_server_init(port, 4);
    tcp_server_run(server);

    return 0;
}