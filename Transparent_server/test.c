#include <stdio.h>
#include <string.h>
#include <time.h>


int main() {
    // char str[] = "toServ:action=log,usrname=xiaowang,passwd=123456,devname=sml001;";
    char str[] = "devname=sml001,led=on,beep=on,oled=hello world,alarm=yes;";
    char *token;
    char *saveptr;


    char input[] = "toServ:actiuoan=log,usrname=xiaowang,passwd=123456,devname=sml02;";
    char devname[50];

    sscanf(input, "%*[^,],%*[^,],%*[^,],devname=%49[^;]", devname);

    printf("devname: %s\n", devname);

    return 0;
}