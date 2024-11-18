#include <head.h>

// 定义子线程传参结构体
struct Client_data
{
    int accept_fd;
    struct sockaddr_in client_addr;
};

char* errmsg = NULL;

// 函数声明
int InitDB(void);
int actionDB();
void *thread_func(void *arg);


int main(int argc, char const *argv[])
{
    // 入参检测
    if (3 != argc)
    {
        printf("Useage: %s ip port\n", argv[0]);
        exit(0);
    }

    // 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd)
    {
        ERR_LOG("socket error");
    }
    
    // 允许端口快速重用
    int reuse = 1;
    if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)))
    {
        ERR_LOG("setsockopt error");
    }

    // 服务端信息结构体
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    socklen_t server_addr_len = sizeof(server_addr);

    // 绑定   
    if(-1 == bind(sockfd, (struct sockaddr *)&server_addr, server_addr_len))
    {
        ERR_LOG("bind error");
    }

    // 监听
    if(-1 == listen(sockfd, 10))
    {
        ERR_LOG("listen error");
    }

    // 初始化数据库
    InitDB();

    // 客户端信息结构体
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(client_addr));

    // 多线程任务处理
    pthread_t tid;
    struct Client_data client_data;
    int accept_fd;

    while (1)
    {
        printf("等待客户端连接...\n");
        // 循环连接客户端
        accept_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (-1 == accept_fd)
        {
            ERR_LOG("accept error");
        }
        printf("客户端[accept_fd:%d, ip:%s, port:%d]连接成功\n", accept_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // 创建子线程
        client_data.accept_fd = accept_fd;
        client_data.client_addr = client_addr;
        pthread_create(&tid, NULL, thread_func, (void *)&client_data);
        
        // 分离子线程
        pthread_detach(tid);

    }
    

    // 关闭套接字
    close(sockfd);

    return 0;
}

// 创建数据库、表
int InitDB(void)
{
    // 创建数据库并打开
    sqlite3 *db;
    if(sqlite3_open("./transparent.db", &db) != SQLITE_OK)
    {
        printf("sqlite3_open failed: %d:%s\n", sqlite3_errcode(db), sqlite3_errmsg(db));
        return -1;
    }
    printf("sqlite3_open success\n");

    // 创建Users表
    char sql[256] = "create table if not exists users (id integer primary key autoincrement, name char[30], passwd text)";
    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg))
    {
        fprintf(stderr, "SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    printf("create table success\n");

    // 创建Devices表
    sql[256] = "create table if not exists devices (id integer primary key autoincrement, device_name char[30], active integer, foreign key(id) references users(id))";
    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg))
    {
        fprintf(stderr, "SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }

    // 关闭数据库
    sqlite3_close(db);
    return 0;
}

// 子线程处理函数
void *thread_func(void *arg)
{
    // 获取传参
    int accept_fd = ((struct Client_data *)arg) -> accept_fd;
    struct sockaddr_in client_addr = ((struct Client_data *)arg) -> client_addr;;
    char buf[1024] = "";
    ssize_t res = 0;

    while (1)
    {
        memset(buf, 0, sizeof(buf));
        // 接收数据
        res = recv(accept_fd, buf, sizeof(buf), 0);
        if (-1 == res)
        {
            perror("recv error");
            break;
        } else if (0 == res)
        {
            printf("客户端[accept_fd:%d:%s:%d]断开连接...\n", accept_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            close(accept_fd);
            pthread_exit(NULL);
            break;
        }
        printf("客户端[%s:%d]发送了数据:%s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buf);

        // 发送数据
        memset(buf, 0, sizeof(buf));
        strcat(buf, "服务器收到数据");
        res = send(accept_fd, buf, sizeof(buf), 0);
        if (-1 == res)
        {
            perror("send error");
            break;
        }
        printf("数据发送成功\n");
    }

    // 关闭连接
    close(accept_fd);
    // 释放线程
    pthread_exit(NULL);
}
