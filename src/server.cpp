#include "auxlib.h"

#define m_lock pthread_mutex_lock(&m)
#define m_unlock pthread_mutex_unlock(&m)

void *handle_connection(void *vp);

int sock, running; //main socket that messages are received on

struct server_data
{
    uint32_t addr;
    uint16_t port;
    bool waiting; // if it is currently waiting
};

std::map<std::string, server_data> clients;

pthread_mutex_t m;

bool put_into_clients(const std::string &s, const uint32_t &a, const uint16_t &p)
{
    server_data sd = {a, p, false};
    return clients.emplace(std::make_pair(s, sd)).second;
}

void exit_function(int signum)
{
    (void)signum;
    running = 0;
}

void print_clients(int fd = STDOUT_FILENO)
{
    int j = 1;
    for (auto i = clients.begin(); i != clients.end(); i++)
    {
        if (i->second.waiting)
        {
            // dprintf(fd, "%d) %s ([%08X:%04hu])\n", j++, i->first.c_str(), i->second.addr, i->second.port);
            dprintf(fd, "%d) %s\n", j++, i->first.c_str());
        }
    }
    if (j == 1)
    {
        dprintf(fd, "No waiting clients\n");
    }
    return;
}

int main(int argc, const char *argv[])
{
    struct sockaddr_in servaddr;

    if (argc != 2)
    {
        fprintf(stderr, "./server <port-number>\n");
        if (argc < 2)
            return EXIT_FAILURE;
    }

    uint16_t port = (uint16_t)strtol(argv[1], nullptr, 10);
    if (port == 0)
    {
        fprintf(stderr, "error: port 0\n");
        return EXIT_FAILURE;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    cerr(sock, "socket()");
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = (argc > 2) ? htonl(INADDR_LOOPBACK) : htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    cerr(bind(sock, (struct sockaddr *)(&servaddr), sizeof(sockaddr)), "Bind()");
    listen(sock, 10);

    fd_set rfds;
    pthread_t ptt;

    running = 1;
    signal(SIGINT, exit_function);
    pthread_mutex_init(&m, NULL);
    while (running)
    {
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        if (select(sock + 1, &rfds, NULL, NULL, NULL) < 0)
        {
            if (errno == EINTR)
            {

                errno = 0;
                continue;
            }
            else
            {
                perror("Select():");
                return EXIT_FAILURE;
            }
        }
        pthread_create(&ptt, NULL, handle_connection, NULL);
        pthread_detach(ptt);
    }

    pthread_mutex_destroy(&m);
    close(sock);
    return EXIT_SUCCESS;
}

void *handle_connection(void *nothing)
{
    (void)nothing;
    struct sockaddr_in addr;
    socklen_t slen = sizeof(addr);
    int comm_fd = accept(sock, (struct sockaddr *)&addr, &slen);
    cerr(comm_fd, "accept()");

    char protocol_code = '0';
    cerr(recvfrom(comm_fd, &protocol_code, 1, MSG_WAITALL, NULL, NULL), "recv");
 
    std::string id = get_ID(comm_fd);
    m_lock; //critical is entire switch statement
    switch (protocol_code)
    {
    case 'a': //checking availability
    {
        uint16_t port;
        uint32_t ipv4_addr = (addr.sin_addr.s_addr);

        cerr((int)recv(comm_fd, &port, 2, 0), "recv()");
        if (clients.find(id) != clients.end())
        {
            send_char(comm_fd, 'n');
            fprintf(stderr, "client id %s is taken\n", id.c_str());
        }
        else
        {
            send_char(comm_fd, 'y');
            put_into_clients(id, ipv4_addr, port);
            fprintf(stderr, "Added %s %X %hu\n", id.c_str(), ipv4_addr, port);
        }
        break;
    }
    case 'q': //query ID for connection details
    {
        auto it = clients.find(id);
        if (it == clients.end() || !it->second.waiting)
        {
            send_char(comm_fd, 'n'); //doesn't exist
        }
        else
        {
            send_char(comm_fd, 'y');
            uint32_t a = it->second.addr;
            uint16_t p = it->second.port;
            it->second.waiting = false;
            send(comm_fd, &a, 4, 0);
            send(comm_fd, &p, 2, 0);
            fprintf(stderr, "%s: %8X %hu \n", id.c_str(), a, p);
        }
        break;
    }
    case 'r': //remove client
    {
        clients.erase(id);
        fprintf(stderr, "Removed %s\n", id.c_str());
        break;
    }
    case 'l':
    {
        print_clients(comm_fd);
        fprintf(stderr, "Listing clients\n");
        break;
    }
    case 'w': //wait
    case 'u': //unwait
    {
        auto it = clients.find(id);
        if (it != clients.end())
        {
            it->second.waiting = (protocol_code == 'w');
            printf("Client %s is %s waiting\n", id.c_str(), (protocol_code == 'w') ? "" : "not");
        }
        break;
    }
    case '0':
        //nothing
        break;
    default:
        fprintf(stderr, "Unrecognized protocol  code %c\n", protocol_code);
    }
    m_unlock;
    if (shutdown(comm_fd, SHUT_RDWR) < 0) // close TCP
        perror("shutdown()");
    close(comm_fd);
    return nullptr;
}
