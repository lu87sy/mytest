#include "head.h"

// 子线程处理函数
void *thread_func(void *arg)
{
    // 获取传参
    int accept_fd = ((struct Client_data *)arg) -> accept_fd;
    struct sockaddr_in client_addr = ((struct Client_data *)arg) -> client_addr;
    char buf[1024] = "";
    ssize_t res = 0;
    struct data_info data;
    struct data_info back_data;
    data.accept_fd = accept_fd;
    data.active = 0;    // 初始化设备状态
    data.tid = pthread_self();

    // char * tmp, *saveptr;
    char value[20];
    // 打印子线程id
    printf("子线程id:%lu\n", pthread_self());

    while (1)
    {
        printf("第二层\n");
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
            printf("-----设备名称%s-------\n", data.devname);   // 此时还未登录无登录设备信息
            printf("-----套接字%d-------\n", data.accept_fd);
            printf("-----线程id%ld---------\n", data.tid);
            actionDB(4, &data);
            // 关闭连接
            close(accept_fd);
            pthread_exit(NULL);
            break;
        }
        printf("客户端[%s:%d]发送了数据:%s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buf);
        
        // 校验数据格式     
        if (strchr(buf, ';'))
        {
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
                    if (strcmp(data.username, "") == 0 || strcmp(data.passwd, "") == 0)     // 判断传参是否合法
                    {
                        if (-1 == send(accept_fd, "toCli:reg failed,username and password cannot be empty;" , strlen("toCli:reg failed,username and password cannot be empty;"), 0))
                        {
                            perror("send error");
                            break;
                        }
                    } else{
                        int res = actionDB(1, &data);
                        if (res == 1)   // 注册失败,账号已存在
                        {
                            if (-1 == send(accept_fd, "toCli:reg failed,username already exists;" , strlen("toCli:reg failed,username already exists;"), 0))
                            {
                                perror("send error");
                                break;
                            }
                        } else {        // 注册成功
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
                    sscanf(buf, "%*[^,],%*[^,],%*[^,],devname=%19[^;]", data.devname);
                    printf("username:%s, passwd:%s, devname:%s\n", data.username, data.passwd, data.devname);
                    if (strcmp(data.username, "") == 0 || strcmp(data.passwd, "") == 0 || strcmp(data.devname, "") == 0)        // 判断传参是否合法
                    {
                        if (-1 == send(accept_fd, "toCli:log failed,username,password and devname cannot be empty;" , strlen("toCli:log failed,username,password and devname cannot be empty;"), 0))
                        {
                            perror("send error");
                            break;
                        }
                    } else {
                        back_data = data;
                        int res = actionDB(2, &back_data);       // 登录数据库查询
                        if (res == 1)
                        {
                            printf("2.%s设备在线\n", back_data.devname);
                            printf("accpet_fd:%d, devname:%s, thread_id:%lu\n", back_data.accept_fd, back_data.devname, back_data.tid);
                            // 更新设备状态
                            printf("更新设备状态\n");
                            if (-1 == send(back_data.accept_fd, "toCli:your device is logged in elsewhere;" , strlen("toCli:your device is logged in elsewhere;"), 0))
                            {
                                perror("send error");
                                break;
                            }

                            actionDB(3, &back_data);
                            // 踢客户端下线
                            printf("踢下线---%d--\n", back_data.accept_fd);
                            close(back_data.accept_fd);
                            pthread_cancel(back_data.tid);
                        
                            // 用户名密码错误
                        } else if (res == 2) {
                            if (-1 == send(accept_fd, "toCli:log failed,username or password is wrong;" , strlen("toCli:log failed,username or password is wrong;"), 0))
                            {
                                perror("send error");
                                break;
                            }
                        } else {        // 登录成功
                            if (-1 == send(accept_fd, "toCli:log success;" , strlen("toCli:log success;"), 0))
                            {
                                perror("send error");
                                break;
                            }
                            printf("登录成功\n");
                            while (1)
                            {
                                printf("推送子线程\n");
                                memset(buf, 0, sizeof(buf));
                                // 接收推送数据
                                res = recv(accept_fd, buf, sizeof(buf), 0);
                                if (-1 == res)
                                {
                                    perror("recv error");
                                    break;
                                } else if (0 == res)
                                {
                                    printf("-----设备名称%s-------\n", data.devname);   // 已登录有设备信息
                                    printf("-----套接字%d-------\n", data.accept_fd);
                                    printf("-----线程id%ld---------\n", data.tid);
                                    actionDB(4, &data);
                                    // 关闭连接
                                    close(accept_fd);
                                    pthread_exit(NULL);
                                    break;
                                }

                                // 校验推送数据格式toDev:devname=sml001,type=msg,data=hello world; toServ:action=log,usrname=xiaoma,passwd=021696,devname=sml001;
                                if (strchr(buf, ';'))
                                {
                                    sscanf(buf, "%[^:]",value);
                                    // 处理推送数据
                                    if (strcmp(value, "toDev") == 0)
                                    {
                                        printf("这是推送数据%s\n", value);
                                        // 判断未登录提示登录
                                        sscanf(buf, "toDev:devname=%19[^,]", data.devname);
                                        sscanf(buf, "toDev:%*[^,],type=%19[^,]", data.type);
                                        sscanf(buf, "toDev:%*[^,],%*[^,],data=%511[^;]", data.data);
                                        printf("devname:%s, type:%s, data:%s\n", data.devname, data.type, data.data);
                                        if (strcmp(data.devname, "") == 0 || strcmp(data.type, "") == 0 || strcmp(data.data, "") == 0)
                                        {
                                            if (-1 == send(accept_fd, "toCli:push failed,devname,type and data cannot be empty;" , strlen("toCli:push failed,devname,type and data cannot be empty;"), 0))
                                            {
                                                perror("send error");
                                                break;
                                            }
                                            
                                        } else {
                                            // 数据库查询设备状态
                                            // int res = actionDB(4, &data);
                                            printf("已登录直接推送数据\n");
                                        }
                                    } else if (1)
                                    {

                                    } else {
                                        printf("数据格式错误\n");
                                        // 格式中无:，发送错误信息
                                        if (-1 == send(accept_fd, "toCli:cmd invalid,missing colon;", strlen("toCli:cmd invalid,missing colon;"), 0))
                                        {
                                            perror("send error");
                                            break;
                                        }
                                    }
                                } else {
                                    printf("数据格式错误\n");
                                    // 没有以;结尾，发送错误信息
                                    if (-1 == send(accept_fd, "toCli:cmd invalid,missing semicolon;", strlen("toCli:cmd invalid,missing semicolon;"), 0))
                                    {
                                        perror("send error");
                                        break;
                                    }
                                }
                                
                            }
                            
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
            } // 直接发送toDev
            else if (strcmp(value, "toDev") == 0)
            {
                printf("未登录情况\n");
                // 未登录直接发送toDev情况下提示登录
                if (-1 == send(accept_fd, "toCli:push failed,please login first;" , strlen("toCli:push failed,please login first;"), 0))
                {
                    perror("send error");
                    break;
                }
            } else {
                printf("数据格式错误\n");
                // 格式中无:，发送错误信息
                if (-1 == send(accept_fd, "toCli:cmd invalid,missing colon;", strlen("toCli:cmd invalid,missing colon;"), 0))
                {
                    perror("send error");
                    break;
                }
            }
            
        } else {
            printf("数据格式错误\n");
            // 没有以;结尾，发送错误信息
            if (-1 == send(accept_fd, "toCli:cmd invalid,missing semicolon;", strlen("toCli:cmd invalid,missing semicolon;"), 0))
            {
                perror("send error");
                break;
            }
        }
    }
    // 关闭连接
    close(accept_fd);
    // 释放线程
    pthread_exit(NULL);
}
