#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>

/**
 * @brief Потокобезопасная очередь (FIFO).
 *
 * Синхронизация доступа — через std::mutex и std::condition_variable.
 * Метод pop() блокируется, пока в очереди пусто и очередь не остановлена.
 * После stop() pop() возвращает false, как только очередь опустеет; уже
 * лежащие в очереди элементы при этом продолжают извлекаться.
 */
template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;

    ThreadSafeQueue(const ThreadSafeQueue&)            = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue(ThreadSafeQueue&&)                 = delete;
    ThreadSafeQueue& operator=(ThreadSafeQueue&&)      = delete;

    /// Добавляет элемент в конец очереди и будит одного ожидающего.
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push_back(std::move(value));
        }
        cv_.notify_one();
    }

    /// Блокирующее извлечение. false — очередь пуста и остановлена.
    bool pop(T& out) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return stopped_ || !queue_.empty(); });
        if (queue_.empty()) return false;
        out = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    /// Неблокирующее извлечение.
    bool tryPop(T& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return false;
        out = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /// Копия содержимого для «снимка» состояния в UI.
    std::vector<T> snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::vector<T>(queue_.begin(), queue_.end());
    }

    /// Полная очистка (без остановки).
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.clear();
    }

    /// Остановка: разбудить всех, дальше pop() отдаёт то,
    /// что уже лежит в очереди, и затем возвращает false.
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_.notify_all();
    }

private:
    mutable std::mutex      mutex_;
    std::condition_variable cv_;
    std::deque<T>           queue_;
    bool                    stopped_ = false;
};