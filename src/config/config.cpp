#include "config.h"

Config::Config(){
    //端口号,默认9006
    PORT = 9010;

    //日志写入方式，默认同步
    LOGWrite = 0;

    //触发组合模式,默认listenfd LT + connfd LT
    TRIGMode = 0;

    //listenfd触发模式，默认LT
    LISTENTrigmode = 0;

    //connfd触发模式，默认LT
    CONNTrigmode = 0;

    //优雅关闭链接，默认不使用
    OPT_LINGER = 0;

    //数据库连接池数量,默认8
    sql_num = 8;

    //线程池内的线程数量,默认8
    thread_num = 8;

    //并发模型,默认是proactor
    actor_model = 0;

    user = "debian-sys-maint";

    passwd = "H1Cx0z2Vn8N0hvqo";

    databasename = "yourdb";
}