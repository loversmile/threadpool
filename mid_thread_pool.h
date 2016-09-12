/****************************************************************************
 *
 * FILENAME:        $RCSfile: mid_thread_pool.h,v $
 *
 *
 ***************************************************************************/

#ifndef MID_THREAD_POOL_H_
#define MID_THREAD_POOL_H_

#include <vector>
#include <queue>
#include "pthread.h"
#include "cgilog.h"


typedef void * (*TaskFunc)(void * arg);

class MidThreadTask
{
public:
    MidThreadTask(TaskFunc task_func, void* userdata)
    {
    userdata_ = userdata;
    task_func_ = task_func;
    }
    ~MidThreadTask()
    {
    }
    void* userdata_;
    TaskFunc task_func_;
};

class MidThreadPool;
class MidThread;

class MidThread
{
public:
    MidThread(MidThreadPool* pool);
    ~MidThread();

    MidThreadTask* task_;
    void AddToFreeThreadQueue();
    void Notify();

    void Lock();
    void Unlock();
    void Wait();
    void Join();
    int GetId();
    void Destroy();

    void Start();

private:
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;

    bool is_destroy_;
    pthread_t thread_;
    MidThreadPool* pool_; //1.pool of the thread
    static void* DoTask(void* userdata);

    int thread_id_;
};

class MidThreadPool
{
public:
    MidThreadPool(int max_thread_number);
    ~MidThreadPool();
    void AddAsynTask(TaskFunc task_func, void* userdata);
    void Activate(); //1.active threadpool and scan task
    void Destroy(); //1.destory thread pool

    void Execute();
    void AddFreeThreadToQueue(MidThread* thread);
    bool is_destroy_; //1.if true, destory thread pool
    bool is_active_;

private:
    static void* ScanTask(void* userdata);

    void LockTaskQueue();
    void UnlockTaskQueue();

    void LockFreeThreadQueue();
    void UnlockFreeThreadQueue();

    int max_thread_number_; //1.max num of run threads

    std::vector<MidThread*> threads_; //1.threads vector
    std::queue<MidThread*> free_thread_que_; //1.free threads
    pthread_mutex_t free_thread_que_mutex_;

    std::queue<MidThreadTask*> task_que_;
    pthread_mutex_t task_que_mutex_;
    pthread_t task_que_thread_;
};

#endif /* MID_THREAD_POOL_H_ */