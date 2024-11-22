#include "head.h"

char* errmsg = NULL;

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
    // case 1为注册，2为登录，3为踢下线更新，4为客户端断线更新，5为转发，6server断掉后再次启动需要将设备状态归零
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
        sprintf(sql, "select devices.active, devices.thread_id, devices.accept_fd from users inner join devices on users.id = devices.user_id where users.name = '%s' and devices.device_name = '%s' and devices.active = 1", data->username, data->devname);
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
            // 更新数据信息结构体
            data->active = atoi(pazResult[3]);
            data->tid = atol(pazResult[4]);
            data->accept_fd = atoi(pazResult[5]);
            printf("在线设备accept_fd:%d, thread_id:%lu\n", data->accept_fd, data->tid);
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
            // 登录失败返回2
            printf("登录失败\n");
            sqlite3_free_table(pazResult);
            sqlite3_close(db);
            return 2;
        }
        // 判断设备信息是否存在
        sprintf(sql, "select * from devices where device_name = '%s' and user_id = (select id from users where name = '%s')", data->devname, data->username);
        if (sqlite3_get_table(db, sql, &pazResult, &nRow, &nColumn, &errmsg))
        {
            printf("SQL error: %s\n", errmsg);
            sqlite3_free(errmsg);
            return -1;
        }
        if (nRow == 1)
        {
            // 存在就更新数据
            sprintf(sql, "update devices set active = 1, thread_id = %lu, accept_fd = %d where device_name = '%s' and user_id = (select id from users where name = '%s')", data->tid, data->accept_fd, data->devname, data->username);
            if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
            {
                printf("SQL error: %s\n", errmsg);
                sqlite3_free(errmsg);
                return -1;
            }
            
        } else {
            // 不存在就插入数据库
            sprintf(sql, "insert into devices (device_name, active, thread_id, accept_fd, user_id) values ('%s', 1, %lu, %d, (select id from users where name = '%s'))", data->devname, data->tid, data->accept_fd, data->username);
            if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
            {
                printf("SQL error: %s\n", errmsg);
                sqlite3_free(errmsg);
                return -1;
            }
        }

        break;
    case 3:
        // 踢下线更新设备的状态
        sprintf(sql, "update devices set active = 0, accept_fd = 0, thread_id = 0 where device_name = '%s' and user_id = (select id from users where name = '%s')", data->devname, data->username);
        if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            printf("SQL error: %s\n", errmsg);
            sqlite3_free(errmsg);
            return -1;
        }
        printf("更新设备状态成功1\n");
        break;
    case 4:
        // 客户端断线
        sprintf(sql, "update devices set active = 0, accept_fd = 0, thread_id = 0 where accept_fd = %d and thread_id = %lu", data->accept_fd, data->tid);
        if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            printf("SQL error: %s\n", errmsg);
            sqlite3_free(errmsg);
            return -1;
        }
        printf("更新设备状态成功2\n");
        break;
    case 5:
        // 转发数据
        break;
    case 6:
        // server服务非正常退出后重启server需将devices表中active归零
        sprintf(sql, "update devices set active = 0 where active = 1");
        if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            printf("SQL error: %s\n", errmsg);
            sqlite3_free(errmsg);
            return -1;
        } else {
            printf("初始化设备状态成功3\n");
        }
        break;
    default:
        break;
    }
    // 关闭数据库
    sqlite3_close(db);
    return 0;
}
