#include "threadpool.h"

template<typename T>
Threadpool<T>::Threadpool(int thread_num) : thread_num(thread_num){
    threads = new pthread_t[thread_num];
    for (int i = 0; i < thread_num; i++) {
        pthread_create(threads+i, NULL, worker, this);
        pthread_detach(threads[i]);
    }
}

template<typename T>
Threadpool<T>::~Threadpool() {
    delete[] threads;
}

template<typename T>
bool Threadpool<T>::append(T* request, int state) {
    queuelocker.lock();
    request->state = state;
    workqueue.push_back(request);
    queuelocker.unlock();
    queuestate.post();
    return true;
}

template<typename T>
void* Threadpool<T>::worker(void *arg) {
    Threadpool *pool = (Threadpool *)arg;
    pool->run();
    return pool;
}

template<typename T>
void Threadpool<T>::run() {
    while(true) {
        // 从请求队列中获取任务资源
        queuestate.wait();
        queuelocker.lock();
        if (workqueue.empty()) {
            queuelocker.unlock();
            continue;
        }
        T *request = workqueue.front();
        workqueue.pop_front();
        queuelocker.unlock();

        if (!request)
            continue;
        
        if (0 == request->state) { // 读操作

        } else {    // 写操作

        }
    }
}