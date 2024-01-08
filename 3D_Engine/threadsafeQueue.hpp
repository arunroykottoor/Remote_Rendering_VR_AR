#pragma once

#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <condition_variable>
#include <mutex>
#include <queue>



// A threadsafe-queue.
template <class T>
class SafeQueue
{
public:
    SafeQueue(int size_in) : q(), m(), c(), size(size_in) {}

    ~SafeQueue() {}

    // Add an element to the queue.
    //bool enqueue(T t)
    //{
    //    bool flag = false;
    //    std::lock_guard<std::mutex> lock(m);
    //    if(q.size() < size)
    //    {
    //        q.push(t);
    //        flag = true;
    //    }
    //    else
    //    {
    //        //q.pop();
    //        //q.push(t);
    //    }
    //    
    //    c.notify_one();
    //    return flag;
    //}
    bool enqueue(T t)
    {
        bool flag = true;
        std::lock_guard<std::mutex> lock(m);
        if (q.size() == size)
        {
            q.pop();
            flag = false;
        }

        q.push(std::move(t)); // Use std::move for efficiency
        c.notify_one();
        return flag;
    }

    // Get the front element.
    // If the queue is empty, wait till a element is avaiable.
    //T dequeue(void)
    //{
    //    std::unique_lock<std::mutex> lock(m);
    //    while (q.empty())
    //    {
    //        // release lock as long as the wait and reaquire it afterwards.
    //        c.wait(lock);
    //    }
    //    T val = q.front();
    //    q.pop();
    //    return val;
    //}
    T dequeue(void)
    {
        std::unique_lock<std::mutex> lock(m);
        c.wait(lock, [&] { return !q.empty(); }); // Use wait with predicate

        T val = std::move(q.front()); // Use std::move for efficiency
        q.pop();

        return val;
    }

private:
    std::queue<T> q;
    mutable std::mutex m;
    std::condition_variable c;
    int size;
};

#endif