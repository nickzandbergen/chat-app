#include "nzandber_lib.h"

int cerr(int return_value, const char *err_msg)
{
    if (return_value < 0)
    { //there is an error
        perror(err_msg);
        exit(EXIT_FAILURE);
    }
    return return_value;
}

int arg_parsing(const char *argv[], uint32_t &address, uint16_t &port)
{
    //parse IPv4 address
    uint32_t ip_a[4];
    int scanned = sscanf(argv[1], "%u.%u.%u.%u", ip_a + 3, ip_a + 2, ip_a + 1, ip_a + 0);
    if (scanned < 4)
    {
        fprintf(stderr, "Incomplete IP address: %d/4\n", scanned);
        exit(EXIT_FAILURE);
    }
    if ((ip_a[0] | ip_a[1] | ip_a[2] | ip_a[3]) > 0xFF)
    {
        fprintf(stderr, "parse_server_file: IP address field too big\n");
        ip_a[0] &= 0xFF;
        ip_a[1] &= 0xFF;
        ip_a[2] &= 0xFF;
        ip_a[3] &= 0xFF;
    }
    address = (ip_a[0]) | (ip_a[1] << 8) | (ip_a[2] << 16) | (ip_a[3] << 24);
    port = (uint16_t)strtol(argv[2], nullptr, 10);
    if (port == 0)
    {
        fprintf(stderr, "Port 0\n");
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}

int get_socket(const uint32_t &address, const uint16_t &port)
{
    int sock = cerr(socket(AF_INET, SOCK_STREAM, 0), "socket()");
    struct sockaddr_in s;
    bzero(&s, sizeof(s));
    s.sin_family = AF_INET;
    s.sin_addr.s_addr = htonl(address);
    s.sin_port = htons(port);
    cerr(connect(sock, (struct sockaddr *)(&s), sizeof(s)), "connect()");
    return sock;
}

ssize_t send_char(int fd, const char c)
{
    return send(fd, &c, 1, 0);
}

void send_ID(int sock, const char *ID_string)
{
    uint16_t len = (uint16_t)strlen(ID_string);
    send(sock, &len, sizeof(len), 0);
    send(sock, ID_string, (size_t)len, 0);
    return;
}

char *get_len_prefaced_buf(int comm_fd)
{
    uint16_t size = 0;
    cerr((int)recv(comm_fd, &size, sizeof(size), 0), "recv()");
    char *buf = (char *)malloc((size_t)(size + 1));
    buf[size] = '\0';
    cerr((int)recv(comm_fd, buf, size, 0), "recv()");
    return buf;
}

std::string get_ID(int comm_fd)
{
    char *buf = get_len_prefaced_buf(comm_fd);
    std::string id(buf);
    free(buf);
    return id;
}

void close_if_nonzero(int &sock, int linenum)
{
    if (sock)
    {
        close(sock);
        sock = 0;
    }
    else
    {
        fprintf(stderr, "%3d: Attempted to close STDIN\n", linenum);
    }
}

void free_if_nonnull(char *&p)
{
    if (p != nullptr)
    {
        free(p);
        p = nullptr;
    }
}