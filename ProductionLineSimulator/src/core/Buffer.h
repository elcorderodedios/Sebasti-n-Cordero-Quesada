#ifndef BUFFER_H
#define BUFFER_H

#include <QQueue>
#include <QMutex>
#include <QSemaphore>
#include <QWaitCondition>
#include <memory>

template<typename T>
class Buffer
{
public:
    explicit Buffer(int capacity = 10)
        : m_capacity(capacity)
        , m_spacesAvailable(capacity)
        , m_itemsAvailable(0)
        , m_stopping(false)
    {
    }
    
    ~Buffer() {
        stop();
    }
    
    // Producer operation - blocks if buffer is full
    bool push(const T& item) {
        if (m_stopping) return false;
        
        // Wait for space
        if (!m_spacesAvailable.tryAcquire(1, 5000)) {
            return false; // Timeout or stopping
        }
        
        {
            QMutexLocker locker(&m_mutex);
            if (m_stopping) {
                m_spacesAvailable.release(1);
                return false;
            }
            m_queue.enqueue(item);
        }
        
        // Signal that an item is available
        m_itemsAvailable.release(1);
        return true;
    }
    
    // Consumer operation - blocks if buffer is empty
    bool pop(T& item) {
        if (m_stopping) return false;
        
        // Wait for item
        if (!m_itemsAvailable.tryAcquire(1, 5000)) {
            return false; // Timeout or stopping
        }
        
        {
            QMutexLocker locker(&m_mutex);
            if (m_stopping) {
                m_itemsAvailable.release(1);
                return false;
            }
            if (!m_queue.isEmpty()) {
                item = m_queue.dequeue();
            }
        }
        
        // Signal that a space is available
        m_spacesAvailable.release(1);
        return true;
    }
    
    // Non-blocking operations
    bool tryPush(const T& item) {
        if (m_stopping) return false;
        
        if (!m_spacesAvailable.tryAcquire(1)) {
            return false;
        }
        
        {
            QMutexLocker locker(&m_mutex);
            if (m_stopping) {
                m_spacesAvailable.release(1);
                return false;
            }
            m_queue.enqueue(item);
        }
        
        m_itemsAvailable.release(1);
        return true;
    }
    
    bool tryPop(T& item) {
        if (m_stopping) return false;
        
        if (!m_itemsAvailable.tryAcquire(1)) {
            return false;
        }
        
        {
            QMutexLocker locker(&m_mutex);
            if (m_stopping) {
                m_itemsAvailable.release(1);
                return false;
            }
            if (!m_queue.isEmpty()) {
                item = m_queue.dequeue();
            }
        }
        
        m_spacesAvailable.release(1);
        return true;
    }
    
    // Status queries
    int size() const {
        QMutexLocker locker(&m_mutex);
        return m_queue.size();
    }
    
    int capacity() const {
        return m_capacity;
    }
    
    bool isEmpty() const {
        QMutexLocker locker(&m_mutex);
        return m_queue.isEmpty();
    }
    
    bool isFull() const {
        QMutexLocker locker(&m_mutex);
        return m_queue.size() >= m_capacity;
    }
    
    // Lifecycle
    void stop() {
        m_stopping = true;
        // Release all waiting threads
        m_spacesAvailable.release(m_capacity);
        m_itemsAvailable.release(m_capacity);
    }
    
    void clear() {
        QMutexLocker locker(&m_mutex);
        m_queue.clear();
        // Reset semaphores
        m_spacesAvailable.release(m_capacity - m_spacesAvailable.available());
        while (m_itemsAvailable.tryAcquire(1, 0)) {
            // Drain items semaphore
        }
    }

private:
    QQueue<T> m_queue;
    mutable QMutex m_mutex;
    QSemaphore m_spacesAvailable;
    QSemaphore m_itemsAvailable;
    int m_capacity;
    bool m_stopping;
};

#endif // BUFFER_H
