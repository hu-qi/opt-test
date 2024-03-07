/**
 * ============================================================================
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2022. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1 Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *   2 Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   3 Neither the names of the copyright holders nor the names of the
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ============================================================================
 */

#ifndef QUEUE_H
#define QUEUE_H
#include <mutex>
#include <queue>

namespace acllite {
template<typename T>
class Queue {
public:
    Queue(uint32_t capacity)
    {
        // check the input value: capacity is valid
        if (capacity >= kMinQueueCapacity && capacity <= kMaxQueueCapacity) {
            queueCapacity = capacity;
        } else { // the input value: capacity is invalid, set the default value
            queueCapacity = kDefaultQueueCapacity;
        }
    }
    
    Queue()
    {
        queueCapacity = kDefaultQueueCapacity;
    }

    ~Queue() = default;

    bool Push(T input_value)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // check current size is less than capacity
        if (queue_.size() < queueCapacity) {
            queue_.push(input_value);
            return true;
        }

        return false;
    }

    T Pop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        T tmp_ptr = queue_.front();
        queue_.pop();
        return tmp_ptr;
    }

    bool Empty()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    uint32_t Size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    void ExtendCapacity(uint32_t newSize)
    {
        queueCapacity = newSize;
    }

private:
    std::queue<T> queue_; // the queue
    uint32_t queueCapacity; // queue capacity
    mutable std::mutex mutex_; // the mutex value
    const uint32_t kMinQueueCapacity = 1; // the minimum queue capacity
    const uint32_t kMaxQueueCapacity = 10000; // the maximum queue capacity
    const uint32_t kDefaultQueueCapacity = 10; // default queue capacity
};
}  // namespace acllite
#endif /* QUEUE_H */