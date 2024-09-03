# include "lock.h"

Locker::Locker() {
    pthread_mutex_init(&m_mutex, NULL);
}

Locker::~Locker() {
    pthread_mutex_destroy(&m_mutex);
}

bool Locker::lock() {
    return pthread_mutex_lock(&m_mutex) == 0;
}

bool Locker::unlock() {
    return pthread_mutex_unlock(&m_mutex) == 0;
}

pthread_mutex_t* Locker::get() {
    return &m_mutex;
}

Sem::Sem() {
    sem_init(&m_sem, 0, 0);
}

Sem::Sem(int num) {
    sem_init(&m_sem, 0, num);
}

Sem::~Sem() {
    sem_destroy(&m_sem);
}

bool Sem::wait() {
    return sem_wait(&m_sem) == 0;
}

bool Sem::post() {
    return sem_post(&m_sem) == 0;
}


