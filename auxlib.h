// C includes
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// C++ includes
#include <string>
#include <map>

int cerr(int return_value, const char *err_msg = "Forgot an error message for cerr()");
int arg_parsing(const char *argv[], uint32_t &address, uint16_t &port);
int get_socket(const uint32_t &address, const uint16_t &port);
ssize_t send_char(int fd, const char c);
void send_ID(int sock, const char *IDstring);
std::string get_ID(int comm_fd);
char *get_len_prefaced_buf(int comm_fd);
void free_if_nonnull(char *&p);
void close_if_nonzero(int &sock, int linenum = 0);
