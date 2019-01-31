#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <memory>
#include <utility>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <algorithm>

class Object;

class Policy {
  public:
    virtual void adjust (std::shared_ptr<Object>) = 0;
};

class Object {
  public:

    virtual void process() = 0;

    inline void setPriority (float p) {
        m_fPriority = p;
    }

    inline float getPriority() const {
        return m_fPriority;
    }

    bool operator < (Object& obj) {
        return m_fPriority < obj.getPriority();
    }

  private:
    float m_fPriority;
};

class ThreadPool {
  public:
    ThreadPool() = delete;
    ThreadPool (ThreadPool&) = delete;
    ThreadPool (ThreadPool&&) = delete;
    ~ThreadPool();

    static ThreadPool* getInstance (int threadNum = 4);

    void run();
    void pause();
    void destroy();
    void consume();
    void updatePriority();
    void setPolicy (std::shared_ptr<Policy>);
    void addTask (std::shared_ptr<Object>);

  private:
    ThreadPool (int threadNum);

    std::vector<std::thread> m_vecThreads;
    std::vector<std::shared_ptr<Object>> m_vecTasks;

    std::shared_ptr<Policy> m_pPolicy;
    std::atomic_bool m_bStop;  // 终止整个线程池
    std::atomic_bool m_bPause; // 暂停线程池执行

    std::mutex m_mtxQue;
    std::condition_variable m_condVar;

    int m_iThreadNum {0};

    static ThreadPool* m_pInstance;
};

ThreadPool* ThreadPool::m_pInstance = nullptr;

ThreadPool::ThreadPool (int threadNum) {
    m_iThreadNum = threadNum;
    m_bStop = true;
    m_bPause = true;
    m_vecTasks.clear();
    m_vecThreads.clear();
}

ThreadPool* ThreadPool::getInstance (int threadNum) {
    if (m_pInstance == nullptr) {
        m_pInstance = new ThreadPool (threadNum);
    }
    return m_pInstance;
}

void ThreadPool::run() {
    m_bStop = false;
    m_bPause = false;
    for (int i = 0; i < m_iThreadNum; ++i) {
        m_vecThreads.emplace_back ([this]() {
            for (;;) {
                std::shared_ptr<Object> ptr;
                {
                    std::unique_lock<std::mutex> lock (m_mtxQue);
                    m_condVar.wait (lock, [this]()->bool {
                        return ( (!m_vecTasks.empty() && !m_bPause) || m_bStop);
                    });

                    if (m_bStop) {
                        return;
                    }

                    std::pop_heap (m_vecTasks.begin(), m_vecTasks.end() );
                    ptr = * (m_vecTasks.end() - 1);
                    m_vecTasks.pop_back();
                }
                ptr->process();
            }
        });
    }
}

void ThreadPool::pause() {
    m_bPause = true;
}

void ThreadPool::consume() {
    m_bPause = false;
    m_condVar.notify_all();
}

void ThreadPool::destroy() {
    m_bPause = false;
    m_bStop = true;
    m_condVar.notify_all();
    for (auto& it : m_vecThreads) {
        it.join();
    }
    m_vecTasks.clear();
    m_vecThreads.clear();
}

void ThreadPool::updatePriority() {
    std::unique_lock<std::mutex> lock (m_mtxQue);
    m_bPause = true;
    for (auto& it : m_vecTasks) {
        m_pPolicy->adjust (it);
    }
    m_bPause = false;
}

void ThreadPool::setPolicy (std::shared_ptr<Policy> policy) {
    m_pPolicy = policy;
}

void ThreadPool::addTask (std::shared_ptr<Object> task) {
    std::unique_lock<std::mutex> lock (m_mtxQue);
    m_vecTasks.emplace_back (task);
    std::push_heap (m_vecTasks.begin(), m_vecTasks.end() );
}

#endif // THREADPOOL_H
