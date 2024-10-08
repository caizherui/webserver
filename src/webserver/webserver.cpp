#include "webserver.h"

WebServer::WebServer() {
    //http_conn类对象
    users = new http_conn[MAX_FD];

    //root文件夹路径
    char server_path[200];
    getcwd(server_path, sizeof(server_path));
    char root[6] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcat(m_root, root);

    //定时器
    users_timer = new client_data[MAX_FD];
}

WebServer::~WebServer() {
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[] users;
    delete[] users_timer;
    delete m_pool;
}

void WebServer::init(int port, string user, string passWord, string databaseName, 
                     int opt_linger, int trigmode, int sql_num, int thread_num, int actor_model) {
    m_port = port;
    m_user = user;
    m_passWord = passWord;
    m_databaseName = databaseName;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_OPT_LINGER = opt_linger;
    m_TRIGMode = trigmode;
    m_actormodel = actor_model;
}

void WebServer::trig_mode() {
    // m_Listen：监听套接字
    // m_Conn: 连接套接字
    //LT + LT
    if (0 == m_TRIGMode) {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 0;
    } else if (1 == m_TRIGMode) { //LT + ET
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 1;
    } else if (2 == m_TRIGMode) { //ET + LT
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 0;
    } else if (3 == m_TRIGMode) { //ET + ET
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 1;
    }
}

void WebServer::sql_pool() {
    //初始化数据库连接池
    m_connPool = connection_pool::GetInstance();
    m_connPool->init("localhost", m_user, m_passWord, m_databaseName, 3306, m_sql_num);

    //初始化数据库读取表
    users->initmysql_result(m_connPool);
}

void WebServer::thread_pool() {
    //线程池
    m_pool = new threadpool<http_conn>(m_actormodel, m_connPool, m_thread_num, 100);
}

void WebServer::eventListen() {
    //网络编程基础步骤
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);

    // 等待一段时间后，再关闭套接字
    struct linger tmp = {1, 1};
    setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    // 绑定ip地址和端口号
    bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
    // 被动监听模式
    listen(m_listenfd, 5);

    //epoll创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER];
    m_epollfd = epoll_create(5);
    http_conn::m_epollfd = m_epollfd;


    utils.init(TIMESLOT);
    utils.addfd(m_epollfd, m_listenfd, false, m_LISTENTrigmode);

    socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    utils.setnonblocking(m_pipefd[1]);
    utils.addfd(m_epollfd, m_pipefd[0], false, 0);

    utils.addsig(SIGPIPE, SIG_IGN);
    utils.addsig(SIGALRM, utils.sig_handler, false);
    utils.addsig(SIGTERM, utils.sig_handler, false);

    alarm(TIMESLOT);

    //工具类,信号和描述符基础操作
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}

void WebServer::timer(int connfd, struct sockaddr_in client_address) {
    //初始化client_data数据
    //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;
    util_timer *timer = new util_timer;
    timer->user_data = &users_timer[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    users_timer[connfd].timer = timer;
    utils.m_timer_lst.add_timer(timer);
}

//若有数据传输，则将定时器往后延迟3个单位
//并对新的定时器在链表上的位置进行调整
void WebServer::adjust_timer(util_timer *timer) {
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    utils.m_timer_lst.adjust_timer(timer);
}

void WebServer::deal_timer(util_timer *timer, int sockfd) {
    timer->cb_func(&users_timer[sockfd]);
    utils.m_timer_lst.del_timer(timer);
}

// 处理客户端新的连接请求
bool WebServer::dealclientdata() {
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);

    // LT模式，无限制次数触发，所以不需要一定完成
    if (0 == m_LISTENTrigmode) {
        // 连接套接字加入epoll内核事件表
        int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
        users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode, m_user, m_passWord, m_databaseName);

        // 加入定时器
        timer(connfd, client_address);
    } else { // ET模式，必须完成
        while (1) {
            int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
            users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode, m_user, m_passWord, m_databaseName);

            timer(connfd, client_address);
        }
        return false;
    }
    return true;
}

bool WebServer::dealwithsignal(bool &timeout, bool &stop_server) {
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if (ret == -1) {
        return false;
    } else if (ret == 0) {
        return false;
    } else {
        for (int i = 0; i < ret; ++i) {
            switch (signals[i]) {
                case SIGALRM: {
                    timeout = true;
                    break;
                }
                case SIGTERM: {
                    stop_server = true;
                    break;
                }
            }
        }
    }
    return true;
}

void WebServer::dealwithread(int sockfd) {
    util_timer *timer = users_timer[sockfd].timer;

    //reactor
    if (1 == m_actormodel) {
        adjust_timer(timer);

        //若监测到读事件，将该事件放入请求队列
        m_pool->append(users + sockfd, 0);

        while (true) {
            if (1 == users[sockfd].improv) {
                if (1 == users[sockfd].timer_flag) {
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    } else { //proactor
        if (users[sockfd].read_once()) {
            inet_ntoa(users[sockfd].get_address()->sin_addr);

            //若监测到读事件，将该事件放入请求队列
            m_pool->append_p(users + sockfd);
            adjust_timer(timer);
        }
        else {
            deal_timer(timer, sockfd);
        }
    }
}

void WebServer::dealwithwrite(int sockfd) {
    util_timer *timer = users_timer[sockfd].timer;
    //reactor
    if (1 == m_actormodel) {
        adjust_timer(timer);
        m_pool->append(users + sockfd, 1);

        while (true) {
            if (1 == users[sockfd].improv) {
                if (1 == users[sockfd].timer_flag) {
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    else {
        //proactor
        if (users[sockfd].write()) {
            inet_ntoa(users[sockfd].get_address()->sin_addr);
            adjust_timer(timer);
        }
        else {
            deal_timer(timer, sockfd);
        }
    }
}

void WebServer::eventLoop() {
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server) {
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);

        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;

            //处理新到的客户连接
            if (sockfd == m_listenfd) {
                bool flag = dealclientdata();
                if (false == flag)
                    continue;
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                //服务器端关闭连接，移除对应的定时器
                util_timer *timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd);
            }
            //处理信号
            else if ((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN))
                bool flag = dealwithsignal(timeout, stop_server);
            //处理客户连接上接收到的数据
            else if (events[i].events & EPOLLIN)
                dealwithread(sockfd);
            else if (events[i].events & EPOLLOUT)
                dealwithwrite(sockfd);
        }
        if (timeout) {
            utils.timer_handler();
            timeout = false;
        }
    }
}
