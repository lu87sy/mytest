#include <head.h>

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
    int active;
    pthread_t tid;
    int accept_fd;
};


char* errmsg = NULL;

// 函数声明
int InitDB(void);
int actionDB(int cases, struct data_info *data);
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
        // 子线程id
        // pthread_t thread;
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

    // 启用外键支持
    char sql[256] = "PRAGMA foreign_keys = ON";
    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    

    // 创建Users表
    sprintf(sql, "create table if not exists users (id integer primary key autoincrement, name char[30], passwd text)");
    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg))
    {
        printf("SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    printf("create table success\n");

    // 创建Devices表
    sprintf(sql,"create table if not exists devices (id integer primary key autoincrement, device_name char[30], active integer, thread_id integer, accept_fd integer, user_id integer, foreign key(user_id) references users(id))");
    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg))
    {
        printf("SQL error: %s\n", errmsg);
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
    struct data_info data;
    data.accept_fd = accept_fd;
    data.active = 0;    // 初始化设备状态
    char * tmp, *saveptr;
    char value[20];
    // char username[20], passwd[20], devname[20];
    // 打印子线程id
    printf("子线程id:%lu\n", pthread_self());

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
        
        // 校验数据格        
        if (strchr(buf, ';'))
        {
            // sscanf(buf, "%*[^u]usrname=%19[^,]",value);
            sscanf(buf, "%[^:]",value);
            // 服务器处理数据
            if (strcmp(value, "toServ") == 0)
            {
                printf("这是发给服务器处理的数据%s\n", value);
                // 判断注册或登录或获取时间
                sscanf(buf, "%*[^a]action=%19[^,;]", value);
                if (!strcmp(value, "reg"))
                {
                    printf("这是注册数据\n");
                    printf("当前会话的accept_fd:%d\n", accept_fd);
                    // 注册流程
                    sscanf(buf, "%*[^u]usrname=%19[^,]", data.username);
                    sscanf(buf, "%*[^p]passwd=%19[^;]", data.passwd);
                    if (strcmp(data.username, "") == 0 || strcmp(data.passwd, "") == 0)
                    {
                        if (-1 == send(accept_fd, "toCli:reg failed,username and password cannot be empty;" , strlen("toCli:reg failed,username and password cannot be empty;"), 0))
                        {
                            perror("send error");
                            break;
                        }
                    } else{
                        int res = actionDB(1, &data);
                        if (res == 1)
                        {
                            if (-1 == send(accept_fd, "toCli:reg failed,username already exists;" , strlen("toCli:reg failed,username already exists;"), 0))
                            {
                                perror("send error");
                                break;
                            }
                        } else {
                            if (-1 == send(accept_fd, "toCli:reg success;" , strlen("toCli:reg success;"), 0))
                            {
                                perror("send error");
                                break;
                            }
                            printf("注册成功\n");
                        }
                    }
                    
                } else if (!strcmp(value, "log"))
                {
                    printf("这是登录数据\n");
                    // 登录流程 toServ:action=log,usrname=xiaowang,passwd=123456,devname=sml001;
                    sscanf(buf, "%*[^u]usrname=%19[^,]", data.username);
                    sscanf(buf, "%*[^p]passwd=%19[^,]", data.passwd);
                    sscanf(buf, "%*[^,],%*[^,],%*[^,],devname=%49[^;]", data.devname);
                    printf("username:%s, passwd:%s, devname:%s\n", data.username, data.passwd, data.devname);
                    if (strcmp(data.username, "") == 0 || strcmp(data.passwd, "") == 0 || strcmp(data.devname, "") == 0)
                    {
                        if (-1 == send(accept_fd, "toCli:log failed,username,password and devname cannot be empty;" , strlen("toCli:log failed,username,password and devname cannot be empty;"), 0))
                        {
                            perror("send error");
                            break;
                        }
                    } else {
                        int res = actionDB(2, &data);
                        if (res == 1)
                        {
                            printf("2.%s设备在线\n", data.devname);
                            printf("accpet_fd:%d, name:%s, passwd:%s, devname:%s, thread_id:%lu\n", data.accept_fd, data.username, data.passwd, data.devname, data.tid);
                            // 同设备多次登录踢下线
                        } else if (res == 2) {
                            if (-1 == send(accept_fd, "toCli:log failed,username or password is wrong;" , strlen("toCli:log failed,username or password is wrong;"), 0))
                            {
                                perror("send error");
                                break;
                            }
                        } else {
                            if (-1 == send(accept_fd, "toCli:log success;" , strlen("toCli:log success;"), 0))
                            {
                                perror("send error");
                                break;
                            }
                            printf("登录成功\n");
                        }
                        
                    }
                    
                } else if (!strcmp(value, "gettime"))
                {
                    // 获取当前时间
                    time_t current_time;
                    time(&current_time);
                    
                    // 将时间转换为本地时间并格式化
                    struct tm *local_time = localtime(&current_time);
                    
                    sprintf(buf, "toCli:Ur MSG GOT!time=%04d-%02d-%02d %02d:%02d:%02d;", 
                        local_time->tm_year + 1900,
                        local_time->tm_mon + 1,
                        local_time->tm_mday,
                        local_time->tm_hour,
                        local_time->tm_min,
                        local_time->tm_sec);
                    if (-1 == send(accept_fd, buf, strlen(buf), 0))
                    {
                        perror("send error");
                        break;
                    }
                } else {
                    printf("数据格式错误\n");
                    if (-1 == send(accept_fd, "toCli:cmd invalid,invalid parameter for action;", strlen("toCli:cmd invalid,invalid parameter for action;"), 0))
                    {
                        perror("send error");
                        break;
                    }
                    
                }  
            } 
            // 服务器转发数据
            else if (strcmp(value, "toDev") == 0)
            {
                printf("这是需要服务器转发的数据\n");
            } else {
                printf("数据格式错误\n");
                // 发送错误信息
                if (-1 == send(accept_fd, "toCli:cmd invalid,missing colon;", strlen("toCli:cmd invalid,missing colon;"), 0))
                {
                    perror("send error");
                    break;
                }
            }
        } else {
            printf("数据格式错误\n");
            // 发送错误信息
            if (-1 == send(accept_fd, "toCli:cmd invalid,missing semicolon;", strlen("toCli:cmd invalid,missing semicolon;"), 0))
            {
                perror("send error");
                break;
            }
            
        }
        
        /*toCli:Ur MSG GOT!time=2021-12-15 19:11:31;
        toCli:cmd invalid,reason ^^^;
    toServ:action=reg,usrname=xiaowang,passwd=123456;
    toServ:action=log,usrname=xiaowang,passwd=123456,devname=sml001;
    toDev:devname=sml001,led=on,beep=on,oled=hello world,alarm=yes;
    toServ:action=gettime;
*/

    }

    // 关闭连接
    close(accept_fd);
    // 释放线程
    pthread_exit(NULL);
}

// 数据库操作
int actionDB(int cases, struct data_info *data)
{
    // 打开数据库
    sqlite3 *db;
    if (sqlite3_open("./transparent.db", &db) != SQLITE_OK)
    {
        printf("sqlite3_open failed: %d:%s\n", sqlite3_errcode(db), sqlite3_errmsg(db));
        return -1;
    }

    // 启用外键支持
    char sql[256] = "PRAGMA foreign_keys = ON";
    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }

    // 判断数据库操作类型
    char **pazResult = NULL;
    int nRow = 0, nColumn = 0;  // 定义行、列
    switch (cases)
    {
    // case 1为注册，2为登录，3为查询
    case 1:
        // 先判断用户名是否存在
        printf("已进入注册流程 %s, %s\n", data->username, data->passwd);
        sprintf(sql, "select * from users where name = '%s'", data->username);
        if (sqlite3_get_table(db, sql, &pazResult, &nRow, &nColumn, &errmsg) != SQLITE_OK)
        {
            printf("SQL error: %s\n", errmsg);
            sqlite3_free(errmsg);
            return -1;
        }
        if (nRow == 1)
        {
            // 通过flag判断是否有返回结果为1则存在
            printf("用户名已存在\n");
            // 释放结果集
            sqlite3_free_table(pazResult);
            // 关闭数据库
            sqlite3_close(db);
            // 用户存在返回1
            return 1;
        }
        // 用户名不存在就注册
        sprintf(sql, "insert into users (name, passwd) values ('%s', '%s')", data->username, data->passwd);
        if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            printf("SQL error: %s\n", errmsg);
            sqlite3_free(errmsg);
            return -1;
        }
        break;
    case 2:
        // 连表判断设备是否在线
        printf("已进入登录流程，accapt_fd:%d, %s, %s, %s, thread_id:%lu\n", data->accept_fd, data->username, data->passwd, data->devname, pthread_self());
        sprintf(sql, "select users.name, devices.device_name, devices.active from users inner join devices on users.id = devices.user_id where users.name = '%s' and devices.device_name = '%s' and devices.active = 1", data->username, data->devname);
        if (sqlite3_get_table(db, sql, &pazResult, &nRow, &nColumn, &errmsg))
        {
            printf("SQL error: %s\n", errmsg);
            sqlite3_free(errmsg);
            return -1;
        }
        if (nRow == 1)
        {
            // 通过flag判断是否有返回结果为1则存在
            printf("1.%s设备在线\n", data->devname);
            // 释放结果集
            sqlite3_free_table(pazResult);
            // 关闭数据库
            sqlite3_close(db);
            // 用户存在返回1
            return 1;
        }
        // 判断用户名密码是否正确
        sprintf(sql, "select * from users where name = '%s' and passwd = '%s'", data->username, data->passwd);
        if (sqlite3_get_table(db, sql, &pazResult, &nRow, &nColumn, &errmsg))
        {
            printf("SQL error: %s\n", errmsg);
            sqlite3_free(errmsg);
            return -1;
        }
        if (nRow != 1)
        {
            // 登录失败
            printf("登录失败\n");
            sqlite3_free_table(pazResult);
            sqlite3_close(db);
            return 2;
        }
        // 判断设备信息是否存在
        sprintf(sql, "select * from devices where device_name = '%s'", data->devname);
        if (sqlite3_get_table(db, sql, &pazResult, &nRow, &nColumn, &errmsg))
        {
            printf("SQL error: %s\n", errmsg);
            sqlite3_free(errmsg);
            return -1;
        }
        if (nRow == 1)
        {
            // 存在就更新数据
            sprintf(sql, "update devices set active = 1, thread_id = %lu, accept_fd = %d where device_name = '%s'", pthread_self(), data->accept_fd, data->devname);
            if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
            {
                printf("SQL error: %s\n", errmsg);
                sqlite3_free(errmsg);
                return -1;
            }
            
        } else {
            // 查询无结果就插入数据库
            sprintf(sql, "insert into devices (device_name, active, thread_id, accept_fd, user_id) values ('%s', 1, %lu, %d, (select id from users where name = '%s'))", data->devname, pthread_self(), data->accept_fd, data->username);
            if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
            {
                printf("SQL error: %s\n", errmsg);
                sqlite3_free(errmsg);
                return -1;
            }
        }

        break;
    case 3:
        /* code */
        break;
    default:
        break;
    }
    // 关闭数据库
    sqlite3_close(db);
    return 0;
}

