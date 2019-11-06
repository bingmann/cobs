/*******************************************************************************
 * cobs/util/thread_object_array.hpp
 *
 * Copyright (c) 2019 Timo Bingmann
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#ifndef COBS_UTIL_THREAD_OBJECT_ARRAY_HEADER
#define COBS_UTIL_THREAD_OBJECT_ARRAY_HEADER

#include <tlx/container/lru_cache.hpp>

#include <cobs/util/parallel_for.hpp>

#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace cobs {

/*!
 * LRU cache to free objects in ThreadObjectArray when they are not used often
 * enough.
 */
template <typename Type>
class ThreadObjectLRUSet
{
public:
    ThreadObjectLRUSet(size_t limit)
        : limit_(limit) { }

    tlx::LruCacheSet<std::shared_ptr<Type> > lru_set_;
    std::mutex mutex_;
    size_t limit_;

    void put(const std::shared_ptr<Type>& ptr) {
        while (lru_set_.size() + 1 > limit_) {
            // pop shared pointer and thus release reference
            lru_set_.pop();
        }
        lru_set_.put(ptr);
    }

    void touch(const std::shared_ptr<Type>& ptr) {
        lru_set_.touch(ptr);
    }
};

/*!
 * Array of objects each local to the current thread.
 */
template <typename Type>
class ThreadObjectArray
{
public:
    ThreadObjectArray(ThreadObjectLRUSet<Type>& lru_set)
        : lru_set_(lru_set) { }

    //! get the thread local object, if released via LRU, return a new one
    template <typename... Args>
    std::shared_ptr<Type> get(Args&& ... args) {
        std::unique_lock<std::mutex> lock(lru_set_.mutex_);
        std::thread::id id = std::this_thread::get_id();
        // try to acquire shared ptr from array
        std::shared_ptr<Type> p = map_[id].lock();
        if (p) {
            lru_set_.touch(p);
            return p;
        }
        // otherwise acquire a token from the LRU set and construct a new object
        p = std::make_shared<Type>(std::forward<Args...>(args) ...);
        map_[id] = p;
        lru_set_.put(p);
        return p;
    }

private:
    std::unordered_map<std::thread::id, std::weak_ptr<Type> > map_;
    ThreadObjectLRUSet<Type>& lru_set_;
};

template <typename Type>
using ThreadObjectArrayPtr = std::shared_ptr<ThreadObjectArray<std::ifstream> >;

} // namespace cobs

#endif // !COBS_UTIL_THREAD_OBJECT_ARRAY_HEADER

/******************************************************************************/
