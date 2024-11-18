#include <head.h>

// 定义子线程传参结构体
struct Client_data
{
    int accept_fd;
    struct sockaddr_in client_addr;
};

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
    if(-1 == listen(sockfd, 5))
    {
        ERR_LOG("listen error");
    }

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
        printf("客户端连接成功\n");

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

void *thread_func(void *arg)
{
    // 获取传参
    struct Client_data *client_data = (struct Client_data *)arg;
    char buf[1024] = {0};
    ssize_t res = 0;

    while (1)
    {
        memset(buf, 0, sizeof(buf));
        // 接收数据
        res = recv(client_data->accept_fd, buf, sizeof(buf), 0);
        if (-1 == res)
        {
            perror("recv error");
            break;
        } else if (0 == res)
        {
            printf("客户端[%s:%d]断开连接...\n", inet_ntoa(client_data->client_addr.sin_addr), ntohs(client_data->client_addr.sin_port));
            close(client_data->accept_fd);
            pthread_exit(NULL);
        }
        printf("客户端[%s:%d]说：%s\n", inet_ntoa(client_data->client_addr.sin_addr), ntohs(client_data->client_addr.sin_port), buf);
        

    }
    

}
