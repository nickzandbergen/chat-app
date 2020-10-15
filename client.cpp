#include "auxlib.h"
#include <readline/readline.h> // readline 8.0+ 

using namespace std;

#define INFO 1
#define CHAT 2
#define WAIT 3
#define DONE -1

//global vars for callback function
volatile int flag = 0;
int chat_sock = 0, server_sock = 0;
int state = 0;
char *prompt;
char *prefix;
char *partner = nullptr;
const char *id;

uint32_t address;
uint16_t port;

static void readline_callback_fn(char *str);

bool is_alphanum(const char *s)
{
    while (*s != '\0')
    {
        if (!isalnum(*s))
            return false;
        s++;
    }
    return true;
}

void handle_sigint(int signum)
{
    (void)signum;
    flag |= 1;
    return;
}

int send_cmd(char c, const char *str)
{
    int s = get_socket(address, port);
    send_char(s, c);
    send_ID(s, str);
    return s;
}

void stop_waiting()
{

    close(send_cmd('u', id));
    rl_clear_visible_line();
    printf("Stopped waiting\n");
    rl_forced_update_display();
    return;
}

void init_chat(const char *str)
{
    if (state == WAIT)
    {
        printf("Error, cannot connect in wait mode.\n");
        return;
    }
    char res;
    sockaddr_in sa;
    uint32_t a;
    uint16_t p;

    int s = send_cmd('q', str + 9);
    recv(s, &res, sizeof(res), 0); //server response
    if (res == 'n')
    {
        fprintf(stderr, "Client %s does not exist or is not waiting\n", str + 9);
        state = INFO;
        shutdown(s, SHUT_RDWR);
        return;
    }
    state = CHAT;

    chat_sock = cerr(socket(AF_INET, SOCK_STREAM, 0), "socket()");
    recv(s, &a, 4, 0);
    recv(s, &p, 2, 0);
    bzero(&sa, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = a;
    sa.sin_port = p;

    if (connect(chat_sock, (struct sockaddr *)(&sa), sizeof(sa)) < 0)
    {
        perror("Couldn't connect");
        state = INFO;
        send_char(s, 'r'); //send removal request because person is dead
        send_ID(s, str + 9);
        close_if_nonzero(chat_sock, __LINE__);
        shutdown(s, SHUT_RDWR);
        return;
    }
    send_char(chat_sock, 'c');
    cerr(recv(chat_sock, &res, sizeof(res), 0));
    if (res != 'y')
    {
        close_if_nonzero(chat_sock, __LINE__);
        fprintf(stderr, "Client \"%s\" doesn't want to talk\n", str + 9);
    }
    else
    {
        printf("Connected successfully to %s\n", str + 9);
        partner = (char *)calloc(sizeof(char), strlen(str + 9));
        strcpy(partner, str + 9);
        send_ID(chat_sock, id);
    }

    shutdown(s, SHUT_RDWR);
    return;
}

void recv_chat()
{
    char code;
    int temp_sock = accept(server_sock, NULL, NULL);
    recv(temp_sock, &code, sizeof(code), 0);
    if (code == 'c')
    {
        if (state == WAIT)
        {
            code = 'y';
            chat_sock = temp_sock;
            send(chat_sock, &code, 1, 0);
            partner = get_len_prefaced_buf(chat_sock);
            rl_clear_visible_line();
            printf("Connection from %s\n", partner);
            rl_forced_update_display();
            state = CHAT;
        }
        else
        {
            code = 'n';
            send(temp_sock, &code, 1, 0);
            close(temp_sock);
        }
    }
    else if (code == 'm') // murder
    {
        state = DONE;
        fprintf(stderr, "Killed by server\n");
    }
    else
    {
        close(temp_sock);
    }
    return;
}

void handle_chat()
{
    //read in uint16_t for message length
    char c;
    ssize_t st = recv(chat_sock, &c, 1, MSG_PEEK);
    if (st < 0)
    {
        perror("recv() on chat");
    }
    else if (st == 0)
    {
        rl_clear_visible_line();
        printf("Conversation with %s over\n", partner);
        free_if_nonnull(partner);
        close_if_nonzero(chat_sock, __LINE__);
        rl_forced_update_display();
        state = INFO;
    }
    else
    {
        rl_clear_visible_line();
        char *p; //prompt
        char *m; //message
        p = get_len_prefaced_buf(chat_sock);
        m = get_len_prefaced_buf(chat_sock);
        printf("%s%s\n", p, m);
        free(p);
        free(m);
        rl_forced_update_display();
    }
    return;
}

int main(int argc, const char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "./client <IP-address> <port> <ID>\n");
        return EXIT_FAILURE;
    }

    arg_parsing(argv, address, port);

    if (strlen(argv[3]) > 255)
    {
        fprintf(stderr, "Invalid ID: max of 255 characters\n");
        return EXIT_FAILURE;
    }
    if (!is_alphanum(argv[3]))
    {
        fprintf(stderr, "Invalid ID %s: invalid characters\n", argv[3]);
        return EXIT_FAILURE;
    }

    id = argv[3];

    { //open server
        struct sockaddr_in servaddr;
        socklen_t ga_l = sizeof(servaddr);

        server_sock = socket(AF_INET, SOCK_STREAM, 0);
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = 0;

        cerr(bind(server_sock, (struct sockaddr *)(&servaddr), sizeof(sockaddr)), "Bind()");
        getsockname(server_sock, (struct sockaddr *)(&servaddr), &ga_l);

        char c;
        int s = send_cmd('a', id);
        send(s, &servaddr.sin_port, sizeof(servaddr.sin_port), 0);
        recv(s, &c, 1, MSG_WAITALL);
        close(s);

        if (c != 'y')
        {
            if (c == 'n')
            {
                fprintf(stderr, "Client ID is already taken.\n");
            }
            else
            {
                fprintf(stderr, "Invalid server response %c\n", c);
            }
            return EXIT_FAILURE;
        }
        listen(server_sock, 10);
    }

    prompt = (char *)calloc(sizeof(char), strlen(argv[3]) + 3);
    prefix = (char *)calloc(sizeof(char), strlen(argv[3]) + 3);
    { //make prompt
        strcpy(prompt, id);
        strcpy(prompt + strlen(id), "> ");
        strcpy(prefix, id);
        strcpy(prefix + strlen(id), ": ");
    }

    signal(SIGINT, handle_sigint);
    signal(SIGPIPE, SIG_IGN);
    rl_callback_handler_install(prompt, readline_callback_fn);

    state = INFO;
    fd_set rfds;
    while (state != DONE)
    {
        //lots of readline alternate interface
        FD_ZERO(&rfds);
        FD_SET(server_sock, &rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(chat_sock, &rfds);

        int max_fd = max(server_sock, chat_sock);
        int rv = select(max_fd + 1, &rfds, NULL, NULL, NULL);
        // use rl_clear_visible_line();

        if (rv < 0)
        {
            if (errno != EINTR)
            {
                perror("select()");
                break;
            }
        }
        else // rv > 0
        {
            if (FD_ISSET(STDIN_FILENO, &rfds))
                rl_callback_read_char();

            if (FD_ISSET(server_sock, &rfds))
                recv_chat();

            if (chat_sock && FD_ISSET(chat_sock, &rfds))
                handle_chat();
        }

        if (flag) //^C
        {
            flag = 0;
            switch (state)
            {
            case CHAT:
            {
                close_if_nonzero(chat_sock, __LINE__);
                rl_clear_visible_line();
                printf("Stopped chatting with %s\n", partner);
                rl_forced_update_display();
                free_if_nonnull(partner);
                break;
            }

            case WAIT:
            {
                stop_waiting();
                break;
            }

            default:
                break;
            }
            state = INFO;
        }
    }
    rl_callback_handler_remove();
    close(send_cmd('r', id));
    close(server_sock);
    free(prompt);
    free(prefix);
    return EXIT_SUCCESS;
}

static void readline_callback_fn(char *str)
{
    if (str[0] == '/')
    {
        if (strncmp(str, "/quit", 5) == 0 || strncmp(str, "/exit", 5) == 0)
        {
            if (state == WAIT)
            {
                stop_waiting();
            }
            else if (state == CHAT)
            {
                free_if_nonnull(partner);
                close_if_nonzero(chat_sock, __LINE__);
                printf("Stopped chatting; exiting\n");
            }
            else if (state == INFO)
            {
                rl_already_prompted = 1;
            }
            state = DONE;
        }
        else if (strncmp(str, "/state", 5) == 0)
        {
            printf("state: %d\n", state);
            printf("csock: %d\n", chat_sock);
            printf("ssock: %d\n", server_sock);
        }
        else if (strncmp(str, "/wait", 5) == 0)
        {
            if (state == INFO)
            {
                state = WAIT;
                close(send_cmd('w', id));
                printf("Waiting for connection\n");
            }
            else if (state == WAIT)
                printf("Already waiting. ^C to stop.\n");
            else
                printf("Cannot wait in this state (%d)\n", state);
        }
        else if (strncmp(str, "/list", 5) == 0)
        {
            if (state == INFO)
            {
                int s = send_cmd('l', id);
                shutdown(s, SHUT_WR); //to send 1 byte on a socket withoutt io_ctl shennanigans
                ssize_t rd;
                char buf[256];
                while ((rd = recv(s, buf, sizeof(buf), 0)) > 0)
                {
                    write(STDOUT_FILENO, buf, rd);
                }
                if (rd < 0)
                    perror("recv()");

                shutdown(s, SHUT_RD);
                close(s);
            }
            else
                printf("/list not supported in %s\n", (state == WAIT) ? "wait" : "chat");
        }
        else if (strncmp(str, "/connect ", 9) == 0)
        {
            init_chat(str);
        }
        else
        {
            fprintf(stderr, "Unrecognized command \"%s\"\n", str);
        }
    }
    else //not a command
    {
        if (state == CHAT && strlen(str) > 0) //if in chat mode send it
        {
            send_ID(chat_sock, prefix);
            send_ID(chat_sock, str);
        }
    }
    free((void *)str);
    return;
}