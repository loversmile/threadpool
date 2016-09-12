/****************************************************************************
 *
 * FILENAME:        $RCSfile: mid_thread_pool.cpp,v $
 *
 ***************************************************************************/

#include "mid_thread_pool.h"

#include <iostream>
#include <stdio.h>
#include <unistd.h>


using namespace std;

MidThread::MidThread(MidThreadPool* pool)
{
    task_ = NULL;
    is_destroy_ = false;
    //mutex_ = NULL;
    //cond_ = NULL;
    pool_ = pool;
    thread_id_ = 0;

    pthread_mutex_init(&mutex_, NULL);
    pthread_cond_init(&cond_, NULL);
}

void MidThread::Start()
{
    pthread_create(&thread_, NULL, &MidThread::DoTask, this);
    thread_id_ = (int)thread_;
}

MidThread::~MidThread()
{
    //cout << "~MidThread" << endl;
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&cond_);
}

void MidThread::Lock()
{
    pthread_mutex_lock(&mutex_);
}

void MidThread::Unlock()
{
    pthread_mutex_unlock(&mutex_);
}

void MidThread::AddToFreeThreadQueue()
{
    pool_->AddFreeThreadToQueue(this);
}

void MidThread::Wait()
{
    pthread_cond_wait(&cond_, &mutex_);
}

void MidThread::Join()
{
    int res = pthread_join(thread_, NULL);
    //cout << "res:  " << res << endl;
}

void MidThread::Destroy()
{
    Lock();
    is_destroy_ = true;
    Notify();
    Unlock();
    Join();
}

int MidThread::GetId()
{
    return thread_id_;
}

void* MidThread::DoTask(void* userdata)
{
    MidThread* thread = (MidThread*) userdata;
    while (true)
    {
        thread->Lock();
        if (thread->is_destroy_)
        {
            thread->Unlock();
            break;
        }
        thread->Unlock();

        MidThreadTask* task = thread->task_;
        if (task)
        {
            (*task->task_func_)(task->userdata_);
            //cout << "task finish: " << thread->GetId() << endl;
            delete task;
            thread->task_ = NULL;
        }

        //1.waitting if not destroy_
        thread->Lock();
        if (thread->is_destroy_)
        {
            thread->Unlock();
            break;
        }
        thread->AddToFreeThreadQueue();
        thread->Wait();
        thread->Unlock();
    }

    //cout << "thread finish: " << thread->GetId() << endl;
    return NULL;
}

void MidThread::Notify()
{
    pthread_cond_signal(&cond_);
}

MidThreadPool::MidThreadPool(int max_thread_number)
{
    max_thread_number_ = max_thread_number;
    is_destroy_ = false;
    is_active_ = false;
    //task_que_mutex_ = NULL;
    //free_thread_que_mutex_ = NULL;

    pthread_mutex_init(&task_que_mutex_, NULL);
    pthread_mutex_init(&free_thread_que_mutex_, NULL);

}

MidThreadPool::~MidThreadPool()
{
    //cout << "~MidThreadPool: " << endl;
    pthread_mutex_destroy(&task_que_mutex_);
    pthread_mutex_destroy(&free_thread_que_mutex_);
}

void MidThreadPool::LockFreeThreadQueue()
{
//	cout << "LockFreeThreadQueue begin: " << endl;
    pthread_mutex_lock(&free_thread_que_mutex_);
//	cout << "LockFreeThreadQueue end: " << endl;
}

void MidThreadPool::UnlockFreeThreadQueue()
{
//	cout << "UnlockFreeThreadQueue begin: " << endl;
    pthread_mutex_unlock(&free_thread_que_mutex_);
//	cout << "UnlockFreeThreadQueue end: " << endl;
}

void MidThreadPool::LockTaskQueue()
{
    pthread_mutex_lock(&task_que_mutex_);
}

void MidThreadPool::UnlockTaskQueue()
{
    pthread_mutex_unlock(&task_que_mutex_);
}

void MidThreadPool::AddFreeThreadToQueue(MidThread* thread)
{
    //1.waitting for free thread to add.
    LockFreeThreadQueue();
    free_thread_que_.push(thread);
    UnlockFreeThreadQueue();
}

void MidThreadPool::Execute()
{
    //	cout << "MidThreadPool::Execute  begin" << endl;
    LockFreeThreadQueue();
    //1.pop free task fro queue, and run task
    if (!free_thread_que_.empty())
    {
        LockTaskQueue();
        if (!task_que_.empty())
        {
            MidThreadTask* task = task_que_.front();
            task_que_.pop();

            //1.set task
            MidThread* free_thread = free_thread_que_.front();
            free_thread_que_.pop();
            free_thread->task_ = task;
            //2.notify to start thread
            free_thread->Notify();
        }
        UnlockTaskQueue();
    }
    UnlockFreeThreadQueue();
//	cout << "MidThreadPool::Execute  end" << endl;
}

void* MidThreadPool::ScanTask(void* userdata)
{
    MidThreadPool* pool = (MidThreadPool*) userdata;
    while (true)
    {
        if (pool->is_destroy_)
        {
            break;
        }
        pool->Execute();
        usleep(1000);
    }

    //cout << "MidThreadPool::ScanTask end" << endl;
    return NULL;
}

void MidThreadPool::Activate()
{
    //1.create threads
    for (int i = 0; i < max_thread_number_; ++i)
    {
        MidThread* thread = new MidThread(this);
        threads_.push_back(thread);
        thread->Start();
    }
    is_active_ = true;
    //1.begin scan threas
    pthread_create(&task_que_thread_, NULL, &ScanTask, this);
}

void MidThreadPool::Destroy()
{
    //1.wait for all the threads stop
    //cout << "MidThreadPool::Destroy begin" << endl;

    //1.stop scan threads
    is_destroy_ = true;
    is_active_ = false;
    pthread_join(task_que_thread_, NULL);

    //1.stop work threads
    size_t size = threads_.size();
    for (size_t i = 0; i < size; ++i)
    {
        MidThread* thread = threads_[i];
        thread->Destroy();
        //cout << "thread->Destroy()" << endl;
        delete thread;
    }

    size_t remain = task_que_.size();
    //cout << "remain task: " << remain << endl;
    for (size_t i = 0; i < remain; ++i)
    {
        MidThreadTask* task = task_que_.front();
        task_que_.pop();
        delete task;
    }

    //cout << "MidThreadPool::Destroy end " << endl;
}

void MidThreadPool::AddAsynTask(TaskFunc task_func, void* userdata)
{
    MidThreadTask *task = new MidThreadTask(task_func, userdata);

    LockTaskQueue();
    //1.task queue
    task_que_.push(task);

    UnlockTaskQueue();
}