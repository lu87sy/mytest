#include <stdio.h>
#include <string.h>
#include <time.h>


int main() {
    // char str[] = "toServ:action=log,usrname=xiaowang,passwd=123456,devname=sml001;";
    char str[] = "devname=sml001,led=on,beep=on,oled=hello world,alarm=yes;";
    char *token;
    char *saveptr;


    char input[] = "toServ:action=log,usrname=xiaowang,passwd=123456,devname=sml02;";
    // char input[] = "devname=sml001,led=on,beep=on,action=hello world,alarm=yes;";
    char action[50];

    // 使用 sscanf 提取字符串中的各个部分
    // sscanf(input, "%[^:]:action=%[^,],usrname=%[^,],passwd=%[^,],devname=%[^;];",
        //    toServ, action, usrname, passwd, devname);"%*[^p]passwd=%19[^,]"
    // sscanf(input, "%*[^l]led=%49[^,]", action);
    sscanf(input, "%[^:]", action);


    // 打印提取的字符串
    // printf("toServ: %s\n", toServ);
    printf("action: %s\n", action);
    // printf("usrname: %s\n", usrname);
    // printf("passwd: %s\n", passwd);
    // printf("devname: %s\n", devname);






    // 使用 strtok_r
    char * tmp;
    token = strtok_r(str, ";", &saveptr);
    // tmp = token;
    token = strtok_r(token, ":", &saveptr);
    // while (token != NULL) {
        printf("%s\n", token);
        token = strtok_r(token, ",", &saveptr+1);
        token = strtok_r(token, "=", &saveptr);
        // token = strtok_r(NULL, ",", &saveptr);
        printf("%s\n", token);
    // }

    time_t current_time;
        
    // 获取当前时间
    time(&current_time);
    
    // 将时间转换为本地时间并格式化
    struct tm *local_time = localtime(&current_time);
    
    // 打印当前时间toCli:Ur MSG GOT!time=2021-12-15 19:11:31;
    printf("当前时间: %04d-%02d-%02d %02d:%02d:%02d\n",
        local_time->tm_year + 1900,
        local_time->tm_mon + 1,
        local_time->tm_mday,
        local_time->tm_hour,
        local_time->tm_min,
        local_time->tm_sec);
    return 0;
}