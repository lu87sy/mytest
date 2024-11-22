#ifndef __HEAD_H__
#define __HEAD_H__

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sqlite3.h>
#include <time.h>

// 定义子线程传参结构体
struct Client_data
{
    int accept_fd;
    struct sockaddr_in client_addr;
};

// 定义数据库参数结构体
struct data_info
{
    char username[20];
    char passwd[20];
    char devname[20];
    char type[20];
    char data[512];
    int active;
    pthread_t tid;
    int accept_fd;
};

// 函数声明
int InitDB(void);
int actionDB(int cases, struct data_info *data);
void *thread_func(void *arg);


#define ERR_LOG(msg) \
    do                 \
    {                  \
        printf("%s %s %d:", __FILE__, __func__, __LINE__); \
        perror(msg);   \
        return -1;     \
    } while (0)

#endif