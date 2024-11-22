#include <stdio.h>
#include <string.h>
#include <time.h>


int main() {
    // char str[] = "toServ:action=log,usrname=xiaowang,passwd=123456,devname=sml001;";
    char str[] = "devname=sml001,led=on,beep=on,oled=hello world,alarm=yes;";
    char *token;
    char *saveptr;


    char input[] = "toDev:devname=sml001,type=msg,data=hello world;";
    char devname[50];

    sscanf(input, "toDev:%*[^,],%*[^,],data=%49[^;]", devname);

    printf("devname: %s\n", devname);

    return 0;
}

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
                    int res = actionDB(4, &data);
                }