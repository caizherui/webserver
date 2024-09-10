#include "config/config.h"

int main(int argc, char *argv[]) {
    Config config;

    WebServer server;

    //初始化
    server.init(config.PORT, config.user, config.passwd, config.databasename,
                config.OPT_LINGER, config.TRIGMode,  config.sql_num,  config.thread_num, 
                config.actor_model);

    //数据库
    server.sql_pool();

    //线程池
    server.thread_pool();

    //触发模式
    server.trig_mode();

    //监听
    server.eventListen();

    //运行
    server.eventLoop();

    return 0;
}